#pragma once

#include "IPlugin.hpp"
#include "PluginMetadata.hpp"

#include <memory>
#include <string>
#include <stdexcept>

#ifdef _WIN32
    #include <windows.h>
    #define PLUGIN_HANDLE HMODULE
#else
    #include <dlfcn.h>
    #define PLUGIN_HANDLE void*
#endif

namespace mcf {

/**
 * @brief Exception thrown when plugin loading fails
 */
class PluginLoadException : public std::runtime_error {
public:
    /**
     * @brief Constructor
     * @param message Error message describing the plugin load failure
     */
    explicit PluginLoadException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief Loaded plugin information
 *
 * Contains all data associated with a dynamically loaded plugin,
 * including the plugin instance, library handle, file path, and metadata.
 * This struct is move-only to ensure unique ownership of plugin resources.
 */
struct LoadedPlugin {
    /**
     * @brief Unique pointer to the plugin instance
     */
    std::unique_ptr<IPlugin> instance;

    /**
     * @brief Platform-specific handle to the loaded library
     *
     * HMODULE on Windows, void* on Linux/Unix
     */
    PLUGIN_HANDLE handle = nullptr;

    /**
     * @brief File system path to the plugin library
     */
    std::string path;

    /**
     * @brief Plugin metadata including name, version, and dependencies
     */
    PluginMetadata metadata;

    /**
     * @brief Default constructor
     */
    LoadedPlugin() = default;

    /**
     * @brief Destructor
     */
    ~LoadedPlugin() = default;

    /**
     * @brief Move constructor
     */
    LoadedPlugin(LoadedPlugin&&) = default;

    /**
     * @brief Move assignment operator
     */
    LoadedPlugin& operator=(LoadedPlugin&&) = default;

    /**
     * @brief Deleted copy constructor (move-only type)
     */
    LoadedPlugin(const LoadedPlugin&) = delete;

    /**
     * @brief Deleted copy assignment operator (move-only type)
     */
    LoadedPlugin& operator=(const LoadedPlugin&) = delete;
};

/**
 * @brief Dynamic plugin loader using platform-specific APIs
 *
 * Loads shared libraries (.so on Linux, .dll on Windows) and
 * resolves plugin symbols.
 */
class PluginLoader {
public:
    /**
     * @brief Load a plugin from a shared library
     * @param path Path to the plugin library file
     * @return LoadedPlugin containing the plugin instance and metadata
     * @throws PluginLoadException on failure
     */
    static LoadedPlugin loadPlugin(const std::string& path) {
        LoadedPlugin loaded;
        loaded.path = path;

        // Load the shared library
        loaded.handle = loadLibrary(path);
        if (!loaded.handle) {
            throw PluginLoadException(
                "Failed to load library: " + path + " - " + getLastError()
            );
        }

        try {
            // Get the create function
            auto createFunc = reinterpret_cast<CreatePluginFunc>(
                getSymbol(loaded.handle, "createPlugin")
            );

            if (!createFunc) {
                throw PluginLoadException(
                    "Failed to find 'createPlugin' symbol in: " + path
                );
            }

            // Create plugin instance
            IPlugin* pluginPtr = createFunc();
            if (!pluginPtr) {
                throw PluginLoadException(
                    "createPlugin() returned nullptr for: " + path
                );
            }

            loaded.instance.reset(pluginPtr);

            // Get metadata from plugin
            loaded.metadata = pluginPtr->getMetadata();

            return loaded;

        } catch (...) {
            // Clean up on error
            if (loaded.handle) {
                unloadLibrary(loaded.handle);
            }
            throw;
        }
    }

    /**
     * @brief Unload a plugin
     * @param loaded The loaded plugin to unload
     */
    static void unloadPlugin(LoadedPlugin& loaded) {
        if (loaded.instance) {
            // Shutdown plugin if still initialized
            if (loaded.instance->isInitialized()) {
                loaded.instance->shutdown();
            }

            // Get destroy function if available
            if (loaded.handle) {
                auto destroyFunc = reinterpret_cast<DestroyPluginFunc>(
                    getSymbol(loaded.handle, "destroyPlugin")
                );

                if (destroyFunc) {
                    destroyFunc(loaded.instance.release());
                } else {
                    // No custom destroy function, use default delete
                    loaded.instance.reset();
                }
            } else {
                loaded.instance.reset();
            }
        }

        // Unload the library
        if (loaded.handle) {
            unloadLibrary(loaded.handle);
            loaded.handle = nullptr;
        }
    }

    /**
     * @brief Get the manifest JSON from a plugin without fully loading it
     * @param path Path to the plugin library
     * @return JSON string containing plugin manifest
     * @throws PluginLoadException on failure
     */
    static std::string getPluginManifest(const std::string& path) {
        PLUGIN_HANDLE handle = loadLibrary(path);
        if (!handle) {
            throw PluginLoadException(
                "Failed to load library for manifest: " + path
            );
        }

        try {
            auto manifestFunc = reinterpret_cast<GetManifestFunc>(
                getSymbol(handle, "getPluginManifest")
            );

            std::string manifest;
            if (manifestFunc) {
                const char* jsonStr = manifestFunc();
                if (jsonStr) {
                    manifest = jsonStr;
                }
            }

            unloadLibrary(handle);
            return manifest;

        } catch (...) {
            unloadLibrary(handle);
            throw;
        }
    }

private:
    /**
     * @brief Load a shared library
     */
    static PLUGIN_HANDLE loadLibrary(const std::string& path) {
#ifdef _WIN32
        return LoadLibraryA(path.c_str());
#else
        // RTLD_LAZY: Resolve symbols as needed
        // RTLD_LOCAL: Don't make symbols available for subsequently loaded libraries
        return dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
#endif
    }

    /**
     * @brief Unload a shared library
     */
    static void unloadLibrary(PLUGIN_HANDLE handle) {
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
    }

    /**
     * @brief Get a symbol from a loaded library
     */
    static void* getSymbol(PLUGIN_HANDLE handle, const char* name) {
#ifdef _WIN32
        return reinterpret_cast<void*>(GetProcAddress(handle, name));
#else
        return dlsym(handle, name);
#endif
    }

    /**
     * @brief Get the last error message
     */
    static std::string getLastError() {
#ifdef _WIN32
        DWORD error = GetLastError();
        if (error == 0) {
            return "No error";
        }

        LPSTR messageBuffer = nullptr;
        size_t size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPSTR>(&messageBuffer), 0, nullptr
        );

        std::string message(messageBuffer, size);
        LocalFree(messageBuffer);
        return message;
#else
        const char* error = dlerror();
        return error ? error : "No error";
#endif
    }
};

} // namespace mcf

#pragma once

#include <string>
#include <vector>
#include <memory>

namespace mcf {

// Forward declarations
class PluginContext;
struct PluginMetadata;

/**
 * @brief Base interface for all plugins
 *
 * All plugins must inherit from this interface and implement
 * the required virtual methods. Plugins are loaded dynamically
 * at runtime through the PluginManager.
 *
 * Plugins are completely time-agnostic by default. If a plugin needs
 * real-time updates (e.g., for game logic, animations), it should
 * additionally implement IRealtimeUpdatable. If a plugin is purely
 * event-driven, it can implement IEventDriven as a marker.
 */
class IPlugin {
public:
    virtual ~IPlugin() = default;

    /**
     * @brief Get the plugin name
     * @return Unique plugin identifier
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Get the plugin version
     * @return Semantic version string (e.g., "1.0.0")
     */
    virtual std::string getVersion() const = 0;

    /**
     * @brief Get plugin metadata
     * @return Metadata containing name, version, dependencies, etc.
     */
    virtual const PluginMetadata& getMetadata() const = 0;

    /**
     * @brief Initialize the plugin
     * @param context Context providing access to core services
     * @return true if initialization succeeded, false otherwise
     */
    virtual bool initialize(PluginContext& context) = 0;

    /**
     * @brief Shutdown the plugin and cleanup resources
     */
    virtual void shutdown() = 0;

    /**
     * @brief Check if plugin is currently initialized
     * @return true if initialized, false otherwise
     */
    virtual bool isInitialized() const = 0;

    /**
     * @brief Serialize plugin state before hot reload
     * @return Serialized state as string (JSON, binary, etc.)
     *
     * Called before the plugin is unloaded during hot reload.
     * Override to save any state that should persist across reload.
     */
    virtual std::string serializeState() { return ""; }

    /**
     * @brief Deserialize plugin state after hot reload
     * @param state Serialized state string from serializeState()
     *
     * Called after the plugin is reloaded during hot reload.
     * Override to restore state that was saved in serializeState().
     */
    virtual void deserializeState(const std::string& state) { (void)state; }

    /**
     * @brief Called before hot reload begins
     *
     * Override to perform cleanup that's specific to hot reload
     * (different from regular shutdown). For example, unregister
     * services but keep their state.
     */
    virtual void onBeforeReload() {}

    /**
     * @brief Called after hot reload completes
     *
     * Override to perform reinitialization after hot reload.
     * State has already been restored via deserializeState().
     */
    virtual void onAfterReload() {}
};

/**
 * @brief Function pointer type for creating a plugin instance
 *
 * Points to the createPlugin() function exported by the plugin library.
 * This function allocates and returns a new plugin instance.
 *
 * @return Pointer to newly created IPlugin instance
 */
using CreatePluginFunc = IPlugin* (*)();

/**
 * @brief Function pointer type for destroying a plugin instance
 *
 * Points to the destroyPlugin() function exported by the plugin library.
 * This function deallocates a plugin instance that was created by createPlugin().
 *
 * @param plugin Pointer to the plugin instance to destroy
 */
using DestroyPluginFunc = void (*)(IPlugin*);

/**
 * @brief Function pointer type for retrieving plugin manifest
 *
 * Points to the getPluginManifest() function exported by the plugin library.
 * This function returns a JSON string containing plugin metadata without
 * requiring full plugin instantiation.
 *
 * @return Pointer to null-terminated JSON string containing plugin manifest
 */
using GetManifestFunc = const char* (*)();

} // namespace mcf

// Macro for exporting plugin symbols
#ifdef _WIN32
    #define PLUGIN_API __declspec(dllexport)
#else
    #define PLUGIN_API __attribute__((visibility("default")))
#endif

// Helper macro to define plugin entry points
#define MCF_PLUGIN_EXPORT(PluginClass) \
    extern "C" { \
        PLUGIN_API mcf::IPlugin* createPlugin() { \
            return new PluginClass(); \
        } \
        PLUGIN_API void destroyPlugin(mcf::IPlugin* plugin) { \
            delete plugin; \
        } \
        PLUGIN_API const char* getPluginManifest() { \
            return PluginClass::getManifestJson(); \
        } \
    }

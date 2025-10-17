#pragma once

#include "DependencyResolver.hpp"
#include "EventBus.hpp"
#include "FileWatcher.hpp"
#include "IPlugin.hpp"
#include "Logger.hpp"
#include "PluginContext.hpp"
#include "PluginLoader.hpp"
#include "ServiceLocator.hpp"
#include "ResourceManager.hpp"

#include <algorithm>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace mcf {

// Forward declarations
class Application;
class ConfigurationManager;

/**
 * @brief Manager for loading, unloading, and managing plugins
 *
 * Singleton class that handles the entire plugin lifecycle:
 * - Discovery and loading
 * - Dependency resolution
 * - Initialization and shutdown
 * - Update dispatching
 */
class PluginManager {
private:
    // Loaded plugins map
    std::map<std::string, LoadedPlugin> m_plugins;

    // Plugin load order
    std::vector<std::string> m_loadOrder;

    // Dependency resolver
    DependencyResolver m_resolver;

    // Core services
    EventBus* m_eventBus = nullptr;
    ServiceLocator* m_serviceLocator = nullptr;
    ResourceManager* m_resourceManager = nullptr;
    Application* m_application = nullptr;
    ThreadPool* m_threadPool = nullptr;
    ConfigurationManager* m_configManager = nullptr;

    // Hot reload support
    std::unique_ptr<FileWatcher> m_fileWatcher;
    bool m_hotReloadEnabled = false;
    std::map<std::string, std::string> m_pluginPaths;  // plugin name -> file path
    std::map<std::string, std::string> m_pluginStates; // plugin name -> serialized state

    // Application control (avoid circular dependency)
    std::function<void()> m_pauseCallback;
    std::function<void()> m_resumeCallback;

    // Logging
    std::shared_ptr<Logger> m_logger;

    // Thread safety
    mutable std::mutex m_mutex;

    // Singleton instance
    static PluginManager* s_instance;

    // Private constructor for singleton
    PluginManager()
        : m_fileWatcher(std::make_unique<FileWatcher>())
        , m_logger(LoggerRegistry::instance().getLogger("PluginManager"))
    {}

public:
    ~PluginManager() {
        unloadAll();
    }

    // Non-copyable
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    /**
     * @brief Get singleton instance
     * @return Reference to the singleton PluginManager instance
     */
    static PluginManager& getInstance() {
        if (!s_instance) {
            s_instance = new PluginManager();
        }
        return *s_instance;
    }

    /**
     * @brief Destroy singleton instance
     */
    static void destroy() {
        delete s_instance;
        s_instance = nullptr;
    }

    /**
     * @brief Initialize the plugin manager with core services
     * @param eventBus Pointer to the EventBus for plugin event communication
     * @param serviceLocator Pointer to the ServiceLocator for dependency injection
     * @param app Pointer to the Application instance
     * @param resourceManager Pointer to the ResourceManager (optional, can be nullptr)
     * @param threadPool Pointer to the ThreadPool for async operations (optional, can be nullptr)
     * @param configManager Pointer to the ConfigurationManager (optional, can be nullptr)
     */
    void initialize(EventBus* eventBus,
                   ServiceLocator* serviceLocator,
                   Application* app,
                   ResourceManager* resourceManager = nullptr,
                   ThreadPool* threadPool = nullptr,
                   ConfigurationManager* configManager = nullptr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_eventBus = eventBus;
        m_serviceLocator = serviceLocator;
        m_application = app;
        m_resourceManager = resourceManager;
        m_threadPool = threadPool;
        m_configManager = configManager;

        // Setup pause/resume callbacks (avoids circular dependency)
        if (app) {
            m_pauseCallback = [app]() {
                // Pause will be available after Application.hpp is fully included
                // For now, this is a placeholder that will work in .cpp files
            };
            m_resumeCallback = [app]() {
                // Resume will be available after Application.hpp is fully included
            };
        }
    }

    /**
     * @brief Load a plugin from a file
     * @param path Path to the plugin shared library
     * @return true if loaded successfully
     */
    bool loadPlugin(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_mutex);

        try {
            // Load the plugin
            LoadedPlugin loaded = PluginLoader::loadPlugin(path);
            std::string name = loaded.metadata.name;

            // Check if already loaded
            if (m_plugins.find(name) != m_plugins.end()) {
                // Plugin already loaded, unload the new one
                PluginLoader::unloadPlugin(loaded);
                return false;
            }

            // Add to dependency resolver
            m_resolver.addPlugin(loaded.metadata);

            // Validate dependencies
            m_resolver.validateMetadata(loaded.metadata);

            // Store the plugin
            m_plugins[name] = std::move(loaded);

            // Store the plugin path for hot reload
            m_pluginPaths[name] = path;

            // Setup file watching if hot reload is enabled
            if (m_hotReloadEnabled) {
                m_fileWatcher->addWatch(path, [this](const std::string& p, FileChangeType ct) {
                    onPluginFileChanged(p, ct);
                });
            }

            // Resolve dependencies and update load order
            m_loadOrder = m_resolver.resolve();

            return true;

        } catch (const std::exception& e) {
            m_logger->error("Failed to load plugin from '" + path + "': " + e.what());
            return false;
        }
    }

    /**
     * @brief Load all plugins from a directory
     * @param directory Path to directory containing plugins
     * @return Number of plugins loaded
     */
    size_t loadPluginsFromDirectory(const std::string& directory) {
        namespace fs = std::filesystem;
        size_t count = 0;

        try {
            if (!fs::exists(directory) || !fs::is_directory(directory)) {
                return 0;
            }

            for (const auto& entry : fs::directory_iterator(directory)) {
                if (!entry.is_regular_file()) {
                    continue;
                }

                std::string path = entry.path().string();
                std::string ext = entry.path().extension().string();

                // Check for plugin library extension
#ifdef _WIN32
                if (ext == ".dll")
#else
                if (ext == ".so")
#endif
                {
                    if (loadPlugin(path)) {
                        count++;
                    }
                }
            }
        } catch (const std::exception& e) {
            m_logger->error("Error while loading plugins from directory '" + directory + "': " + e.what());
        }

        return count;
    }

    /**
     * @brief Initialize all loaded plugins in dependency order
     * @return true if all plugins initialized successfully
     */
    bool initializeAll() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_eventBus || !m_serviceLocator || !m_application) {
            return false;
        }

        // Initialize in load order (dependencies first)
        for (const auto& name : m_loadOrder) {
            auto it = m_plugins.find(name);
            if (it == m_plugins.end()) {
                continue;
            }

            LoadedPlugin& plugin = it->second;

            if (plugin.instance && !plugin.instance->isInitialized()) {
                // Create context for this plugin
                PluginContext context(
                    m_eventBus,
                    m_serviceLocator,
                    m_application,
                    m_threadPool,
                    m_configManager,
                    name
                );

                // Initialize the plugin
                if (!plugin.instance->initialize(context)) {
                    m_logger->error("Plugin '" + name + "' failed to initialize");
                    return false;
                }
            }
        }

        return true;
    }

    /**
     * @brief Unload a specific plugin
     * @param name Plugin name to unload
     * @return void (plugin is silently skipped if not found)
     */
    void unloadPlugin(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_plugins.find(name);
        if (it == m_plugins.end()) {
            return;
        }

        // Unload the plugin
        PluginLoader::unloadPlugin(it->second);

        // Remove from maps
        m_plugins.erase(it);
        m_resolver.removePlugin(name);

        // Update load order
        m_loadOrder.erase(
            std::remove(m_loadOrder.begin(), m_loadOrder.end(), name),
            m_loadOrder.end()
        );
    }

    /**
     * @brief Unload all plugins in reverse order
     */
    void unloadAll() {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Unload in reverse order (reverse of dependencies)
        for (auto it = m_loadOrder.rbegin(); it != m_loadOrder.rend(); ++it) {
            auto pluginIt = m_plugins.find(*it);
            if (pluginIt != m_plugins.end()) {
                PluginLoader::unloadPlugin(pluginIt->second);
            }
        }

        m_plugins.clear();
        m_loadOrder.clear();
        m_resolver.clear();
    }


    /**
     * @brief Get a plugin by name
     * @tparam T Plugin type (must inherit from IPlugin, defaults to IPlugin)
     * @param name Plugin name to retrieve
     * @return Pointer to plugin cast to type T, or nullptr if not found or cast fails
     */
    template<typename T = IPlugin>
    T* getPlugin(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_plugins.find(name);
        if (it != m_plugins.end() && it->second.instance) {
            return dynamic_cast<T*>(it->second.instance.get());
        }
        return nullptr;
    }

    /**
     * @brief Check if a plugin is loaded
     * @param name Plugin name to check
     * @return true if the plugin is currently loaded, false otherwise
     */
    bool isLoaded(const std::string& name) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_plugins.find(name) != m_plugins.end();
    }

    /**
     * @brief Get all loaded plugin names
     * @return Vector of plugin names in dependency order (dependencies first)
     */
    std::vector<std::string> getLoadedPlugins() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_loadOrder;
    }

    /**
     * @brief Get plugin count
     * @return Number of currently loaded plugins
     */
    size_t getPluginCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_plugins.size();
    }

    /**
     * @brief Get plugin metadata
     * @param name Plugin name to query
     * @return Pointer to PluginMetadata if plugin exists, nullptr otherwise
     */
    const PluginMetadata* getPluginMetadata(const std::string& name) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_plugins.find(name);
        return (it != m_plugins.end()) ? &it->second.metadata : nullptr;
    }

    /**
     * @brief Enable hot reload monitoring for plugins
     * @param pollInterval How often to check for file changes (milliseconds)
     */
    void enableHotReload(std::chrono::milliseconds pollInterval = std::chrono::milliseconds(1000)) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_hotReloadEnabled) {
            return;
        }

        m_fileWatcher->setPollInterval(pollInterval);
        m_fileWatcher->start();
        m_hotReloadEnabled = true;
    }

    /**
     * @brief Disable hot reload monitoring
     */
    void disableHotReload() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_hotReloadEnabled) {
            return;
        }

        m_fileWatcher->stop();
        m_hotReloadEnabled = false;
    }

    /**
     * @brief Check if hot reload is enabled
     * @return true if hot reload file monitoring is active, false otherwise
     */
    bool isHotReloadEnabled() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_hotReloadEnabled;
    }

    /**
     * @brief Set pause callback for hot reload (called by Application)
     * @param callback Function to call when pausing application during plugin reload
     */
    void setPauseCallback(std::function<void()> callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pauseCallback = std::move(callback);
    }

    /**
     * @brief Set resume callback for hot reload (called by Application)
     * @param callback Function to call when resuming application after plugin reload
     */
    void setResumeCallback(std::function<void()> callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_resumeCallback = std::move(callback);
    }

    /**
     * @brief Manually reload a specific plugin
     * @param name Plugin name to reload
     * @return true if reload succeeded (including dependent plugins), false on failure
     */
    bool reloadPlugin(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_plugins.find(name);
        if (it == m_plugins.end()) {
            return false; // Plugin not loaded
        }

        auto pathIt = m_pluginPaths.find(name);
        if (pathIt == m_pluginPaths.end()) {
            return false; // No path recorded
        }

        return reloadPluginInternal(name, pathIt->second);
    }

private:
    /**
     * @brief Internal reload implementation (must be called with lock held)
     */
    bool reloadPluginInternal(const std::string& name, const std::string& path) {
        auto it = m_plugins.find(name);
        if (it == m_plugins.end()) {
            return false;
        }

        LoadedPlugin& oldPlugin = it->second;

        // Step 1: Get dependents that need to be reloaded
        std::vector<std::string> dependents = m_resolver.getDependents(name);

        // Step 2: Pause application if possible
        bool wasPaused = false;
        if (m_pauseCallback) {
            m_pauseCallback();
            wasPaused = true;
        }

        // Step 3: Save state for plugin and dependents
        std::map<std::string, std::string> states;

        if (oldPlugin.instance && oldPlugin.instance->isInitialized()) {
            oldPlugin.instance->onBeforeReload();
            states[name] = oldPlugin.instance->serializeState();
        }

        for (const auto& depName : dependents) {
            auto depIt = m_plugins.find(depName);
            if (depIt != m_plugins.end() && depIt->second.instance) {
                if (depIt->second.instance->isInitialized()) {
                    depIt->second.instance->onBeforeReload();
                    states[depName] = depIt->second.instance->serializeState();
                }
            }
        }

        // Step 4: Shutdown and unload in reverse dependency order
        // Shutdown dependents first
        for (auto rit = dependents.rbegin(); rit != dependents.rend(); ++rit) {
            auto depIt = m_plugins.find(*rit);
            if (depIt != m_plugins.end() && depIt->second.instance) {
                if (depIt->second.instance->isInitialized()) {
                    depIt->second.instance->shutdown();
                }

                // Cleanup plugin-specific registrations
                cleanupPluginResources(*rit);

                // Unload the plugin
                PluginLoader::unloadPlugin(depIt->second);
            }
        }

        // Shutdown target plugin
        if (oldPlugin.instance && oldPlugin.instance->isInitialized()) {
            oldPlugin.instance->shutdown();
        }

        // Cleanup target plugin resources
        cleanupPluginResources(name);

        // Unload target plugin
        PluginLoader::unloadPlugin(oldPlugin);

        // Step 5: Reload plugins
        try {
            // Reload target plugin
            LoadedPlugin newPlugin = PluginLoader::loadPlugin(path);

            if (newPlugin.metadata.name != name) {
                // Plugin name changed, this is an error
                PluginLoader::unloadPlugin(newPlugin);

                // Try to restore old plugin
                restoreFailedReload(name, path, dependents, states);
                if (wasPaused && m_resumeCallback) m_resumeCallback();
                return false;
            }

            m_plugins[name] = std::move(newPlugin);

            // Reload dependents
            for (const auto& depName : dependents) {
                auto depPathIt = m_pluginPaths.find(depName);
                if (depPathIt != m_pluginPaths.end()) {
                    LoadedPlugin depPlugin = PluginLoader::loadPlugin(depPathIt->second);
                    m_plugins[depName] = std::move(depPlugin);
                }
            }

            // Step 6: Reinitialize in dependency order
            // Reinitialize target plugin
            if (m_plugins[name].instance) {
                PluginContext context(m_eventBus, m_serviceLocator, m_application, m_threadPool, m_configManager, name);

                if (!m_plugins[name].instance->initialize(context)) {
                    // Initialization failed
                    restoreFailedReload(name, path, dependents, states);
                    if (wasPaused && m_resumeCallback) m_resumeCallback();
                    return false;
                }

                // Restore state
                if (states.find(name) != states.end()) {
                    m_plugins[name].instance->deserializeState(states[name]);
                }

                m_plugins[name].instance->onAfterReload();
            }

            // Reinitialize dependents
            for (const auto& depName : dependents) {
                auto depIt = m_plugins.find(depName);
                if (depIt != m_plugins.end() && depIt->second.instance) {
                    PluginContext context(m_eventBus, m_serviceLocator, m_application, m_threadPool, m_configManager, depName);

                    if (!depIt->second.instance->initialize(context)) {
                        // Initialization failed
                        restoreFailedReload(name, path, dependents, states);
                        if (wasPaused && m_resumeCallback) m_resumeCallback();
                        return false;
                    }

                    // Restore state
                    if (states.find(depName) != states.end()) {
                        depIt->second.instance->deserializeState(states[depName]);
                    }

                    depIt->second.instance->onAfterReload();
                }
            }

            // Step 7: Resume application
            if (wasPaused && m_resumeCallback) {
                m_resumeCallback();
            }

            return true;

        } catch (const std::exception& e) {
            // Reload failed, try to restore old plugin
            restoreFailedReload(name, path, dependents, states);
            if (wasPaused && m_resumeCallback) m_resumeCallback();
            return false;
        }
    }

    /**
     * @brief Cleanup plugin-specific resources from core services
     */
    void cleanupPluginResources(const std::string& pluginId) {
        if (m_eventBus) {
            m_eventBus->unsubscribePlugin(pluginId);
        }

        if (m_serviceLocator) {
            m_serviceLocator->unregisterPlugin(pluginId);
        }

        if (m_resourceManager) {
            m_resourceManager->unloadPlugin(pluginId);
        }
    }

    /**
     * @brief Restore plugins after failed reload
     */
    void restoreFailedReload(const std::string& name,
                            const std::string& path,
                            const std::vector<std::string>& dependents,
                            const std::map<std::string, std::string>& states) {
        // Try to reload the old version
        try {
            LoadedPlugin restored = PluginLoader::loadPlugin(path);
            m_plugins[name] = std::move(restored);

            if (m_plugins[name].instance) {
                PluginContext context(m_eventBus, m_serviceLocator, m_application, m_threadPool, m_configManager, name);
                m_plugins[name].instance->initialize(context);

                auto it = states.find(name);
                if (it != states.end()) {
                    m_plugins[name].instance->deserializeState(it->second);
                }
            }
        } catch (...) {
            // Can't restore, plugin will remain unloaded
        }
    }

    /**
     * @brief Callback for file watcher
     */
    void onPluginFileChanged(const std::string& path, FileChangeType changeType) {
        if (changeType != FileChangeType::Modified) {
            return; // Only handle modifications
        }

        // Find which plugin this path belongs to
        std::string pluginName;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (const auto& [name, pluginPath] : m_pluginPaths) {
                if (pluginPath == path) {
                    pluginName = name;
                    break;
                }
            }
        }

        if (!pluginName.empty()) {
            reloadPlugin(pluginName);
        }
    }
};

// Static instance initialization
inline PluginManager* PluginManager::s_instance = nullptr;

} // namespace mcf

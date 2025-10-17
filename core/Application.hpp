#pragma once

#include "ConfigurationManager.hpp"
#include "EventBus.hpp"
#include "IModule.hpp"
#include "PluginManager.hpp"
#include "ResourceManager.hpp"
#include "ServiceLocator.hpp"
#include "ThreadPool.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace mcf {

/**
 * @brief Application configuration
 *
 * Configuration structure defining application-wide settings and behaviors.
 */
struct ApplicationConfig {
    /** @brief Application name for identification purposes */
    std::string name = "ModularCppApp";

    /** @brief Application version string (semantic versioning recommended) */
    std::string version = "1.0.0";

    /** @brief Directory path where plugins are located and will be loaded from */
    std::string pluginDirectory = "./plugins";

    /**
     * @brief Path to configuration file for persistent settings
     *
     * Empty string disables auto-loading from file. When specified, configuration
     * will be loaded during initialization and can override other settings.
     */
    std::string configFile = "";

    /**
     * @brief Whether to automatically discover and load plugins from pluginDirectory
     *
     * If true, all plugins in the plugin directory will be loaded during initialization.
     * If false, plugins must be loaded manually via PluginManager.
     */
    bool autoLoadPlugins = true;

    /**
     * @brief Whether to automatically initialize all loaded plugins
     *
     * If true, all loaded plugins will be initialized during application initialization.
     * If false, plugins must be initialized manually via PluginManager.
     */
    bool autoInitPlugins = true;

    /**
     * @brief Number of worker threads in the thread pool
     *
     * 0 = auto-detect based on std::thread::hardware_concurrency().
     * Manual specification allows control over thread pool size.
     */
    size_t threadPoolSize = 0;
};

/**
 * @brief Base application class with plugin and module support
 *
 * Provides the foundation for creating modular applications with:
 * - Plugin system (dynamic loading)
 * - Module system (static linking)
 * - Event bus (pub/sub messaging)
 * - Service locator (dependency injection)
 * - Resource manager (caching, lifetime management)
 * - Configuration management
 * - Thread pool
 *
 * This class is completely time-agnostic and suitable for any application type:
 * - Web services / REST APIs
 * - CLI tools
 * - Batch processors
 * - Event-driven systems
 * - Real-time applications (when combined with RealtimeModule)
 *
 * For applications requiring a game loop or frame-based updates,
 * add the RealtimeModule to your application.
 */
class Application {
protected:
    // Application state
    bool m_running = false;
    bool m_initialized = false;
    ApplicationConfig m_config;

    // Core systems
    std::unique_ptr<EventBus> m_eventBus;
    std::unique_ptr<ServiceLocator> m_serviceLocator;
    std::unique_ptr<ResourceManager> m_resourceManager;
    std::unique_ptr<ConfigurationManager> m_configManager;
    std::unique_ptr<ThreadPool> m_threadPool;
    PluginManager& m_pluginManager;

    // Modules
    std::vector<std::unique_ptr<IModule>> m_modules;

public:
    /**
     * @brief Constructs the application with the given configuration
     *
     * Creates all core systems including EventBus, ServiceLocator, ResourceManager,
     * ConfigurationManager, and ThreadPool. The PluginManager singleton is obtained
     * and will be initialized during the initialize() call.
     *
     * @param config Configuration settings for the application. Defaults to ApplicationConfig()
     *               with standard settings if not provided.
     *
     * @note The application is not ready to use after construction. Call initialize()
     *       before using the application or accessing its services.
     *
     * @see initialize()
     */
    explicit Application(const ApplicationConfig& config = ApplicationConfig())
        : m_config(config)
        , m_pluginManager(PluginManager::getInstance()) {

        // Create core systems
        m_eventBus = std::make_unique<EventBus>();
        m_serviceLocator = std::make_unique<ServiceLocator>();
        m_resourceManager = std::make_unique<ResourceManager>();
        m_configManager = std::make_unique<ConfigurationManager>();
        m_threadPool = std::make_unique<ThreadPool>(config.threadPoolSize);
    }

    /**
     * @brief Virtual destructor ensures proper cleanup of derived classes
     *
     * Automatically calls shutdown() if the application is still initialized,
     * ensuring all plugins, modules, and resources are properly cleaned up
     * following RAII principles.
     *
     * @note Plugin and module shutdown occurs in reverse initialization order
     *       to respect dependencies.
     */
    virtual ~Application() {
        if (m_initialized) {
            shutdown();
        }
    }

    /**
     * @brief Initialize the application
     * @return true if initialization succeeded
     */
    virtual bool initialize() {
        if (m_initialized) {
            return true;
        }

        // Load configuration if specified
        if (!m_config.configFile.empty()) {
            if (!m_configManager->load(m_config.configFile)) {
                // Config file doesn't exist, will be created on first save
                // This is not an error - we'll use default values
            }

            // Override config from file if present
            if (m_configManager->has("application.name")) {
                m_config.name = m_configManager->getString("application.name");
            }
            if (m_configManager->has("application.version")) {
                m_config.version = m_configManager->getString("application.version");
            }
            if (m_configManager->has("application.pluginDirectory")) {
                m_config.pluginDirectory = m_configManager->getString("application.pluginDirectory");
            }
            if (m_configManager->has("application.autoLoadPlugins")) {
                m_config.autoLoadPlugins = m_configManager->getBool("application.autoLoadPlugins");
            }
            if (m_configManager->has("application.autoInitPlugins")) {
                m_config.autoInitPlugins = m_configManager->getBool("application.autoInitPlugins");
            }
            if (m_configManager->has("application.threadPoolSize")) {
                m_config.threadPoolSize = static_cast<size_t>(m_configManager->getInt("application.threadPoolSize"));
            }
        }

        // Register core services
        m_serviceLocator->registerSingleton<EventBus>(
            std::shared_ptr<EventBus>(m_eventBus.get(), [](EventBus*){}));
        m_serviceLocator->registerSingleton<ResourceManager>(
            std::shared_ptr<ResourceManager>(m_resourceManager.get(), [](ResourceManager*){}));
        m_serviceLocator->registerSingleton<ConfigurationManager>(
            std::shared_ptr<ConfigurationManager>(m_configManager.get(), [](ConfigurationManager*){}));

        // Initialize plugin manager
        m_pluginManager.initialize(
            m_eventBus.get(),
            m_serviceLocator.get(),
            this,
            m_resourceManager.get(),
            m_threadPool.get(),
            m_configManager.get()
        );

        // Setup hot reload callbacks (avoids circular dependency)
        // TODO: Hot reload needs to be adapted for time-agnostic architecture
        // Pause/resume should be handled by RealtimeModule if present
        // m_pluginManager.setPauseCallback([this]() { /* pause realtime module */ });
        // m_pluginManager.setResumeCallback([this]() { /* resume realtime module */ });

        // Initialize modules by priority
        std::sort(m_modules.begin(), m_modules.end(),
            [](const auto& a, const auto& b) {
                return a->getPriority() > b->getPriority();
            }
        );

        for (auto& module : m_modules) {
            if (!module->initialize(*this)) {
                // Module initialization failed
                return false;
            }
        }

        // Application-specific initialization
        if (!onInitialize()) {
            return false;
        }

        // Load plugins if configured
        if (m_config.autoLoadPlugins) {
            m_pluginManager.loadPluginsFromDirectory(m_config.pluginDirectory);
        }

        // Initialize plugins if configured
        if (m_config.autoInitPlugins) {
            if (!m_pluginManager.initializeAll()) {
                return false;
            }
        }

        m_initialized = true;
        return true;
    }

    /**
     * @brief Shutdown the application
     */
    virtual void shutdown() {
        if (!m_initialized) {
            return;
        }

        // Application-specific shutdown
        onShutdown();

        // Unload all plugins
        m_pluginManager.unloadAll();

        // Shutdown modules in reverse order
        for (auto it = m_modules.rbegin(); it != m_modules.rend(); ++it) {
            (*it)->shutdown();
        }

        // Clear resources
        m_resourceManager->clear();
        m_eventBus->clear();
        m_serviceLocator->clear();

        m_initialized = false;
    }

    /**
     * @brief Run the application
     *
     * Default implementation does nothing - application stays initialized
     * and running until stop() is called. Override this for custom behavior:
     * - CLI apps: execute commands and return
     * - Event-driven apps: wait for events
     * - Real-time apps: use RealtimeModule and call its run()
     */
    virtual void run() {
        if (!m_initialized) {
            if (!initialize()) {
                return;
            }
        }

        m_running = true;

        // Default: just keep running until stopped
        // Derived classes override this for their specific behavior
    }

    /**
     * @brief Stop the application
     */
    virtual void stop() {
        m_running = false;
    }

    /**
     * @brief Check if application is running
     *
     * @return true if the application is currently running (between run() and stop() calls),
     *         false otherwise
     */
    bool isRunning() const { return m_running; }

    /**
     * @brief Check if application is initialized
     *
     * @return true if initialize() has been called successfully and shutdown() has not
     *         been called yet, false otherwise
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Get application configuration
     *
     * @return Const reference to the ApplicationConfig structure containing
     *         all application settings including name, version, plugin directory,
     *         and behavioral flags
     */
    const ApplicationConfig& getConfig() const { return m_config; }

    /**
     * @brief Get event bus
     *
     * The EventBus provides publish-subscribe messaging between plugins and modules.
     *
     * @return Pointer to the EventBus instance. Never null after construction.
     *
     * @see EventBus
     */
    EventBus* getEventBus() { return m_eventBus.get(); }

    /**
     * @brief Get service locator
     *
     * The ServiceLocator provides dependency injection for services with
     * configurable lifetimes (singleton, transient, scoped).
     *
     * @return Pointer to the ServiceLocator instance. Never null after construction.
     *
     * @see ServiceLocator
     */
    ServiceLocator* getServiceLocator() { return m_serviceLocator.get(); }

    /**
     * @brief Get resource manager
     *
     * The ResourceManager handles loading, caching, and lifetime management
     * of resources with automatic reference counting.
     *
     * @return Pointer to the ResourceManager instance. Never null after construction.
     *
     * @see ResourceManager
     */
    ResourceManager* getResourceManager() { return m_resourceManager.get(); }

    /**
     * @brief Get configuration manager
     *
     * The ConfigurationManager handles loading/saving configuration from files
     * with support for various data types and nested keys.
     *
     * @return Pointer to the ConfigurationManager instance. Never null after construction.
     *
     * @see ConfigurationManager
     */
    ConfigurationManager* getConfigurationManager() { return m_configManager.get(); }

    /**
     * @brief Get plugin manager
     *
     * The PluginManager handles dynamic loading, initialization, and lifecycle
     * management of plugins with dependency resolution.
     *
     * @return Reference to the singleton PluginManager instance.
     *
     * @see PluginManager
     */
    PluginManager& getPluginManager() { return m_pluginManager; }

    /**
     * @brief Add a module to the application
     *
     * Creates a new module of type T with the given constructor arguments and
     * adds it to the application's module list. Modules are statically linked
     * components that are initialized by priority order during application
     * initialization.
     *
     * @tparam T The module type to create. Must derive from IModule.
     * @tparam Args Variadic template parameter pack for the module's constructor arguments.
     *
     * @param args Constructor arguments to forward to the module's constructor.
     *             Arguments are perfectly forwarded using std::forward.
     *
     * @return Pointer to the created module. The pointer remains valid for the
     *         lifetime of the application. Ownership is retained by the Application.
     *
     * @note Modules are initialized during Application::initialize() in priority order
     *       (higher priority values initialize first). The module must not be added
     *       after initialization has occurred.
     *
     * @see IModule, getModule()
     *
     * Example usage:
     * @code
     * auto* realtimeModule = app.addModule<RealtimeModule>(60.0f); // 60 FPS target
     * auto* logModule = app.addModule<LoggingModule>("app.log");
     * @endcode
     */
    template<typename T, typename... Args>
    T* addModule(Args&&... args) {
        auto module = std::make_unique<T>(std::forward<Args>(args)...);
        T* modulePtr = module.get();
        m_modules.push_back(std::move(module));
        return modulePtr;
    }

    /**
     * @brief Get a module by type
     *
     * Searches through all registered modules and returns the first module
     * that can be cast to type T using dynamic_cast. This allows retrieving
     * modules by their interface or concrete type.
     *
     * @tparam T The module type to search for. Can be a concrete module class
     *           or an interface that the module implements.
     *
     * @return Pointer to the module if found, nullptr if no module of type T exists.
     *         The pointer remains valid for the lifetime of the application.
     *
     * @note If multiple modules of the same type exist, only the first one
     *       (in registration order) will be returned.
     *
     * @see addModule()
     *
     * Example usage:
     * @code
     * auto* realtimeModule = app.getModule<RealtimeModule>();
     * if (realtimeModule) {
     *     realtimeModule->setTargetFPS(120.0f);
     * }
     * @endcode
     */
    template<typename T>
    T* getModule() {
        for (auto& module : m_modules) {
            if (auto* typedModule = dynamic_cast<T*>(module.get())) {
                return typedModule;
            }
        }
        return nullptr;
    }

protected:
    /**
     * @brief Called during initialization (override in derived class)
     * @return true if initialization succeeded
     */
    virtual bool onInitialize() { return true; }

    /**
     * @brief Called during shutdown (override in derived class)
     */
    virtual void onShutdown() {}
};

} // namespace mcf

#pragma once

#include <memory>
#include <string>

namespace mcf {

// Forward declarations
class EventBus;
class ServiceLocator;
class Application;
class ThreadPool;
class ConfigurationManager;

/**
 * @brief Context provided to plugins during initialization
 *
 * Provides access to core services and application features.
 * Each plugin receives a context when initialized.
 */
class PluginContext {
private:
    EventBus* m_eventBus = nullptr;
    ServiceLocator* m_serviceLocator = nullptr;
    Application* m_application = nullptr;
    ThreadPool* m_threadPool = nullptr;
    ConfigurationManager* m_configManager = nullptr;
    std::string m_pluginName;

public:
    /**
     * @brief Default constructor - creates an invalid context
     */
    PluginContext() = default;

    /**
     * @brief Constructor with full initialization
     * @param eventBus Pointer to the event bus instance
     * @param serviceLocator Pointer to the service locator instance
     * @param app Pointer to the application instance
     * @param threadPool Pointer to the thread pool instance
     * @param configManager Pointer to the configuration manager instance
     * @param pluginName Name of the plugin owning this context
     */
    PluginContext(EventBus* eventBus,
                 ServiceLocator* serviceLocator,
                 Application* app,
                 ThreadPool* threadPool,
                 ConfigurationManager* configManager,
                 const std::string& pluginName)
        : m_eventBus(eventBus)
        , m_serviceLocator(serviceLocator)
        , m_application(app)
        , m_threadPool(threadPool)
        , m_configManager(configManager)
        , m_pluginName(pluginName) {}

    /**
     * @brief Get the event bus for publishing/subscribing to events
     * @return Pointer to the EventBus instance, or nullptr if not available
     */
    EventBus* getEventBus() const { return m_eventBus; }

    /**
     * @brief Get the service locator for dependency injection
     * @return Pointer to the ServiceLocator instance, or nullptr if not available
     */
    ServiceLocator* getServiceLocator() const { return m_serviceLocator; }

    /**
     * @brief Get reference to the application
     * @return Pointer to the Application instance, or nullptr if not available
     */
    Application* getApplication() const { return m_application; }

    /**
     * @brief Get the name of the plugin owning this context
     * @return Reference to the plugin name string
     */
    const std::string& getPluginName() const { return m_pluginName; }

    /**
     * @brief Get the thread pool for async task execution
     * @return Pointer to the ThreadPool instance, or nullptr if not available
     */
    ThreadPool* getThreadPool() const { return m_threadPool; }

    /**
     * @brief Get the configuration manager for plugin settings
     * @return Pointer to the ConfigurationManager instance, or nullptr if not available
     */
    ConfigurationManager* getConfigurationManager() const { return m_configManager; }

    /**
     * @brief Check if context is valid
     */
    bool isValid() const {
        return m_eventBus != nullptr &&
               m_serviceLocator != nullptr &&
               m_application != nullptr &&
               m_threadPool != nullptr &&
               m_configManager != nullptr;
    }
};

} // namespace mcf

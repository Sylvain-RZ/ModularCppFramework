#pragma once

#include <string>
#include <memory>

namespace mcf {

// Forward declaration
class Application;

/**
 * @brief Base interface for core application modules
 *
 * Modules are internal components that provide core functionality
 * to the application. Unlike plugins, modules are statically linked
 * and loaded at compile time.
 *
 * Modules are time-agnostic by default. If a module needs real-time
 * updates (e.g., rendering, physics), it should additionally implement
 * IRealtimeUpdatable. Event-driven modules can implement IEventDriven.
 */
class IModule {
public:
    virtual ~IModule() = default;

    /**
     * @brief Get the module name
     * @return Unique module identifier
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Get the module version
     * @return Semantic version string
     */
    virtual std::string getVersion() const = 0;

    /**
     * @brief Initialize the module
     * @param app Reference to the parent application
     * @return true if initialization succeeded
     */
    virtual bool initialize(Application& app) = 0;

    /**
     * @brief Shutdown the module
     */
    virtual void shutdown() = 0;

    /**
     * @brief Check if module is initialized
     */
    virtual bool isInitialized() const = 0;

    /**
     * @brief Get module priority for initialization order
     * @return Higher priority = initialized earlier
     */
    virtual int getPriority() const { return 100; }
};

/**
 * @brief Base class for implementing modules with common functionality
 */
class ModuleBase : public IModule {
protected:
    bool m_initialized = false;
    std::string m_name;
    std::string m_version;
    int m_priority;

public:
    ModuleBase(const std::string& name,
              const std::string& version = "1.0.0",
              int priority = 100)
        : m_name(name), m_version(version), m_priority(priority) {}

    virtual ~ModuleBase() = default;

    std::string getName() const override { return m_name; }
    std::string getVersion() const override { return m_version; }
    bool isInitialized() const override { return m_initialized; }
    int getPriority() const override { return m_priority; }
};

} // namespace mcf

#pragma once

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <vector>
#include <atomic>

namespace mcf {

// Forward declaration
class ServiceLocator;

/**
 * @brief Service lifetime management strategies
 */
enum class ServiceLifetime {
    Singleton,   // One instance for entire application
    Transient,   // New instance every time
    Scoped       // One instance per scope
};

/**
 * @brief Unique identifier for a scope
 */
using ScopeId = size_t;

/**
 * @brief Service registration information
 */
struct ServiceInfo {
    std::any instance;                    ///< Current service instance (if singleton)
    std::function<std::any()> factory;    ///< Factory function to create new instances
    ServiceLifetime lifetime;             ///< Service lifetime strategy
    std::type_index typeIndex;            ///< Type information for the service
    std::string pluginId;                 ///< Optional plugin identifier for cleanup

    // Scoped instances: map of scope ID -> instance
    std::map<ScopeId, std::any> scopedInstances;

    /**
     * @brief Construct service information
     * @param inst Service instance (for singleton)
     * @param fact Factory function to create instances
     * @param life Service lifetime strategy
     * @param type Type index of the service
     * @param pid Plugin identifier for tracking (optional)
     */
    ServiceInfo(std::any inst, std::function<std::any()> fact,
               ServiceLifetime life, std::type_index type, std::string pid = "")
        : instance(std::move(inst))
        , factory(std::move(fact))
        , lifetime(life)
        , typeIndex(type)
        , pluginId(std::move(pid)) {}
};

/**
 * @brief Service locator for dependency injection
 *
 * Provides a central registry for services that can be accessed
 * by plugins and modules. Supports different lifetime strategies.
 */
class ServiceLocator {
private:
    // Type-based service registry
    std::map<std::type_index, ServiceInfo> m_services;

    // Name-based service registry
    std::map<std::string, std::any> m_namedServices;

    // Scope management
    std::vector<ScopeId> m_scopeStack;
    std::atomic<ScopeId> m_nextScopeId{1};

    // Thread safety
    mutable std::mutex m_mutex;

    /**
     * @brief Get current scope ID (top of stack)
     * @return Current scope ID, or 0 if no scope active
     */
    ScopeId getCurrentScope() const {
        return m_scopeStack.empty() ? 0 : m_scopeStack.back();
    }

public:
    ServiceLocator() = default;
    ~ServiceLocator() = default;

    // Non-copyable
    ServiceLocator(const ServiceLocator&) = delete;
    ServiceLocator& operator=(const ServiceLocator&) = delete;

    /**
     * @brief Register a singleton service instance
     * @tparam T Service interface type
     * @param instance The service instance
     */
    template<typename T>
    void registerSingleton(std::shared_ptr<T> instance) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto typeIdx = std::type_index(typeid(T));
        m_services.emplace(
            typeIdx,
            ServiceInfo(instance, nullptr, ServiceLifetime::Singleton, typeIdx)
        );
    }

    /**
     * @brief Register a service with a factory function
     * @tparam T Service interface type
     * @param factory Factory function to create instances
     * @param lifetime Service lifetime strategy (default: Transient)
     */
    template<typename T>
    void registerFactory(std::function<std::shared_ptr<T>()> factory,
                        ServiceLifetime lifetime = ServiceLifetime::Transient) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto typeIdx = std::type_index(typeid(T));

        // For singleton, create instance immediately
        std::any instance;
        if (lifetime == ServiceLifetime::Singleton) {
            instance = factory();
        }

        m_services.emplace(
            typeIdx,
            ServiceInfo(
                instance,
                [factory]() -> std::any { return factory(); },
                lifetime,
                typeIdx
            )
        );
    }

    /**
     * @brief Register a service type with automatic construction
     * @tparam TInterface Service interface type
     * @tparam TImpl Service implementation type
     * @param lifetime Service lifetime strategy (default: Singleton)
     */
    template<typename TInterface, typename TImpl>
    void registerType(ServiceLifetime lifetime = ServiceLifetime::Singleton) {
        auto factory = []() -> std::shared_ptr<TInterface> {
            return std::make_shared<TImpl>();
        };

        registerFactory<TInterface>(factory, lifetime);
    }

    /**
     * @brief Register a singleton service instance with plugin tracking
     * @tparam T Service interface type
     * @param instance The service instance to register
     * @param pluginId Plugin identifier for cleanup tracking
     */
    template<typename T>
    void registerSingletonWithPlugin(std::shared_ptr<T> instance, const std::string& pluginId) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto typeIdx = std::type_index(typeid(T));
        m_services.emplace(
            typeIdx,
            ServiceInfo(instance, nullptr, ServiceLifetime::Singleton, typeIdx, pluginId)
        );
    }

    /**
     * @brief Register a named service
     * @tparam T Service type
     * @param name Unique name for the service
     * @param instance Service instance to register
     */
    template<typename T>
    void registerNamed(const std::string& name, std::shared_ptr<T> instance) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_namedServices[name] = instance;
    }

    /**
     * @brief Resolve a service by type
     * @tparam T Service type
     * @return Shared pointer to service instance
     * @throws std::runtime_error if service not found
     */
    template<typename T>
    std::shared_ptr<T> resolve() {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto typeIdx = std::type_index(typeid(T));
        auto it = m_services.find(typeIdx);

        if (it == m_services.end()) {
            throw std::runtime_error(
                "Service not registered: " + std::string(typeid(T).name())
            );
        }

        ServiceInfo& info = it->second;

        switch (info.lifetime) {
            case ServiceLifetime::Singleton:
                return std::any_cast<std::shared_ptr<T>>(info.instance);

            case ServiceLifetime::Transient:
                if (info.factory) {
                    return std::any_cast<std::shared_ptr<T>>(info.factory());
                }
                throw std::runtime_error("Transient service has no factory");

            case ServiceLifetime::Scoped:
                {
                    ScopeId currentScope = getCurrentScope();
                    if (currentScope == 0) {
                        throw std::runtime_error(
                            "Cannot resolve scoped service outside of a scope. "
                            "Use ServiceScope to create a scope."
                        );
                    }

                    // Check if instance exists for this scope
                    auto scopeIt = info.scopedInstances.find(currentScope);
                    if (scopeIt != info.scopedInstances.end()) {
                        return std::any_cast<std::shared_ptr<T>>(scopeIt->second);
                    }

                    // Create new instance for this scope
                    if (!info.factory) {
                        throw std::runtime_error("Scoped service has no factory");
                    }

                    auto instance = info.factory();
                    info.scopedInstances[currentScope] = instance;
                    return std::any_cast<std::shared_ptr<T>>(instance);
                }
        }

        throw std::runtime_error("Invalid service lifetime");
    }

    /**
     * @brief Resolve a service by type, returns nullptr if not found
     * @tparam T Service type to resolve
     * @return Shared pointer to service instance, or nullptr if not found
     */
    template<typename T>
    std::shared_ptr<T> tryResolve() {
        try {
            return resolve<T>();
        } catch (...) {
            return nullptr;
        }
    }

    /**
     * @brief Resolve a named service
     * @tparam T Service type to resolve
     * @param name Unique name of the service
     * @return Shared pointer to service instance
     * @throws std::runtime_error if service not found
     */
    template<typename T>
    std::shared_ptr<T> resolveNamed(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_namedServices.find(name);
        if (it == m_namedServices.end()) {
            throw std::runtime_error("Named service not found: " + name);
        }

        return std::any_cast<std::shared_ptr<T>>(it->second);
    }

    /**
     * @brief Resolve a named service, returns nullptr if not found
     * @tparam T Service type to resolve
     * @param name Unique name of the service
     * @return Shared pointer to service instance, or nullptr if not found
     */
    template<typename T>
    std::shared_ptr<T> tryResolveNamed(const std::string& name) {
        try {
            return resolveNamed<T>(name);
        } catch (...) {
            return nullptr;
        }
    }

    /**
     * @brief Check if a service is registered
     * @tparam T Service type to check
     * @return True if service is registered, false otherwise
     */
    template<typename T>
    bool isRegistered() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_services.find(std::type_index(typeid(T))) != m_services.end();
    }

    /**
     * @brief Check if a named service is registered
     * @param name Unique name of the service
     * @return True if named service is registered, false otherwise
     */
    bool isRegisteredNamed(const std::string& name) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_namedServices.find(name) != m_namedServices.end();
    }

    /**
     * @brief Unregister a service
     * @tparam T Service type to unregister
     */
    template<typename T>
    void unregister() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_services.erase(std::type_index(typeid(T)));
    }

    /**
     * @brief Unregister a named service
     * @param name Unique name of the service to unregister
     */
    void unregisterNamed(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_namedServices.erase(name);
    }

    /**
     * @brief Unregister all services registered by a specific plugin
     * @param pluginId Plugin identifier
     * @return Number of services removed
     */
    size_t unregisterPlugin(const std::string& pluginId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t count = 0;

        // Remove services registered by this plugin
        for (auto it = m_services.begin(); it != m_services.end(); ) {
            if (it->second.pluginId == pluginId) {
                it = m_services.erase(it);
                count++;
            } else {
                ++it;
            }
        }

        return count;
    }

    /**
     * @brief Clear all registered services
     */
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_services.clear();
        m_namedServices.clear();
    }

    /**
     * @brief Get count of registered services
     * @return Total number of registered services (type-based and named)
     */
    size_t serviceCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_services.size() + m_namedServices.size();
    }

    /**
     * @brief Enter a new scope
     * @return Scope ID of the new scope
     */
    ScopeId enterScope() {
        std::lock_guard<std::mutex> lock(m_mutex);
        ScopeId newScope = m_nextScopeId++;
        m_scopeStack.push_back(newScope);
        return newScope;
    }

    /**
     * @brief Exit the current scope and cleanup scoped services
     * @throws std::runtime_error if no scope is active
     */
    void exitScope() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_scopeStack.empty()) {
            throw std::runtime_error("Cannot exit scope: no scope is active");
        }

        ScopeId scopeToExit = m_scopeStack.back();
        m_scopeStack.pop_back();

        // Cleanup all scoped instances for this scope
        for (auto& [typeIdx, serviceInfo] : m_services) {
            if (serviceInfo.lifetime == ServiceLifetime::Scoped) {
                serviceInfo.scopedInstances.erase(scopeToExit);
            }
        }
    }

    /**
     * @brief Check if currently inside a scope
     * @return True if inside a scope, false otherwise
     */
    bool isInScope() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return !m_scopeStack.empty();
    }

    /**
     * @brief Get the depth of the current scope stack
     * @return Number of nested scopes
     */
    size_t scopeDepth() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_scopeStack.size();
    }
};

/**
 * @brief RAII guard for automatic scope management
 *
 * Creates a scope on construction and automatically exits on destruction.
 * Use this to ensure proper cleanup of scoped services.
 *
 * Example:
 * @code
 * {
 *     ServiceScope scope(serviceLocator);
 *     auto service = serviceLocator.resolve<IScopedService>();
 *     // Use service...
 * } // Scope automatically exited, scoped services cleaned up
 * @endcode
 */
class ServiceScope {
private:
    ServiceLocator& m_locator;
    ScopeId m_scopeId;

public:
    /**
     * @brief Construct a scope guard and enter a new scope
     * @param locator Reference to the ServiceLocator
     */
    explicit ServiceScope(ServiceLocator& locator)
        : m_locator(locator)
        , m_scopeId(locator.enterScope())
    {}

    /**
     * @brief Destroy the scope guard and exit the scope
     */
    ~ServiceScope() {
        try {
            m_locator.exitScope();
        } catch (...) {
            // Destructors should not throw
        }
    }

    // Non-copyable and non-movable
    ServiceScope(const ServiceScope&) = delete;
    ServiceScope& operator=(const ServiceScope&) = delete;
    ServiceScope(ServiceScope&&) = delete;
    ServiceScope& operator=(ServiceScope&&) = delete;

    /**
     * @brief Get the ID of this scope
     * @return Scope ID
     */
    ScopeId getScopeId() const { return m_scopeId; }
};

} // namespace mcf

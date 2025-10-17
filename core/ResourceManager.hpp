#pragma once

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>

namespace mcf {

/**
 * @brief Resource handle ID for tracking resources
 */
using ResourceHandleID = size_t;

/**
 * @brief Resource metadata
 *
 * Stores information about a loaded resource including its type-erased data,
 * reference count, caching status, and optional plugin ownership.
 */
struct ResourceInfo {
    /**
     * @brief Type-erased resource data stored as std::any
     *
     * Actual type is std::shared_ptr<T> where T is the resource type.
     * Use std::any_cast<std::shared_ptr<T>> to retrieve the typed resource.
     */
    std::any resource;

    /**
     * @brief Path or identifier used to load the resource
     *
     * Serves as the unique key in the resource cache.
     */
    std::string path;

    /**
     * @brief Type information for runtime type checking
     *
     * Used to verify type safety when casting the type-erased resource.
     */
    std::type_index typeIndex;

    /**
     * @brief Number of active references to this resource
     *
     * Incremented on load/get, decremented on release.
     * When reaches 0 and cached=false, resource may be unloaded.
     */
    size_t referenceCount = 0;

    /**
     * @brief Whether to keep resource in cache when reference count reaches zero
     *
     * true = keep in memory, false = unload when no references remain
     */
    bool cached = true;

    /**
     * @brief Optional plugin identifier for cleanup tracking
     *
     * If set, allows bulk unloading of all resources owned by a plugin
     * via unloadPlugin(). Empty string indicates no plugin ownership.
     */
    std::string pluginId;

    /**
     * @brief Construct a ResourceInfo object
     * @param res Type-erased resource as std::any
     * @param p Resource path or identifier
     * @param type Type index for runtime type checking
     * @param pid Plugin identifier (empty for non-plugin resources)
     */
    ResourceInfo(std::any res, const std::string& p, std::type_index type, std::string pid = "")
        : resource(std::move(res)), path(p), typeIndex(type), pluginId(std::move(pid)) {}
};

/**
 * @brief Resource loader function type
 * @tparam T Resource type
 */
template<typename T>
using ResourceLoader = std::function<std::shared_ptr<T>(const std::string&)>;

/**
 * @brief Manager for shared resources with caching and reference counting
 *
 * Provides centralized resource management with:
 * - Automatic reference counting
 * - Resource caching
 * - Type-safe loading
 * - Custom loader registration
 */
class ResourceManager {
private:
    // Resource storage (path -> resource)
    std::unordered_map<std::string, std::shared_ptr<ResourceInfo>> m_resources;

    // Resource loaders by type
    std::map<std::type_index, std::any> m_loaders;

    // Handle counter
    ResourceHandleID m_nextHandle = 1;

    // Thread safety
    mutable std::mutex m_mutex;

public:
    ResourceManager() = default;
    ~ResourceManager() {
        clear();
    }

    // Non-copyable
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    /**
     * @brief Register a resource loader for a specific type
     * @tparam T Resource type
     * @param loader Function to load resources of type T
     */
    template<typename T>
    void registerLoader(ResourceLoader<T> loader) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_loaders[std::type_index(typeid(T))] = loader;
    }

    /**
     * @brief Load or get a cached resource
     * @tparam T Resource type
     * @param path Path or identifier for the resource
     * @return Shared pointer to the resource
     */
    template<typename T>
    std::shared_ptr<T> load(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Check if already loaded
        auto it = m_resources.find(path);
        if (it != m_resources.end()) {
            auto& info = it->second;
            info->referenceCount++;

            try {
                return std::any_cast<std::shared_ptr<T>>(info->resource);
            } catch (const std::bad_any_cast&) {
                // Type mismatch, remove old resource and reload
                m_resources.erase(it);
            }
        }

        // Load new resource
        auto typeIdx = std::type_index(typeid(T));
        auto loaderIt = m_loaders.find(typeIdx);

        if (loaderIt == m_loaders.end()) {
            throw std::runtime_error(
                "No loader registered for type: " + std::string(typeid(T).name())
            );
        }

        // Get the loader and load the resource
        auto loader = std::any_cast<ResourceLoader<T>>(loaderIt->second);
        auto resource = loader(path);

        if (!resource) {
            throw std::runtime_error("Failed to load resource: " + path);
        }

        // Store in cache
        auto info = std::make_shared<ResourceInfo>(resource, path, typeIdx);
        info->referenceCount = 1;
        m_resources[path] = info;

        return resource;
    }

    /**
     * @brief Get a resource without loading (must be already loaded)
     * @tparam T Resource type
     * @param path Resource path
     * @return Shared pointer to resource or nullptr if not found
     */
    template<typename T>
    std::shared_ptr<T> get(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_resources.find(path);
        if (it == m_resources.end()) {
            return nullptr;
        }

        try {
            return std::any_cast<std::shared_ptr<T>>(it->second->resource);
        } catch (const std::bad_any_cast&) {
            return nullptr;
        }
    }

    /**
     * @brief Add a pre-loaded resource to the manager
     *
     * Adds an already-constructed resource to the cache without using a loader.
     * The resource will have an initial reference count of 1 and be marked as cached.
     *
     * @tparam T Resource type
     * @param path Resource identifier (unique key in cache)
     * @param resource The resource to add (must not be nullptr)
     */
    template<typename T>
    void add(const std::string& path, std::shared_ptr<T> resource) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto info = std::make_shared<ResourceInfo>(
            resource,
            path,
            std::type_index(typeid(T))
        );
        info->referenceCount = 1;
        m_resources[path] = info;
    }

    /**
     * @brief Add a pre-loaded resource with plugin tracking
     *
     * Similar to add(), but associates the resource with a specific plugin.
     * This allows bulk cleanup via unloadPlugin() when the plugin is unloaded.
     * The resource will have an initial reference count of 1 and be marked as cached.
     *
     * @tparam T Resource type
     * @param path Resource identifier (unique key in cache)
     * @param resource The resource to add (must not be nullptr)
     * @param pluginId Plugin identifier for ownership tracking and cleanup
     */
    template<typename T>
    void addWithPlugin(const std::string& path, std::shared_ptr<T> resource, const std::string& pluginId) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto info = std::make_shared<ResourceInfo>(
            resource,
            path,
            std::type_index(typeid(T)),
            pluginId
        );
        info->referenceCount = 1;
        m_resources[path] = info;
    }

    /**
     * @brief Release a resource reference
     * @param path Resource path
     * @return true if resource was removed from cache
     */
    bool release(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_resources.find(path);
        if (it == m_resources.end()) {
            return false;
        }

        auto& info = it->second;
        if (info->referenceCount > 0) {
            info->referenceCount--;
        }

        // Remove if no more references and not marked for caching
        if (info->referenceCount == 0 && !info->cached) {
            m_resources.erase(it);
            return true;
        }

        return false;
    }

    /**
     * @brief Unload a specific resource
     *
     * Immediately removes the resource from cache regardless of reference count.
     * This is a forceful unload - existing shared_ptr references will remain valid
     * but the resource is removed from the manager's tracking.
     *
     * @param path Resource path to unload
     */
    void unload(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_resources.erase(path);
    }

    /**
     * @brief Unload all resources loaded by a specific plugin
     * @param pluginId Plugin identifier
     * @return Number of resources unloaded
     */
    size_t unloadPlugin(const std::string& pluginId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t count = 0;

        for (auto it = m_resources.begin(); it != m_resources.end(); ) {
            if (it->second->pluginId == pluginId) {
                it = m_resources.erase(it);
                count++;
            } else {
                ++it;
            }
        }

        return count;
    }

    /**
     * @brief Check if a resource is loaded
     *
     * Returns true if the resource exists in the cache, regardless of its
     * reference count or type.
     *
     * @param path Resource path to check
     * @return true if resource is in cache, false otherwise
     */
    bool isLoaded(const std::string& path) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_resources.find(path) != m_resources.end();
    }

    /**
     * @brief Get reference count for a resource
     *
     * Returns the current number of active references to the resource.
     * Returns 0 if the resource is not loaded.
     *
     * @param path Resource path to query
     * @return Current reference count, or 0 if resource not found
     */
    size_t getReferenceCount(const std::string& path) const {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_resources.find(path);
        return (it != m_resources.end()) ? it->second->referenceCount : 0;
    }

    /**
     * @brief Set whether a resource should be cached
     * @param path Resource path
     * @param cached true to keep in cache even with 0 references
     */
    void setCached(const std::string& path, bool cached) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_resources.find(path);
        if (it != m_resources.end()) {
            it->second->cached = cached;
        }
    }

    /**
     * @brief Clear all unreferenced resources
     */
    void clearUnreferenced() {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (auto it = m_resources.begin(); it != m_resources.end();) {
            if (it->second->referenceCount == 0 && !it->second->cached) {
                it = m_resources.erase(it);
            } else {
                ++it;
            }
        }
    }

    /**
     * @brief Clear all resources
     */
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_resources.clear();
    }

    /**
     * @brief Get number of loaded resources
     */
    size_t getResourceCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_resources.size();
    }

    /**
     * @brief Get all loaded resource paths
     */
    std::vector<std::string> getLoadedResources() const {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::vector<std::string> paths;
        paths.reserve(m_resources.size());

        for (const auto& [path, info] : m_resources) {
            paths.push_back(path);
        }

        return paths;
    }
};

/**
 * @brief RAII wrapper for automatic resource release
 *
 * Provides automatic reference counting for resources managed by ResourceManager.
 * When the handle is destroyed, it automatically calls ResourceManager::release()
 * to decrement the reference count. Supports move semantics but not copying.
 *
 * @tparam T Type of the resource being managed
 */
template<typename T>
class ResourceHandle {
private:
    std::shared_ptr<T> m_resource;  ///< Shared pointer to the actual resource
    std::string m_path;              ///< Resource path for release tracking
    ResourceManager* m_manager;      ///< Pointer to managing ResourceManager

public:
    /**
     * @brief Construct a ResourceHandle
     *
     * Creates a handle that will automatically release the resource when destroyed.
     * The resource's reference count should already be incremented by the manager.
     *
     * @param resource Shared pointer to the resource
     * @param path Resource path identifier
     * @param manager Pointer to the ResourceManager that owns this resource
     */
    ResourceHandle(std::shared_ptr<T> resource,
                  const std::string& path,
                  ResourceManager* manager)
        : m_resource(std::move(resource))
        , m_path(path)
        , m_manager(manager) {}

    /**
     * @brief Destructor - automatically releases the resource
     *
     * Calls ResourceManager::release() to decrement the reference count.
     * If the count reaches zero and the resource is not cached, it will be unloaded.
     */
    ~ResourceHandle() {
        if (m_manager) {
            m_manager->release(m_path);
        }
    }

    /**
     * @brief Move constructor
     *
     * Transfers ownership of the resource from another handle.
     * The source handle is left in a valid but empty state (nullptr manager).
     *
     * @param other Source handle to move from
     */
    ResourceHandle(ResourceHandle&& other) noexcept
        : m_resource(std::move(other.m_resource))
        , m_path(std::move(other.m_path))
        , m_manager(other.m_manager) {
        other.m_manager = nullptr;
    }

    /**
     * @brief Move assignment operator
     *
     * Releases the current resource (if any) and transfers ownership from another handle.
     * The source handle is left in a valid but empty state (nullptr manager).
     *
     * @param other Source handle to move from
     * @return Reference to this handle
     */
    ResourceHandle& operator=(ResourceHandle&& other) noexcept {
        if (this != &other) {
            if (m_manager) {
                m_manager->release(m_path);
            }
            m_resource = std::move(other.m_resource);
            m_path = std::move(other.m_path);
            m_manager = other.m_manager;
            other.m_manager = nullptr;
        }
        return *this;
    }

    // Non-copyable
    ResourceHandle(const ResourceHandle&) = delete;
    ResourceHandle& operator=(const ResourceHandle&) = delete;

    /**
     * @brief Arrow operator for member access
     * @return Pointer to the resource
     */
    T* operator->() { return m_resource.get(); }

    /**
     * @brief Const arrow operator for member access
     * @return Const pointer to the resource
     */
    const T* operator->() const { return m_resource.get(); }

    /**
     * @brief Dereference operator
     * @return Reference to the resource
     */
    T& operator*() { return *m_resource; }

    /**
     * @brief Const dereference operator
     * @return Const reference to the resource
     */
    const T& operator*() const { return *m_resource; }

    /**
     * @brief Get raw pointer to the resource
     * @return Pointer to the resource
     */
    T* get() { return m_resource.get(); }

    /**
     * @brief Get const raw pointer to the resource
     * @return Const pointer to the resource
     */
    const T* get() const { return m_resource.get(); }

    /**
     * @brief Boolean conversion operator
     *
     * Allows using the handle in boolean contexts (e.g., if statements).
     *
     * @return true if the resource pointer is not nullptr, false otherwise
     */
    operator bool() const { return m_resource != nullptr; }
};

} // namespace mcf

#pragma once

#include "PluginMetadata.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace mcf {

/**
 * @brief Exception thrown when dependency resolution fails
 */
class DependencyException : public std::runtime_error {
public:
    /**
     * @brief Construct a new Dependency Exception object
     * @param message Error message describing the dependency failure
     */
    explicit DependencyException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief Node in the dependency graph
 */
struct DependencyNode {
    /** @brief Name of the plugin */
    std::string name;

    /** @brief Version string of the plugin */
    std::string version;

    /** @brief Forward dependencies: plugins that this plugin depends on */
    std::vector<std::string> dependencies;

    /** @brief Reverse dependencies: plugins that depend on this plugin */
    std::vector<std::string> dependents;

    /** @brief Load priority (higher values loaded first among peers) */
    int priority = 100;

    /** @brief Flag indicating if this node has been visited during graph traversal */
    bool visited = false;

    /** @brief Flag indicating if this node is currently in the DFS call stack (used for cycle detection) */
    bool inStack = false;

    DependencyNode() = default;

    /**
     * @brief Construct a new Dependency Node object
     * @param n Plugin name
     * @param v Plugin version
     * @param p Load priority (default 100)
     */
    DependencyNode(const std::string& n, const std::string& v, int p = 100)
        : name(n), version(v), priority(p) {}
};

/**
 * @brief Dependency resolver using topological sort
 *
 * Resolves plugin dependencies and determines the correct loading order.
 * Detects circular dependencies and validates version constraints.
 */
class DependencyResolver {
private:
    std::map<std::string, DependencyNode> m_nodes;

public:
    /**
     * @brief Add a plugin to the dependency graph
     * @param metadata Plugin metadata containing name, version, and dependencies
     */
    void addPlugin(const PluginMetadata& metadata) {
        DependencyNode node(metadata.name, metadata.version, metadata.loadPriority);

        // Extract dependency names
        for (const auto& dep : metadata.dependencies) {
            if (dep.required) {
                node.dependencies.push_back(dep.pluginName);
            }
        }

        m_nodes[metadata.name] = node;

        // Build reverse dependencies
        rebuildReverseDependencies();
    }

    /**
     * @brief Resolve dependencies and return load order
     * @return Vector of plugin names in load order (dependencies first)
     * @throws DependencyException if resolution fails
     */
    std::vector<std::string> resolve() {
        // Check for missing dependencies
        validateDependencies();

        // Perform topological sort
        std::vector<std::string> order;
        std::set<std::string> visited;

        // Reset visited flags
        for (auto& [name, node] : m_nodes) {
            node.visited = false;
            node.inStack = false;
        }

        // Visit each node
        for (auto& [name, node] : m_nodes) {
            if (!node.visited) {
                topologicalSort(name, visited, order);
            }
        }

        // Apply priority sorting for nodes at the same level
        // Higher priority plugins should be loaded first among peers
        std::stable_sort(order.begin(), order.end(),
            [this](const std::string& a, const std::string& b) {
                auto it_a = m_nodes.find(a);
                auto it_b = m_nodes.find(b);
                if (it_a == m_nodes.end() || it_b == m_nodes.end()) {
                    return false;
                }
                // Higher priority = loaded first
                return it_a->second.priority > it_b->second.priority;
            }
        );

        return order;
    }

    /**
     * @brief Validate plugin metadata constraints
     * @param metadata Plugin metadata to validate
     * @throws DependencyException if validation fails
     */
    void validateMetadata(const PluginMetadata& metadata) {
        // Check for self-dependency
        if (metadata.dependsOn(metadata.name)) {
            throw DependencyException(
                "Plugin '" + metadata.name + "' depends on itself"
            );
        }

        // Validate version constraints of dependencies
        for (const auto& dep : metadata.dependencies) {
            auto it = m_nodes.find(dep.pluginName);
            if (it != m_nodes.end()) {
                const auto& node = it->second;

                if (!VersionUtils::satisfiesConstraint(
                    node.version, dep.minVersion, dep.maxVersion)) {
                    throw DependencyException(
                        "Plugin '" + metadata.name +
                        "' requires '" + dep.pluginName +
                        "' version between " + dep.minVersion +
                        " and " + dep.maxVersion +
                        ", but found version " + node.version
                    );
                }
            } else if (dep.required) {
                throw DependencyException(
                    "Plugin '" + metadata.name +
                    "' requires missing dependency: " + dep.pluginName
                );
            }
        }
    }

    /**
     * @brief Check if a plugin exists in the graph
     * @param name Plugin name to check
     * @return true if the plugin exists in the graph, false otherwise
     */
    bool hasPlugin(const std::string& name) const {
        return m_nodes.find(name) != m_nodes.end();
    }

    /**
     * @brief Get plugin metadata
     * @param name Plugin name to retrieve
     * @return Pointer to the DependencyNode if found, nullptr otherwise
     */
    const DependencyNode* getNode(const std::string& name) const {
        auto it = m_nodes.find(name);
        return (it != m_nodes.end()) ? &it->second : nullptr;
    }

    /**
     * @brief Remove a plugin from the graph
     * @param name Plugin name to remove
     */
    void removePlugin(const std::string& name) {
        m_nodes.erase(name);
        rebuildReverseDependencies();
    }

    /**
     * @brief Get all plugins that depend on the given plugin
     * @param name Plugin name
     * @return Vector of plugin names that depend on this plugin
     */
    std::vector<std::string> getDependents(const std::string& name) const {
        auto it = m_nodes.find(name);
        if (it != m_nodes.end()) {
            return it->second.dependents;
        }
        return {};
    }

    /**
     * @brief Get all plugins that this plugin depends on
     * @param name Plugin name
     * @return Vector of plugin names this plugin depends on
     */
    std::vector<std::string> getDependencies(const std::string& name) const {
        auto it = m_nodes.find(name);
        if (it != m_nodes.end()) {
            return it->second.dependencies;
        }
        return {};
    }

    /**
     * @brief Clear all plugins
     */
    void clear() {
        m_nodes.clear();
    }

    /**
     * @brief Get number of plugins in graph
     * @return Number of plugins currently in the dependency graph
     */
    size_t pluginCount() const {
        return m_nodes.size();
    }

    /**
     * @brief Get all plugin names
     * @return Vector containing names of all plugins in the graph
     */
    std::vector<std::string> getPluginNames() const {
        std::vector<std::string> names;
        names.reserve(m_nodes.size());
        for (const auto& [name, node] : m_nodes) {
            names.push_back(name);
        }
        return names;
    }

private:
    /**
     * @brief Rebuild reverse dependency mappings
     *
     * Updates the 'dependents' field for all nodes based on the
     * 'dependencies' field. Called after adding or removing plugins.
     */
    void rebuildReverseDependencies() {
        // Clear all dependents
        for (auto& [name, node] : m_nodes) {
            node.dependents.clear();
        }

        // Rebuild dependents from dependencies
        for (const auto& [name, node] : m_nodes) {
            for (const auto& dep : node.dependencies) {
                auto it = m_nodes.find(dep);
                if (it != m_nodes.end()) {
                    it->second.dependents.push_back(name);
                }
            }
        }
    }

    /**
     * @brief Validate that all dependencies exist
     * @throws DependencyException if a required dependency is missing
     */
    void validateDependencies() {
        for (const auto& [name, node] : m_nodes) {
            for (const auto& dep : node.dependencies) {
                if (m_nodes.find(dep) == m_nodes.end()) {
                    throw DependencyException(
                        "Plugin '" + name +
                        "' depends on missing plugin: " + dep
                    );
                }
            }
        }
    }

    /**
     * @brief Topological sort using DFS
     * @param name Current plugin name being visited
     * @param visited Set of already visited plugin names
     * @param order Output vector containing the sorted plugin order
     * @throws DependencyException if circular dependency detected
     */
    void topologicalSort(const std::string& name,
                        std::set<std::string>& visited,
                        std::vector<std::string>& order) {
        auto it = m_nodes.find(name);
        if (it == m_nodes.end()) {
            return;
        }

        DependencyNode& node = it->second;

        // Check for circular dependency
        if (node.inStack) {
            throw DependencyException(
                "Circular dependency detected involving: " + name
            );
        }

        if (node.visited) {
            return;
        }

        node.inStack = true;

        // Visit dependencies first
        for (const auto& dep : node.dependencies) {
            topologicalSort(dep, visited, order);
        }

        node.inStack = false;
        node.visited = true;
        visited.insert(name);
        order.push_back(name);
    }
};

/**
 * @brief Helper function to create a dependency graph from multiple plugins
 * @param plugins Vector of plugin metadata to resolve dependencies for
 * @return Vector of plugin names in the correct load order (dependencies first)
 * @throws DependencyException if dependency resolution fails due to missing dependencies,
 *         circular dependencies, or version constraint violations
 */
inline std::vector<std::string> resolveDependencies(
    const std::vector<PluginMetadata>& plugins) {
    DependencyResolver resolver;

    // Add all plugins
    for (const auto& metadata : plugins) {
        resolver.addPlugin(metadata);
    }

    // Validate each plugin
    for (const auto& metadata : plugins) {
        resolver.validateMetadata(metadata);
    }

    // Resolve and return order
    return resolver.resolve();
}

} // namespace mcf

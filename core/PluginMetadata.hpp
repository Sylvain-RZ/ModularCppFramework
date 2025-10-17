#pragma once

#include <string>
#include <vector>
#include <map>

namespace mcf {

/**
 * @brief Represents a dependency version constraint
 */
struct VersionConstraint {
    std::string pluginName;  ///< Name of the plugin dependency
    std::string minVersion;  ///< Minimum required version (empty = no minimum)
    std::string maxVersion;  ///< Maximum allowed version (empty = no maximum)
    bool required = true;    ///< Whether this dependency is required

    /**
     * @brief Default constructor
     */
    VersionConstraint() = default;

    /**
     * @brief Construct a version constraint
     * @param name Name of the plugin dependency
     * @param min Minimum required version (empty = no minimum)
     * @param max Maximum allowed version (empty = no maximum)
     * @param req Whether this dependency is required
     */
    VersionConstraint(const std::string& name,
                     const std::string& min = "",
                     const std::string& max = "",
                     bool req = true)
        : pluginName(name), minVersion(min), maxVersion(max), required(req) {}
};

/**
 * @brief Plugin metadata containing all information about a plugin
 */
struct PluginMetadata {
    // Basic information
    std::string name;         ///< Plugin name (unique identifier)
    std::string version;      ///< Semantic version string (e.g., "1.2.3")
    std::string author;       ///< Plugin author name
    std::string description;  ///< Brief description of plugin functionality
    std::string license;      ///< License type (e.g., "MIT", "GPL")

    // Dependencies
    std::vector<VersionConstraint> dependencies;  ///< List of plugin dependencies

    // Loading configuration
    int loadPriority = 100;   ///< Load priority (higher = loaded earlier)
    bool autoLoad = true;     ///< Whether to automatically load on startup

    // Entry points
    std::string entryPoint = "createPlugin";    ///< Factory function name
    std::string destroyPoint = "destroyPlugin"; ///< Destructor function name

    // Additional metadata
    std::map<std::string, std::string> customFields;  ///< User-defined key-value pairs

    /**
     * @brief Add a dependency to the plugin
     * @param name Name of the plugin dependency
     * @param minVersion Minimum required version (empty = no minimum)
     * @param maxVersion Maximum allowed version (empty = no maximum)
     * @param required Whether this dependency is required
     */
    void addDependency(const std::string& name,
                      const std::string& minVersion = "",
                      const std::string& maxVersion = "",
                      bool required = true) {
        dependencies.emplace_back(name, minVersion, maxVersion, required);
    }

    /**
     * @brief Check if this plugin depends on another
     * @param pluginName Name of the plugin to check
     * @return true if this plugin depends on the specified plugin, false otherwise
     */
    bool dependsOn(const std::string& pluginName) const {
        for (const auto& dep : dependencies) {
            if (dep.pluginName == pluginName) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Get custom field value
     * @param key The custom field key
     * @param defaultValue Value to return if key is not found
     * @return The custom field value or defaultValue if not found
     */
    std::string getCustomField(const std::string& key,
                              const std::string& defaultValue = "") const {
        auto it = customFields.find(key);
        return (it != customFields.end()) ? it->second : defaultValue;
    }

    /**
     * @brief Set custom field value
     * @param key The custom field key
     * @param value The value to set
     */
    void setCustomField(const std::string& key, const std::string& value) {
        customFields[key] = value;
    }
};

/**
 * @brief Version comparison utilities
 */
class VersionUtils {
public:
    /**
     * @brief Parse semantic version string (e.g., "1.2.3")
     * @param version Semantic version string to parse
     * @return Vector of version components [major, minor, patch]
     */
    static std::vector<int> parseVersion(const std::string& version) {
        std::vector<int> parts;
        size_t start = 0;
        size_t end = version.find('.');

        while (end != std::string::npos) {
            parts.push_back(std::stoi(version.substr(start, end - start)));
            start = end + 1;
            end = version.find('.', start);
        }

        if (start < version.length()) {
            parts.push_back(std::stoi(version.substr(start)));
        }

        // Ensure we have at least 3 components
        while (parts.size() < 3) {
            parts.push_back(0);
        }

        return parts;
    }

    /**
     * @brief Compare two version strings
     * @param v1 First version string
     * @param v2 Second version string
     * @return -1 if v1 < v2, 0 if equal, 1 if v1 > v2
     */
    static int compareVersions(const std::string& v1, const std::string& v2) {
        auto parts1 = parseVersion(v1);
        auto parts2 = parseVersion(v2);

        for (size_t i = 0; i < std::min(parts1.size(), parts2.size()); ++i) {
            if (parts1[i] < parts2[i]) return -1;
            if (parts1[i] > parts2[i]) return 1;
        }

        return 0;
    }

    /**
     * @brief Check if version satisfies constraint
     * @param version Version string to check
     * @param minVersion Minimum required version (empty = no minimum)
     * @param maxVersion Maximum allowed version (empty = no maximum)
     * @return true if version satisfies the constraint, false otherwise
     */
    static bool satisfiesConstraint(const std::string& version,
                                    const std::string& minVersion,
                                    const std::string& maxVersion) {
        if (!minVersion.empty() && compareVersions(version, minVersion) < 0) {
            return false;
        }
        if (!maxVersion.empty() && compareVersions(version, maxVersion) > 0) {
            return false;
        }
        return true;
    }
};

} // namespace mcf

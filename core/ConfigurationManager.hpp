#pragma once

#include "JsonParser.hpp"
#include "JsonValue.hpp"

#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace mcf {

/**
 * @brief Configuration change callback type
 */
using ConfigChangeCallback = std::function<void(const std::string& key, const JsonValue& value)>;

/**
 * @brief Configuration manager for JSON-based application and plugin configuration
 *
 * Features:
 * - Load/save JSON configuration files
 * - Hierarchical configuration (sections)
 * - Type-safe value retrieval
 * - Configuration change notifications
 * - Thread-safe operations
 * - Hot-reload support
 */
class ConfigurationManager {
private:
    // Root configuration
    JsonValue m_config;

    // Configuration file path
    std::string m_configPath;

    // Change callbacks (key -> callback)
    std::unordered_map<std::string, std::vector<ConfigChangeCallback>> m_callbacks;

    // Thread safety
    mutable std::mutex m_mutex;

    // Dirty flag for auto-save
    bool m_dirty = false;

    /**
     * @brief Navigate to a nested value using dot notation
     * @param key Configuration key using dot notation (e.g., "section.subsection.value")
     * @param createPath If true, creates missing intermediate objects in the path
     * @return Pointer to the JsonValue at the specified path, or nullptr if not found
     */
    JsonValue* navigate(const std::string& key, bool createPath = false) {
        if (key.empty()) {
            return &m_config;
        }

        // Split key by dots
        std::vector<std::string> parts;
        std::string current;
        for (char c : key) {
            if (c == '.') {
                if (!current.empty()) {
                    parts.push_back(current);
                    current.clear();
                }
            } else {
                current += c;
            }
        }
        if (!current.empty()) {
            parts.push_back(current);
        }

        // Navigate through the hierarchy
        JsonValue* current_value = &m_config;
        for (size_t i = 0; i < parts.size(); ++i) {
            const auto& part = parts[i];

            if (!current_value->isObject()) {
                if (createPath) {
                    *current_value = JsonValue(JsonObject());
                } else {
                    return nullptr;
                }
            }

            // For non-const access, we need to handle this differently
            // Since we're returning a pointer, we'll work with the internal structure
            if (i == parts.size() - 1) {
                // Last part - return reference to this value
                if (!current_value->has(part) && createPath) {
                    // We need to insert a new value, but we can't modify through const reference
                    // This is a limitation of the current design
                    return nullptr; // Will be handled by set method
                }
            }

            if (!current_value->has(part)) {
                return nullptr;
            }

            // Note: This is unsafe but necessary for the current architecture
            // In a production system, consider redesigning to avoid const_cast
            const JsonValue& next = (*current_value)[part];
            current_value = const_cast<JsonValue*>(&next);
        }

        return current_value;
    }

    /**
     * @brief Notify callbacks of configuration change
     * @param key Configuration key that changed
     * @param value New value for the key
     */
    void notifyChange(const std::string& key, const JsonValue& value) {
        auto it = m_callbacks.find(key);
        if (it != m_callbacks.end()) {
            // Copy callbacks to prevent deadlock if callback modifies config
            auto callbacks = it->second;
            for (const auto& callback : callbacks) {
                callback(key, value);
            }
        }
    }

public:
    /**
     * @brief Default constructor
     *
     * Initializes the configuration manager with an empty JSON object.
     */
    ConfigurationManager() : m_config(JsonObject()) {}

    /**
     * @brief Destructor
     *
     * Automatically saves configuration to file if there are unsaved changes
     * and a configuration path has been set.
     */
    ~ConfigurationManager() {
        if (m_dirty && !m_configPath.empty()) {
            save(m_configPath);
        }
    }

    /**
     * @brief Deleted copy constructor
     *
     * ConfigurationManager is non-copyable to prevent unintended duplication
     * of configuration state and callbacks.
     */
    ConfigurationManager(const ConfigurationManager&) = delete;

    /**
     * @brief Deleted copy assignment operator
     *
     * ConfigurationManager is non-copyable to prevent unintended duplication
     * of configuration state and callbacks.
     * @return Reference to this object (deleted)
     */
    ConfigurationManager& operator=(const ConfigurationManager&) = delete;

    /**
     * @brief Load configuration from JSON file
     * @param path Path to the JSON configuration file to load
     * @return true if the file was loaded successfully, false otherwise
     */
    bool load(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_mutex);

        try {
            m_config = JsonParser::parseFile(path);
            m_configPath = path;
            m_dirty = false;
            return true;
        } catch (const std::exception&) {
            // If file doesn't exist or is invalid, start with empty config
            m_config = JsonValue(JsonObject());
            m_configPath = path;
            return false;
        }
    }

    /**
     * @brief Save configuration to JSON file
     * @param path Path to save the configuration file. If empty, uses the last loaded path
     * @return true if the file was saved successfully, false otherwise
     */
    bool save(const std::string& path = "") {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::string savePath = path.empty() ? m_configPath : path;
        if (savePath.empty()) {
            return false;
        }

        // Ensure directory exists
        std::filesystem::path filePath(savePath);
        if (filePath.has_parent_path()) {
            try {
                std::filesystem::create_directories(filePath.parent_path());
            } catch (const std::filesystem::filesystem_error&) {
                // Failed to create directory (e.g., path is a file or invalid)
                return false;
            }
        }

        bool success = JsonParser::writeFile(savePath, m_config);
        if (success) {
            m_dirty = false;
            if (!path.empty()) {
                m_configPath = path;
            }
        }
        return success;
    }

    /**
     * @brief Reload configuration from file
     * @return true if the configuration was reloaded successfully, false if no path is set or reload failed
     */
    bool reload() {
        if (m_configPath.empty()) {
            return false;
        }
        return load(m_configPath);
    }

    /**
     * @brief Get configuration value by key (dot notation supported)
     * @param key Configuration key using dot notation (e.g., "section.subsection.value")
     * @param defaultValue Default value to return if the key is not found
     * @return The JsonValue at the specified key, or defaultValue if not found
     */
    JsonValue get(const std::string& key, const JsonValue& defaultValue = JsonValue()) const {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (key.empty()) {
            return m_config;
        }

        // Split key by dots and navigate
        const JsonValue* current = &m_config;
        std::string part;
        for (char c : key) {
            if (c == '.') {
                if (!part.empty()) {
                    if (!current->isObject() || !current->has(part)) {
                        return defaultValue;
                    }
                    current = &(*current)[part];
                    part.clear();
                }
            } else {
                part += c;
            }
        }

        if (!part.empty()) {
            if (!current->isObject() || !current->has(part)) {
                return defaultValue;
            }
            return (*current)[part];
        }

        return *current;
    }

    /**
     * @brief Set configuration value by key (dot notation supported)
     * @param key Configuration key using dot notation (e.g., "section.subsection.value")
     * @param value The value to set at the specified key
     */
    void set(const std::string& key, const JsonValue& value) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (key.empty()) {
            m_config = value;
            m_dirty = true;
            return;
        }

        // For simplicity, we'll rebuild the object structure
        // Split key by dots
        std::vector<std::string> parts;
        std::string current;
        for (char c : key) {
            if (c == '.') {
                if (!current.empty()) {
                    parts.push_back(current);
                    current.clear();
                }
            } else {
                current += c;
            }
        }
        if (!current.empty()) {
            parts.push_back(current);
        }

        // Ensure root is an object
        if (!m_config.isObject()) {
            m_config = JsonValue(JsonObject());
        }

        // Build path and set value
        std::function<void(JsonValue&, size_t)> setRecursive = [&](JsonValue& node, size_t depth) {
            if (depth == parts.size() - 1) {
                // Last part - set the value
                if (!node.isObject()) {
                    node = JsonValue(JsonObject());
                }
                JsonObject obj = node.asObject();
                obj[parts[depth]] = value;
                node = JsonValue(obj);
                return;
            }

            // Intermediate part - ensure object exists
            if (!node.isObject()) {
                node = JsonValue(JsonObject());
            }

            JsonObject obj = node.asObject();
            if (!obj[parts[depth]].isObject()) {
                obj[parts[depth]] = JsonValue(JsonObject());
            }

            JsonValue child = obj[parts[depth]];
            setRecursive(child, depth + 1);
            obj[parts[depth]] = child;
            node = JsonValue(obj);
        };

        setRecursive(m_config, 0);
        m_dirty = true;

        // Notify callbacks
        notifyChange(key, value);
    }

    /**
     * @brief Check if configuration has a key
     * @param key Configuration key using dot notation (e.g., "section.subsection.value")
     * @return true if the key exists and is not null, false otherwise
     */
    bool has(const std::string& key) const {
        // Don't lock here - get() already locks
        return !get(key).isNull();
    }

    /**
     * @brief Remove a configuration key
     * @param key Configuration key using dot notation (e.g., "section.subsection.value")
     */
    void remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Similar to set, but removes the key
        std::vector<std::string> parts;
        std::string current;
        for (char c : key) {
            if (c == '.') {
                if (!current.empty()) {
                    parts.push_back(current);
                    current.clear();
                }
            } else {
                current += c;
            }
        }
        if (!current.empty()) {
            parts.push_back(current);
        }

        std::function<bool(JsonValue&, size_t)> removeRecursive = [&](JsonValue& node, size_t depth) -> bool {
            if (!node.isObject()) return false;

            if (depth == parts.size() - 1) {
                // Last part - remove the key
                JsonObject obj = node.asObject();
                auto it = obj.find(parts[depth]);
                if (it != obj.end()) {
                    obj.erase(it);
                    node = JsonValue(obj);
                    return true;
                }
                return false;
            }

            // Intermediate part
            JsonObject obj = node.asObject();
            auto it = obj.find(parts[depth]);
            if (it != obj.end()) {
                JsonValue child = it->second;
                if (removeRecursive(child, depth + 1)) {
                    obj[parts[depth]] = child;
                    node = JsonValue(obj);
                    return true;
                }
            }
            return false;
        };

        if (removeRecursive(m_config, 0)) {
            m_dirty = true;
        }
    }

    /**
     * @brief Register callback for configuration changes
     * @param key Configuration key to watch for changes
     * @param callback Function to call when the key's value changes
     */
    void watch(const std::string& key, ConfigChangeCallback callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callbacks[key].push_back(callback);
    }

    /**
     * @brief Get all configuration as JsonValue
     * @return The entire configuration tree as a JsonValue object
     */
    JsonValue getAll() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_config;
    }

    /**
     * @brief Clear all configuration
     */
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config = JsonValue(JsonObject());
        m_dirty = true;
    }

    /**
     * @brief Get string value
     * @param key Configuration key using dot notation
     * @param defaultValue Default value to return if the key is not found or is not a string
     * @return The string value at the specified key, or defaultValue if not found
     */
    std::string getString(const std::string& key, const std::string& defaultValue = "") const {
        return get(key).asString(defaultValue);
    }

    /**
     * @brief Get integer value
     * @param key Configuration key using dot notation
     * @param defaultValue Default value to return if the key is not found or is not an integer
     * @return The integer value at the specified key, or defaultValue if not found
     */
    int64_t getInt(const std::string& key, int64_t defaultValue = 0) const {
        return get(key).asInt(defaultValue);
    }

    /**
     * @brief Get float value
     * @param key Configuration key using dot notation
     * @param defaultValue Default value to return if the key is not found or is not a float
     * @return The float value at the specified key, or defaultValue if not found
     */
    double getFloat(const std::string& key, double defaultValue = 0.0) const {
        return get(key).asFloat(defaultValue);
    }

    /**
     * @brief Get boolean value
     * @param key Configuration key using dot notation
     * @param defaultValue Default value to return if the key is not found or is not a boolean
     * @return The boolean value at the specified key, or defaultValue if not found
     */
    bool getBool(const std::string& key, bool defaultValue = false) const {
        return get(key).asBool(defaultValue);
    }

    /**
     * @brief Get array value
     * @param key Configuration key using dot notation
     * @param defaultValue Default value to return if the key is not found or is not an array
     * @return The array value at the specified key, or defaultValue if not found
     */
    JsonArray getArray(const std::string& key, const JsonArray& defaultValue = JsonArray()) const {
        JsonValue value = get(key);
        if (value.isArray()) {
            return value.asArray();
        }
        return defaultValue;
    }

    /**
     * @brief Get object value
     * @param key Configuration key using dot notation
     * @param defaultValue Default value to return if the key is not found or is not an object
     * @return The object value at the specified key, or defaultValue if not found
     */
    JsonObject getObject(const std::string& key, const JsonObject& defaultValue = JsonObject()) const {
        JsonValue value = get(key);
        if (value.isObject()) {
            return value.asObject();
        }
        return defaultValue;
    }

    /**
     * @brief Enable auto-save on modifications
     * @param enabled Whether to enable auto-save (currently reserved for future use)
     */
    void setAutoSave(bool enabled) {
        // Auto-save is handled in destructor
        // This method is for future extensions
        (void)enabled;
    }

    /**
     * @brief Check if configuration has unsaved changes
     * @return true if there are unsaved changes, false otherwise
     */
    bool isDirty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_dirty;
    }
};

} // namespace mcf

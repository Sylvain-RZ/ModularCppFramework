#pragma once

#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace mcf {

/**
 * @brief JSON value types
 */
enum class JsonType {
    Null,
    Boolean,
    Integer,
    Float,
    String,
    Array,
    Object
};

// Forward declaration
class JsonValue;

/**
 * @brief JSON array type
 */
using JsonArray = std::vector<JsonValue>;

/**
 * @brief JSON object type
 */
using JsonObject = std::map<std::string, JsonValue>;

/**
 * @brief Variant type for JSON value storage
 */
using JsonVariant = std::variant<
    std::nullptr_t,
    bool,
    int64_t,
    double,
    std::string,
    std::shared_ptr<JsonArray>,
    std::shared_ptr<JsonObject>>;

/**
 * @brief JSON value class representing any JSON data type
 */
class JsonValue {
private:
    JsonVariant m_value;

public:
    /**
     * @brief Default constructor - creates a null JSON value
     */
    JsonValue() : m_value(nullptr) {}

    /**
     * @brief Construct a null JSON value
     * @param nullptr_t Null pointer value
     */
    JsonValue(std::nullptr_t) : m_value(nullptr) {}

    /**
     * @brief Construct a boolean JSON value
     * @param value Boolean value to store
     */
    JsonValue(bool value) : m_value(value) {}

    /**
     * @brief Construct an integer JSON value from int
     * @param value Integer value to store
     */
    JsonValue(int value) : m_value(static_cast<int64_t>(value)) {}

    /**
     * @brief Construct an integer JSON value from int64_t
     * @param value 64-bit integer value to store
     */
    JsonValue(int64_t value) : m_value(value) {}

    /**
     * @brief Construct a float JSON value
     * @param value Double precision floating point value to store
     */
    JsonValue(double value) : m_value(value) {}

    /**
     * @brief Construct a string JSON value from std::string
     * @param value String value to store
     */
    JsonValue(const std::string& value) : m_value(value) {}

    /**
     * @brief Construct a string JSON value from C-string
     * @param value C-style string to store
     */
    JsonValue(const char* value) : m_value(std::string(value)) {}

    /**
     * @brief Construct an array JSON value
     * @param value JSON array to store
     */
    JsonValue(const JsonArray& value) : m_value(std::make_shared<JsonArray>(value)) {}

    /**
     * @brief Construct an object JSON value
     * @param value JSON object to store
     */
    JsonValue(const JsonObject& value) : m_value(std::make_shared<JsonObject>(value)) {}

    /**
     * @brief Get the type of this JSON value
     * @return The JsonType of this value
     */
    JsonType type() const {
        return static_cast<JsonType>(m_value.index());
    }

    /**
     * @brief Check if this value is null
     * @return true if this value is null, false otherwise
     */
    bool isNull() const { return type() == JsonType::Null; }

    /**
     * @brief Check if this value is a boolean
     * @return true if this value is a boolean, false otherwise
     */
    bool isBool() const { return type() == JsonType::Boolean; }

    /**
     * @brief Check if this value is an integer
     * @return true if this value is an integer, false otherwise
     */
    bool isInt() const { return type() == JsonType::Integer; }

    /**
     * @brief Check if this value is a float
     * @return true if this value is a float, false otherwise
     */
    bool isFloat() const { return type() == JsonType::Float; }

    /**
     * @brief Check if this value is a number (int or float)
     * @return true if this value is numeric, false otherwise
     */
    bool isNumber() const { return isInt() || isFloat(); }

    /**
     * @brief Check if this value is a string
     * @return true if this value is a string, false otherwise
     */
    bool isString() const { return type() == JsonType::String; }

    /**
     * @brief Check if this value is an array
     * @return true if this value is an array, false otherwise
     */
    bool isArray() const { return type() == JsonType::Array; }

    /**
     * @brief Check if this value is an object
     * @return true if this value is an object, false otherwise
     */
    bool isObject() const { return type() == JsonType::Object; }

    /**
     * @brief Get as boolean
     * @param defaultValue Value to return if this is not a boolean
     * @return The boolean value or defaultValue if not a boolean
     */
    bool asBool(bool defaultValue = false) const {
        if (isBool()) return std::get<bool>(m_value);
        return defaultValue;
    }

    /**
     * @brief Get as integer
     * @param defaultValue Value to return if this is not a number
     * @return The integer value (with conversion from float if needed) or defaultValue
     */
    int64_t asInt(int64_t defaultValue = 0) const {
        if (isInt()) return std::get<int64_t>(m_value);
        if (isFloat()) return static_cast<int64_t>(std::get<double>(m_value));
        return defaultValue;
    }

    /**
     * @brief Get as float
     * @param defaultValue Value to return if this is not a number
     * @return The float value (with conversion from int if needed) or defaultValue
     */
    double asFloat(double defaultValue = 0.0) const {
        if (isFloat()) return std::get<double>(m_value);
        if (isInt()) return static_cast<double>(std::get<int64_t>(m_value));
        return defaultValue;
    }

    /**
     * @brief Get as string
     * @param defaultValue Value to return if this is not a string
     * @return The string value or defaultValue if not a string
     */
    std::string asString(const std::string& defaultValue = "") const {
        if (isString()) return std::get<std::string>(m_value);
        return defaultValue;
    }

    /**
     * @brief Get as array
     * @return Reference to the underlying JsonArray
     * @throws std::runtime_error if this value is not an array
     */
    const JsonArray& asArray() const {
        if (!isArray()) {
            throw std::runtime_error("JsonValue is not an array");
        }
        return *std::get<std::shared_ptr<JsonArray>>(m_value);
    }

    /**
     * @brief Get as object
     * @return Reference to the underlying JsonObject
     * @throws std::runtime_error if this value is not an object
     */
    const JsonObject& asObject() const {
        if (!isObject()) {
            throw std::runtime_error("JsonValue is not an object");
        }
        return *std::get<std::shared_ptr<JsonObject>>(m_value);
    }

    /**
     * @brief Get array or object size
     * @return Number of elements in array or object, 0 for other types
     */
    size_t size() const {
        if (isArray()) return asArray().size();
        if (isObject()) return asObject().size();
        return 0;
    }

    /**
     * @brief Check if object has a key
     * @param key The key to check for
     * @return true if this is an object and contains the key, false otherwise
     */
    bool has(const std::string& key) const {
        if (!isObject()) return false;
        return asObject().find(key) != asObject().end();
    }

    /**
     * @brief Get value by key (for objects)
     * @param key The key to look up
     * @return The value associated with the key, or a null JsonValue if not found
     */
    const JsonValue& operator[](const std::string& key) const {
        if (!isObject()) {
            static JsonValue null_value;
            return null_value;
        }
        const auto& obj = asObject();
        auto it = obj.find(key);
        if (it != obj.end()) {
            return it->second;
        }
        static JsonValue null_value;
        return null_value;
    }

    /**
     * @brief Get value by index (for arrays)
     * @param index The array index to access
     * @return The value at the specified index, or a null JsonValue if out of bounds
     */
    const JsonValue& operator[](size_t index) const {
        if (!isArray()) {
            static JsonValue null_value;
            return null_value;
        }
        const auto& arr = asArray();
        if (index < arr.size()) {
            return arr[index];
        }
        static JsonValue null_value;
        return null_value;
    }

    /**
     * @brief Get value by key with default
     * @param key The key to look up
     * @param defaultValue Value to return if key is not found
     * @return The value associated with the key, or defaultValue if not found or not an object
     */
    JsonValue get(const std::string& key, const JsonValue& defaultValue = JsonValue()) const {
        if (!isObject()) return defaultValue;
        const auto& obj = asObject();
        auto it = obj.find(key);
        return (it != obj.end()) ? it->second : defaultValue;
    }

    /**
     * @brief Convert to string representation
     * @param indent Indentation level for pretty printing
     * @return JSON string representation of this value
     */
    std::string toString(int indent = 0) const {
        std::ostringstream oss;
        std::string indentStr(indent, ' ');

        switch (type()) {
            case JsonType::Null:
                oss << "null";
                break;
            case JsonType::Boolean:
                oss << (asBool() ? "true" : "false");
                break;
            case JsonType::Integer:
                oss << asInt();
                break;
            case JsonType::Float:
                oss << asFloat();
                break;
            case JsonType::String:
                oss << "\"" << asString() << "\"";
                break;
            case JsonType::Array: {
                const auto& arr = asArray();
                oss << "[";
                for (size_t i = 0; i < arr.size(); ++i) {
                    if (i > 0) oss << ", ";
                    oss << arr[i].toString(indent + 2);
                }
                oss << "]";
                break;
            }
            case JsonType::Object: {
                const auto& obj = asObject();
                oss << "{\n";
                bool first = true;
                for (const auto& [key, value] : obj) {
                    if (!first) oss << ",\n";
                    oss << std::string(indent + 2, ' ') << "\"" << key << "\": "
                        << value.toString(indent + 2);
                    first = false;
                }
                oss << "\n" << indentStr << "}";
                break;
            }
        }
        return oss.str();
    }
};

} // namespace mcf

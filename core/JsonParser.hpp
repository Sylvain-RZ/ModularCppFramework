#pragma once

#include "JsonValue.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace mcf {

/**
 * @brief Simple JSON parser
 *
 * Provides basic JSON parsing capabilities:
 * - Parse from string
 * - Parse from file
 * - Support for all JSON types
 * - Error reporting with position
 */
class JsonParser {
private:
    std::string m_input;
    size_t m_pos = 0;
    int m_line = 1;
    int m_column = 1;

    /**
     * @brief Skip whitespace characters
     */
    void skipWhitespace() {
        while (m_pos < m_input.size()) {
            char c = m_input[m_pos];
            if (c == ' ' || c == '\t' || c == '\r') {
                m_pos++;
                m_column++;
            } else if (c == '\n') {
                m_pos++;
                m_line++;
                m_column = 1;
            } else {
                break;
            }
        }
    }

    /**
     * @brief Peek at current character without consuming
     */
    char peek() const {
        if (m_pos >= m_input.size()) return '\0';
        return m_input[m_pos];
    }

    /**
     * @brief Consume and return current character
     */
    char consume() {
        if (m_pos >= m_input.size()) return '\0';
        char c = m_input[m_pos++];
        m_column++;
        return c;
    }

    /**
     * @brief Expect and consume a specific character
     */
    void expect(char expected) {
        char c = consume();
        if (c != expected) {
            throw std::runtime_error("Expected '" + std::string(1, expected) +
                "' at line " + std::to_string(m_line) +
                ", column " + std::to_string(m_column));
        }
    }

    /**
     * @brief Parse a JSON value
     */
    JsonValue parseValue() {
        skipWhitespace();
        char c = peek();

        if (c == '{') return parseObject();
        if (c == '[') return parseArray();
        if (c == '"') return parseString();
        if (c == 't' || c == 'f') return parseBoolean();
        if (c == 'n') return parseNull();
        if (c == '-' || std::isdigit(c)) return parseNumber();

        throw std::runtime_error("Unexpected character '" + std::string(1, c) +
            "' at line " + std::to_string(m_line) +
            ", column " + std::to_string(m_column));
    }

    /**
     * @brief Parse null value
     */
    JsonValue parseNull() {
        expect('n');
        expect('u');
        expect('l');
        expect('l');
        return JsonValue(nullptr);
    }

    /**
     * @brief Parse boolean value
     */
    JsonValue parseBoolean() {
        if (peek() == 't') {
            expect('t');
            expect('r');
            expect('u');
            expect('e');
            return JsonValue(true);
        } else {
            expect('f');
            expect('a');
            expect('l');
            expect('s');
            expect('e');
            return JsonValue(false);
        }
    }

    /**
     * @brief Parse number (integer or float)
     */
    JsonValue parseNumber() {
        std::string numStr;
        bool isFloat = false;

        // Handle negative sign
        if (peek() == '-') {
            numStr += consume();
        }

        // Parse digits
        while (std::isdigit(peek())) {
            numStr += consume();
        }

        // Check for decimal point
        if (peek() == '.') {
            isFloat = true;
            numStr += consume();
            while (std::isdigit(peek())) {
                numStr += consume();
            }
        }

        // Check for exponent
        if (peek() == 'e' || peek() == 'E') {
            isFloat = true;
            numStr += consume();
            if (peek() == '+' || peek() == '-') {
                numStr += consume();
            }
            while (std::isdigit(peek())) {
                numStr += consume();
            }
        }

        if (isFloat) {
            return JsonValue(std::stod(numStr));
        } else {
            return JsonValue(static_cast<int64_t>(std::stoll(numStr)));
        }
    }

    /**
     * @brief Parse string value
     */
    JsonValue parseString() {
        expect('"');
        std::string str;

        while (peek() != '"' && peek() != '\0') {
            char c = consume();
            if (c == '\\') {
                char escaped = consume();
                switch (escaped) {
                    case '"': str += '"'; break;
                    case '\\': str += '\\'; break;
                    case '/': str += '/'; break;
                    case 'b': str += '\b'; break;
                    case 'f': str += '\f'; break;
                    case 'n': str += '\n'; break;
                    case 'r': str += '\r'; break;
                    case 't': str += '\t'; break;
                    default:
                        throw std::runtime_error("Invalid escape sequence at line " +
                            std::to_string(m_line) + ", column " + std::to_string(m_column));
                }
            } else {
                str += c;
            }
        }

        expect('"');
        return JsonValue(str);
    }

    /**
     * @brief Parse array value
     */
    JsonValue parseArray() {
        expect('[');
        skipWhitespace();

        JsonArray arr;

        if (peek() == ']') {
            consume();
            return JsonValue(arr);
        }

        while (true) {
            arr.push_back(parseValue());
            skipWhitespace();

            if (peek() == ']') {
                consume();
                break;
            }

            expect(',');
            skipWhitespace();
        }

        return JsonValue(arr);
    }

    /**
     * @brief Parse object value
     */
    JsonValue parseObject() {
        expect('{');
        skipWhitespace();

        JsonObject obj;

        if (peek() == '}') {
            consume();
            return JsonValue(obj);
        }

        while (true) {
            skipWhitespace();

            // Parse key
            JsonValue keyValue = parseString();
            std::string key = keyValue.asString();

            skipWhitespace();
            expect(':');
            skipWhitespace();

            // Parse value
            JsonValue value = parseValue();
            obj[key] = value;

            skipWhitespace();

            if (peek() == '}') {
                consume();
                break;
            }

            expect(',');
        }

        return JsonValue(obj);
    }

public:
    /**
     * @brief Parse JSON from string
     * @param json JSON string to parse
     * @return JsonValue containing the parsed JSON data
     * @throws std::runtime_error if parsing fails
     */
    static JsonValue parse(const std::string& json) {
        JsonParser parser;
        parser.m_input = json;
        parser.m_pos = 0;
        parser.m_line = 1;
        parser.m_column = 1;

        try {
            return parser.parseValue();
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("JSON parse error: ") + e.what());
        }
    }

    /**
     * @brief Parse JSON from file
     * @param filename Path to the JSON file to parse
     * @return JsonValue containing the parsed JSON data
     * @throws std::runtime_error if file cannot be opened or parsing fails
     */
    static JsonValue parseFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return parse(buffer.str());
    }

    /**
     * @brief Write JSON to file
     * @param filename Path to the file where JSON will be written
     * @param value JsonValue to write to the file
     * @return true if write succeeded, false if file cannot be opened
     */
    static bool writeFile(const std::string& filename, const JsonValue& value) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        file << value.toString(0);
        return true;
    }
};

} // namespace mcf

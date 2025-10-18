#pragma once

#include <algorithm>
#include <chrono>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace mcf {

/**
 * @brief Log level enumeration
 */
enum class LogLevel {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warning = 3,
    Error = 4,
    Critical = 5,
    Off = 6
};

/**
 * @brief Convert log level to string
 * @param level The log level to convert
 * @return String representation of the log level (e.g., "TRACE", "DEBUG", "INFO")
 */
inline std::string logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:    return "TRACE";
        case LogLevel::Debug:    return "DEBUG";
        case LogLevel::Info:     return "INFO";
        case LogLevel::Warning:  return "WARN";
        case LogLevel::Error:    return "ERROR";
        case LogLevel::Critical: return "CRIT";
        case LogLevel::Off:      return "OFF";
        default:                 return "UNKNOWN";
    }
}

/**
 * @brief Convert string to log level
 * @param str String representation of log level (case-insensitive)
 * @return Corresponding LogLevel enum value, defaults to LogLevel::Info if unknown
 */
inline LogLevel stringToLogLevel(const std::string& str) {
    if (str == "TRACE" || str == "trace") return LogLevel::Trace;
    if (str == "DEBUG" || str == "debug") return LogLevel::Debug;
    if (str == "INFO" || str == "info") return LogLevel::Info;
    if (str == "WARN" || str == "warn" || str == "WARNING" || str == "warning") return LogLevel::Warning;
    if (str == "ERROR" || str == "error") return LogLevel::Error;
    if (str == "CRIT" || str == "crit" || str == "CRITICAL" || str == "critical") return LogLevel::Critical;
    if (str == "OFF" || str == "off") return LogLevel::Off;
    return LogLevel::Info; // Default
}

/**
 * @brief Log message structure
 *
 * Contains all information about a log entry including timestamp,
 * severity level, logger name, message content, and source location.
 */
struct LogMessage {
    std::chrono::system_clock::time_point timestamp; ///< Time when the log message was created
    LogLevel level;                                   ///< Severity level of the log message
    std::string logger_name;                          ///< Name of the logger that created this message
    std::string message;                              ///< The actual log message content
    std::string file;                                 ///< Source file where the log was created
    int line;                                         ///< Line number in the source file
    std::string function;                             ///< Function name where the log was created

    /**
     * @brief Constructs a log message
     * @param lvl Log level/severity
     * @param name Logger name
     * @param msg Message content
     * @param f Source file path (optional)
     * @param l Line number (optional)
     * @param func Function name (optional)
     */
    LogMessage(LogLevel lvl, const std::string& name, const std::string& msg,
               const std::string& f = "", int l = 0, const std::string& func = "")
        : timestamp(std::chrono::system_clock::now())
        , level(lvl)
        , logger_name(name)
        , message(msg)
        , file(f)
        , line(l)
        , function(func) {}
};

/**
 * @brief Log message formatter
 *
 * Formats log messages according to a pattern string with placeholders.
 * Supported placeholders:
 * - %Y-%m-%d %H:%M:%S.%e - Full timestamp with milliseconds
 * - %n - Logger name
 * - %l - Log level
 * - %v - Message content
 * - %s - Source file (basename only)
 * - %# - Line number
 * - %! - Function name
 * - %t - Thread ID
 * - %% - Literal %
 */
class LogFormatter {
private:
    std::string m_pattern;

    /**
     * @brief Format timestamp with milliseconds
     * @param time Time point to format
     * @return Formatted time string in "YYYY-MM-DD HH:MM:SS.mmm" format
     */
    static std::string formatTime(const std::chrono::system_clock::time_point& time) {
        auto time_t = std::chrono::system_clock::to_time_t(time);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()) % 1000;

        std::tm tm_time;
#ifdef _WIN32
        localtime_s(&tm_time, &time_t);
#else
        localtime_r(&time_t, &tm_time);
#endif

        std::ostringstream oss;
        oss << std::put_time(&tm_time, "%Y-%m-%d %H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }

    /**
     * @brief Extract filename from full path
     * @param path Full file path
     * @return Filename without directory path
     */
    static std::string extractFilename(const std::string& path) {
        size_t pos = path.find_last_of("/\\");
        return (pos == std::string::npos) ? path : path.substr(pos + 1);
    }

public:
    /**
     * @brief Constructs a log formatter with specified pattern
     * @param pattern Format pattern string with placeholders (default: "[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v")
     */
    LogFormatter(const std::string& pattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v")
        : m_pattern(pattern) {}

    /**
     * @brief Format a log message according to the pattern
     * @param msg Log message to format
     * @return Formatted string with all placeholders replaced
     */
    std::string format(const LogMessage& msg) const {
        std::string result = m_pattern;

        // Replace placeholders
        size_t pos = 0;
        while ((pos = result.find('%', pos)) != std::string::npos) {
            if (pos + 1 >= result.length()) break;

            char code = result[pos + 1];
            std::string replacement;

            switch (code) {
                case 'Y': // Full year - handled in time format
                case 'm': // Month - handled in time format
                case 'd': // Day - handled in time format
                case 'H': // Hour - handled in time format
                case 'M': // Minute - handled in time format
                case 'S': // Second - handled in time format
                case 'e': // Milliseconds - handled in time format
                    // Replace entire time pattern with formatted time
                    if (pos == result.find("[%Y-%m-%d %H:%M:%S.%e]")) {
                        size_t end = result.find(']', pos);
                        if (end != std::string::npos) {
                            result.replace(pos, end - pos + 1, "[" + formatTime(msg.timestamp) + "]");
                            continue;
                        }
                    }
                    pos++;
                    continue;
                case 'n': // Logger name
                    replacement = msg.logger_name;
                    break;
                case 'l': // Log level
                    replacement = logLevelToString(msg.level);
                    break;
                case 'v': // Message
                    replacement = msg.message;
                    break;
                case 's': // Source file
                    replacement = extractFilename(msg.file);
                    break;
                case '#': // Line number
                    replacement = std::to_string(msg.line);
                    break;
                case '!': // Function name
                    replacement = msg.function;
                    break;
                case 't': // Thread ID
                    replacement = std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
                    break;
                case '%': // Literal %
                    replacement = "%";
                    break;
                default:
                    pos++;
                    continue;
            }

            result.replace(pos, 2, replacement);
            pos += replacement.length();
        }

        return result;
    }

    /**
     * @brief Set a new format pattern
     * @param pattern Format pattern string with placeholders
     */
    void setPattern(const std::string& pattern) {
        m_pattern = pattern;
    }
};

/**
 * @brief Base class for log sinks (output destinations)
 *
 * Abstract base class for implementing different log output targets
 * (console, file, network, etc.). Thread-safe via internal mutex.
 */
class LogSink {
protected:
    LogLevel m_level;        ///< Minimum log level for this sink
    LogFormatter m_formatter; ///< Formatter for log messages
    mutable std::mutex m_mutex; ///< Mutex for thread-safety

public:
    /**
     * @brief Constructs a log sink
     * @param level Minimum log level to output (default: Trace)
     */
    LogSink(LogLevel level = LogLevel::Trace)
        : m_level(level) {}

    /**
     * @brief Virtual destructor
     */
    virtual ~LogSink() = default;

    /**
     * @brief Write a log message (pure virtual)
     * @param msg Log message to write
     */
    virtual void log(const LogMessage& msg) = 0;

    /**
     * @brief Flush buffered log messages
     *
     * Default implementation does nothing. Override in derived classes
     * that buffer output.
     */
    virtual void flush() {}

    /**
     * @brief Set the minimum log level for this sink
     * @param level New minimum log level
     */
    void setLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_level = level;
    }

    /**
     * @brief Get the minimum log level for this sink
     * @return Current minimum log level
     */
    LogLevel getLevel() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_level;
    }

    /**
     * @brief Set the formatter for this sink
     * @param formatter Log formatter to use
     */
    void setFormatter(const LogFormatter& formatter) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_formatter = formatter;
    }

    /**
     * @brief Check if a message should be logged based on level
     * @param level Log level to check
     * @return true if level is >= minimum level, false otherwise
     */
    bool shouldLog(LogLevel level) const {
        return level >= m_level;
    }
};

/**
 * @brief Console sink (stdout/stderr)
 *
 * Outputs log messages to console with optional ANSI color codes.
 * Error and Critical messages go to stderr, others to stdout.
 */
class ConsoleSink : public LogSink {
private:
    bool m_use_color;

    /**
     * @brief Get ANSI color code for log level
     * @param level Log level
     * @return ANSI escape sequence for color, or empty string if color disabled
     */
    std::string getColorCode(LogLevel level) const {
        if (!m_use_color) return "";

        switch (level) {
            case LogLevel::Trace:    return "\033[37m";  // White
            case LogLevel::Debug:    return "\033[36m";  // Cyan
            case LogLevel::Info:     return "\033[32m";  // Green
            case LogLevel::Warning:  return "\033[33m";  // Yellow
            case LogLevel::Error:    return "\033[31m";  // Red
            case LogLevel::Critical: return "\033[35m";  // Magenta
            default:                 return "\033[0m";   // Reset
        }
    }

    /**
     * @brief Get ANSI reset code
     * @return ANSI escape sequence to reset color, or empty string if color disabled
     */
    std::string getResetCode() const {
        return m_use_color ? "\033[0m" : "";
    }

public:
    /**
     * @brief Constructs a console sink
     * @param use_color Enable ANSI color codes (default: true)
     * @param level Minimum log level (default: Trace)
     */
    ConsoleSink(bool use_color = true, LogLevel level = LogLevel::Trace)
        : LogSink(level), m_use_color(use_color) {}

    /**
     * @brief Write log message to console
     * @param msg Log message to write
     */
    void log(const LogMessage& msg) override {
        if (!shouldLog(msg.level)) return;

        std::lock_guard<std::mutex> lock(m_mutex);
        std::string formatted = m_formatter.format(msg);

        std::ostream& out = (msg.level >= LogLevel::Error) ? std::cerr : std::cout;
        out << getColorCode(msg.level) << formatted << getResetCode() << std::endl;
    }

    /**
     * @brief Flush console output streams
     */
    void flush() override {
        std::cout.flush();
        std::cerr.flush();
    }
};

/**
 * @brief File sink (writes to a file)
 *
 * Writes log messages to a single file. Can either truncate existing
 * file or append to it.
 */
class FileSink : public LogSink {
private:
    std::string m_filepath;
    std::ofstream m_file;
    bool m_truncate;

public:
    /**
     * @brief Constructs a file sink
     * @param filepath Path to log file
     * @param truncate If true, truncate existing file; if false, append (default: false)
     * @param level Minimum log level (default: Trace)
     * @throws std::runtime_error if file cannot be opened
     */
    FileSink(const std::string& filepath, bool truncate = false, LogLevel level = LogLevel::Trace)
        : LogSink(level), m_filepath(filepath), m_truncate(truncate) {

        auto mode = truncate ? std::ios::out : (std::ios::out | std::ios::app);
        m_file.open(filepath, mode);

        if (!m_file.is_open()) {
            throw std::runtime_error("Failed to open log file: " + filepath);
        }
    }

    /**
     * @brief Destructor - closes file if open
     */
    ~FileSink() {
        if (m_file.is_open()) {
            m_file.close();
        }
    }

    /**
     * @brief Write log message to file
     * @param msg Log message to write
     */
    void log(const LogMessage& msg) override {
        if (!shouldLog(msg.level)) return;

        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_file.is_open()) {
            m_file << m_formatter.format(msg) << std::endl;
        }
    }

    /**
     * @brief Flush file buffer
     */
    void flush() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_file.is_open()) {
            m_file.flush();
        }
    }
};

/**
 * @brief Rotating file sink (creates new files based on size)
 *
 * Automatically rotates log files when they reach a maximum size.
 * Old files are renamed with numeric suffixes (.1, .2, etc.), with
 * oldest files being deleted when max file count is reached.
 */
class RotatingFileSink : public LogSink {
private:
    std::string m_base_filepath;
    size_t m_max_size;
    size_t m_max_files;
    std::ofstream m_file;
    size_t m_current_size;
    size_t m_current_index;

    /**
     * @brief Rotate log files when size limit is reached
     *
     * Closes current file, renames existing files (.1 -> .2, etc.),
     * deletes oldest file if necessary, and opens new file.
     */
    void rotateFiles() {
        m_file.close();

        // Remove oldest file if exists
        if (m_max_files > 0) {
            std::string oldest = m_base_filepath + "." + std::to_string(m_max_files);
            std::remove(oldest.c_str());

            // Rotate existing files
            for (size_t i = m_max_files - 1; i > 0; --i) {
                std::string from = m_base_filepath + "." + std::to_string(i);
                std::string to = m_base_filepath + "." + std::to_string(i + 1);
                std::rename(from.c_str(), to.c_str());
            }

            // Rename current to .1
            std::rename(m_base_filepath.c_str(), (m_base_filepath + ".1").c_str());
        }

        // Open new file
        m_file.open(m_base_filepath, std::ios::out);
        m_current_size = 0;
    }

public:
    /**
     * @brief Constructs a rotating file sink
     * @param filepath Base path for log files
     * @param max_size Maximum size in bytes before rotation
     * @param max_files Maximum number of rotated files to keep (0 = unlimited)
     * @param level Minimum log level (default: Trace)
     * @throws std::runtime_error if file cannot be opened
     */
    RotatingFileSink(const std::string& filepath, size_t max_size, size_t max_files, LogLevel level = LogLevel::Trace)
        : LogSink(level)
        , m_base_filepath(filepath)
        , m_max_size(max_size)
        , m_max_files(max_files)
        , m_current_size(0)
        , m_current_index(0) {

        m_file.open(m_base_filepath, std::ios::out | std::ios::app);
        if (!m_file.is_open()) {
            throw std::runtime_error("Failed to open rotating log file: " + filepath);
        }

        // Get current file size
        m_file.seekp(0, std::ios::end);
        m_current_size = m_file.tellp();
    }

    /**
     * @brief Destructor - closes file if open
     */
    ~RotatingFileSink() {
        if (m_file.is_open()) {
            m_file.close();
        }
    }

    /**
     * @brief Write log message to file, rotating if size limit exceeded
     * @param msg Log message to write
     */
    void log(const LogMessage& msg) override {
        if (!shouldLog(msg.level)) return;

        std::lock_guard<std::mutex> lock(m_mutex);

        std::string formatted = m_formatter.format(msg) + "\n";
        size_t msg_size = formatted.size();

        if (m_current_size + msg_size > m_max_size) {
            rotateFiles();
        }

        if (m_file.is_open()) {
            m_file << formatted;
            m_current_size += msg_size;
        }
    }

    /**
     * @brief Flush file buffer
     */
    void flush() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_file.is_open()) {
            m_file.flush();
        }
    }
};

/**
 * @brief Logger class
 *
 * Main logging interface that manages multiple sinks and provides
 * level-based filtering. Thread-safe for concurrent access.
 */
class Logger {
private:
    std::string m_name;                                ///< Name of this logger
    LogLevel m_level;                                  ///< Minimum log level
    std::vector<std::shared_ptr<LogSink>> m_sinks;    ///< Registered output sinks
    mutable std::mutex m_mutex;                        ///< Mutex for thread-safety

public:
    /**
     * @brief Constructs a logger
     * @param name Logger name
     * @param level Minimum log level (default: Trace)
     */
    Logger(const std::string& name, LogLevel level = LogLevel::Trace)
        : m_name(name), m_level(level) {}

    /**
     * @brief Add an output sink to this logger
     * @param sink Shared pointer to sink to add
     */
    void addSink(std::shared_ptr<LogSink> sink) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sinks.push_back(sink);
    }

    /**
     * @brief Remove a sink from this logger
     * @param sink Shared pointer to sink to remove
     */
    void removeSink(std::shared_ptr<LogSink> sink) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sinks.erase(std::remove(m_sinks.begin(), m_sinks.end(), sink), m_sinks.end());
    }

    /**
     * @brief Remove all sinks from this logger
     */
    void clearSinks() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sinks.clear();
    }

    /**
     * @brief Set the minimum log level for this logger
     * @param level New minimum log level
     */
    void setLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_level = level;
    }

    /**
     * @brief Get the minimum log level for this logger
     * @return Current minimum log level
     */
    LogLevel getLevel() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_level;
    }

    /**
     * @brief Log a message with specified level
     * @param level Log level/severity
     * @param message Message content
     * @param file Source file path (optional)
     * @param line Line number (optional)
     * @param function Function name (optional)
     */
    void log(LogLevel level, const std::string& message,
             const std::string& file = "", int line = 0, const std::string& function = "") {
        if (level < m_level) return;

        LogMessage msg(level, m_name, message, file, line, function);

        // Copy sinks to avoid holding lock during log operations
        std::vector<std::shared_ptr<LogSink>> sinks;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            sinks = m_sinks;
        }

        for (auto& sink : sinks) {
            sink->log(msg);
        }
    }

    /**
     * @brief Log a trace message
     * @param message Message content
     * @param file Source file path (optional)
     * @param line Line number (optional)
     * @param function Function name (optional)
     */
    void trace(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "") {
        log(LogLevel::Trace, message, file, line, function);
    }

    /**
     * @brief Log a debug message
     * @param message Message content
     * @param file Source file path (optional)
     * @param line Line number (optional)
     * @param function Function name (optional)
     */
    void debug(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "") {
        log(LogLevel::Debug, message, file, line, function);
    }

    /**
     * @brief Log an info message
     * @param message Message content
     * @param file Source file path (optional)
     * @param line Line number (optional)
     * @param function Function name (optional)
     */
    void info(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "") {
        log(LogLevel::Info, message, file, line, function);
    }

    /**
     * @brief Log a warning message
     * @param message Message content
     * @param file Source file path (optional)
     * @param line Line number (optional)
     * @param function Function name (optional)
     */
    void warning(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "") {
        log(LogLevel::Warning, message, file, line, function);
    }

    /**
     * @brief Log an error message
     * @param message Message content
     * @param file Source file path (optional)
     * @param line Line number (optional)
     * @param function Function name (optional)
     */
    void error(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "") {
        log(LogLevel::Error, message, file, line, function);
    }

    /**
     * @brief Log a critical message
     * @param message Message content
     * @param file Source file path (optional)
     * @param line Line number (optional)
     * @param function Function name (optional)
     */
    void critical(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "") {
        log(LogLevel::Critical, message, file, line, function);
    }

    /**
     * @brief Flush all sinks associated with this logger
     */
    void flush() {
        std::vector<std::shared_ptr<LogSink>> sinks;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            sinks = m_sinks;
        }

        for (auto& sink : sinks) {
            sink->flush();
        }
    }

    /**
     * @brief Get the name of this logger
     * @return Logger name
     */
    std::string getName() const { return m_name; }
};

/**
 * @brief Global logger registry
 *
 * Singleton registry for managing named loggers. Provides a default
 * logger with console output and allows creating/retrieving loggers
 * by name. Thread-safe for concurrent access.
 */
class LoggerRegistry {
private:
    std::unordered_map<std::string, std::shared_ptr<Logger>> m_loggers; ///< Map of named loggers
    std::shared_ptr<Logger> m_default_logger;                            ///< Default logger instance
    mutable std::mutex m_mutex;                                          ///< Mutex for thread-safety

    /**
     * @brief Private constructor (singleton pattern)
     *
     * Creates default logger with console sink at Info level.
     */
    LoggerRegistry() {
        // Create default logger with console sink
        m_default_logger = std::make_shared<Logger>("default", LogLevel::Info);
        m_default_logger->addSink(std::make_shared<ConsoleSink>(true, LogLevel::Info));
        m_loggers["default"] = m_default_logger;
    }

public:
    /**
     * @brief Get the singleton registry instance
     * @return Reference to the global LoggerRegistry instance
     */
    static LoggerRegistry& instance() {
        static LoggerRegistry registry;
        return registry;
    }

    /**
     * @brief Get or create a logger by name
     *
     * If logger doesn't exist, creates a new one with default level.
     *
     * @param name Logger name
     * @return Shared pointer to logger
     */
    std::shared_ptr<Logger> getLogger(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_loggers.find(name);
        if (it != m_loggers.end()) {
            return it->second;
        }

        // Create new logger with default configuration
        auto logger = std::make_shared<Logger>(name, m_default_logger->getLevel());
        m_loggers[name] = logger;
        return logger;
    }

    /**
     * @brief Register a logger with a specific name
     *
     * Adds or replaces a logger in the registry.
     *
     * @param name Logger name
     * @param logger Shared pointer to logger
     */
    void registerLogger(const std::string& name, std::shared_ptr<Logger> logger) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_loggers[name] = logger;
    }

    /**
     * @brief Get the default logger
     * @return Shared pointer to default logger
     */
    std::shared_ptr<Logger> getDefaultLogger() {
        return m_default_logger;
    }

    /**
     * @brief Set a new default logger
     * @param logger Shared pointer to logger to use as default
     */
    void setDefaultLogger(std::shared_ptr<Logger> logger) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_default_logger = logger;
    }

    /**
     * @brief Set log level for all registered loggers
     * @param level New minimum log level to apply globally
     */
    void setGlobalLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& pair : m_loggers) {
            pair.second->setLevel(level);
        }
    }

    /**
     * @brief Flush all registered loggers
     */
    void flushAll() {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& pair : m_loggers) {
            pair.second->flush();
        }
    }
};

} // namespace mcf

/**
 * @defgroup LoggingMacros Logging Convenience Macros
 * @brief Convenience macros that automatically capture source location (file, line, function)
 * @{
 */

/**
 * @brief Log trace message with specific logger
 * @param logger Logger instance (shared_ptr)
 * @param msg Message to log
 */
#define MCF_LOG_TRACE(logger, msg) logger->trace(msg, __FILE__, __LINE__, __FUNCTION__)

/**
 * @brief Log debug message with specific logger
 * @param logger Logger instance (shared_ptr)
 * @param msg Message to log
 */
#define MCF_LOG_DEBUG(logger, msg) logger->debug(msg, __FILE__, __LINE__, __FUNCTION__)

/**
 * @brief Log info message with specific logger
 * @param logger Logger instance (shared_ptr)
 * @param msg Message to log
 */
#define MCF_LOG_INFO(logger, msg) logger->info(msg, __FILE__, __LINE__, __FUNCTION__)

/**
 * @brief Log warning message with specific logger
 * @param logger Logger instance (shared_ptr)
 * @param msg Message to log
 */
#define MCF_LOG_WARNING(logger, msg) logger->warning(msg, __FILE__, __LINE__, __FUNCTION__)

/**
 * @brief Log error message with specific logger
 * @param logger Logger instance (shared_ptr)
 * @param msg Message to log
 */
#define MCF_LOG_ERROR(logger, msg) logger->error(msg, __FILE__, __LINE__, __FUNCTION__)

/**
 * @brief Log critical message with specific logger
 * @param logger Logger instance (shared_ptr)
 * @param msg Message to log
 */
#define MCF_LOG_CRITICAL(logger, msg) logger->critical(msg, __FILE__, __LINE__, __FUNCTION__)

/**
 * @brief Log trace message using default logger
 * @param msg Message to log
 */
#define MCF_TRACE(msg) mcf::LoggerRegistry::instance().getDefaultLogger()->trace(msg, __FILE__, __LINE__, __FUNCTION__)

/**
 * @brief Log debug message using default logger
 * @param msg Message to log
 */
#define MCF_DEBUG(msg) mcf::LoggerRegistry::instance().getDefaultLogger()->debug(msg, __FILE__, __LINE__, __FUNCTION__)

/**
 * @brief Log info message using default logger
 * @param msg Message to log
 */
#define MCF_INFO(msg) mcf::LoggerRegistry::instance().getDefaultLogger()->info(msg, __FILE__, __LINE__, __FUNCTION__)

/**
 * @brief Log warning message using default logger
 * @param msg Message to log
 */
#define MCF_WARNING(msg) mcf::LoggerRegistry::instance().getDefaultLogger()->warning(msg, __FILE__, __LINE__, __FUNCTION__)

/**
 * @brief Log error message using default logger
 * @param msg Message to log
 */
#define MCF_ERROR(msg) mcf::LoggerRegistry::instance().getDefaultLogger()->error(msg, __FILE__, __LINE__, __FUNCTION__)

/**
 * @brief Log critical message using default logger
 * @param msg Message to log
 */
#define MCF_CRITICAL(msg) mcf::LoggerRegistry::instance().getDefaultLogger()->critical(msg, __FILE__, __LINE__, __FUNCTION__)

/** @} */ // end of LoggingMacros group

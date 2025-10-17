#pragma once

#include "../../core/IModule.hpp"
#include "../../core/Logger.hpp"
#include "../../core/ConfigurationManager.hpp"
#include "../../core/Application.hpp"

#include <memory>
#include <string>

namespace mcf {

/**
 * @brief Logger module that integrates with ConfigurationManager
 *
 * Configuration format (JSON):
 * {
 *   "logging": {
 *     "global_level": "info",
 *     "pattern": "[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v",
 *     "loggers": [
 *       {
 *         "name": "app",
 *         "level": "debug",
 *         "sinks": [
 *           {
 *             "type": "console",
 *             "level": "debug",
 *             "color": true
 *           },
 *           {
 *             "type": "file",
 *             "level": "trace",
 *             "path": "logs/app.log",
 *             "truncate": false
 *           },
 *           {
 *             "type": "rotating",
 *             "level": "info",
 *             "path": "logs/app_rotating.log",
 *             "max_size": 10485760,
 *             "max_files": 5
 *           }
 *         ]
 *       }
 *     ]
 *   }
 * }
 */
class LoggerModule : public ModuleBase {
private:
    ConfigurationManager* m_config_manager;
    std::vector<std::shared_ptr<Logger>> m_managed_loggers;
    bool m_watch_config;

    /**
     * @brief Parse log level from JSON value
     */
    LogLevel parseLogLevel(const JsonValue& value, LogLevel defaultLevel = LogLevel::Info) const {
        if (value.isString()) {
            return stringToLogLevel(value.asString());
        }
        return defaultLevel;
    }

    /**
     * @brief Create a sink from configuration
     */
    std::shared_ptr<LogSink> createSink(const JsonValue& sinkConfig) const {
        if (!sinkConfig.isObject() || !sinkConfig.has("type")) {
            return nullptr;
        }

        std::string type = sinkConfig["type"].asString();
        LogLevel level = parseLogLevel(sinkConfig.has("level") ? sinkConfig["level"] : JsonValue());

        if (type == "console") {
            bool use_color = true;
            if (sinkConfig.has("color")) {
                use_color = sinkConfig["color"].asBool();
            }
            return std::make_shared<ConsoleSink>(use_color, level);
        }
        else if (type == "file") {
            if (!sinkConfig.has("path")) {
                return nullptr;
            }

            std::string path = sinkConfig["path"].asString();
            bool truncate = false;
            if (sinkConfig.has("truncate")) {
                truncate = sinkConfig["truncate"].asBool();
            }

            try {
                return std::make_shared<FileSink>(path, truncate, level);
            } catch (const std::exception&) {
                return nullptr;
            }
        }
        else if (type == "rotating") {
            if (!sinkConfig.has("path")) {
                return nullptr;
            }

            std::string path = sinkConfig["path"].asString();
            size_t max_size = 10 * 1024 * 1024; // 10MB default
            size_t max_files = 5; // Default

            if (sinkConfig.has("max_size")) {
                max_size = static_cast<size_t>(sinkConfig["max_size"].asInt());
            }
            if (sinkConfig.has("max_files")) {
                max_files = static_cast<size_t>(sinkConfig["max_files"].asInt());
            }

            try {
                return std::make_shared<RotatingFileSink>(path, max_size, max_files, level);
            } catch (const std::exception&) {
                return nullptr;
            }
        }

        return nullptr;
    }

    /**
     * @brief Configure a logger from JSON configuration
     */
    std::shared_ptr<Logger> configureLogger(const JsonValue& loggerConfig) const {
        if (!loggerConfig.isObject() || !loggerConfig.has("name")) {
            return nullptr;
        }

        std::string name = loggerConfig["name"].asString();
        LogLevel level = parseLogLevel(loggerConfig.has("level") ? loggerConfig["level"] : JsonValue());

        auto logger = std::make_shared<Logger>(name, level);

        // Add sinks
        if (loggerConfig.has("sinks") && loggerConfig["sinks"].isArray()) {
            JsonArray sinks = loggerConfig["sinks"].asArray();
            for (const auto& sinkConfig : sinks) {
                auto sink = createSink(sinkConfig);
                if (sink) {
                    logger->addSink(sink);
                }
            }
        }

        return logger;
    }

    /**
     * @brief Load logging configuration
     */
    void loadConfiguration() {
        if (!m_config_manager) {
            return;
        }

        JsonValue loggingConfig = m_config_manager->get("logging");
        if (loggingConfig.isNull() || !loggingConfig.isObject()) {
            // No logging configuration, use defaults
            return;
        }

        // Set global level
        if (loggingConfig.has("global_level")) {
            LogLevel globalLevel = parseLogLevel(loggingConfig["global_level"]);
            LoggerRegistry::instance().setGlobalLevel(globalLevel);
        }

        // Set global pattern
        if (loggingConfig.has("pattern")) {
            // Pattern will be applied to new sinks
            // Existing sinks won't be modified
        }

        // Configure loggers
        if (loggingConfig.has("loggers") && loggingConfig["loggers"].isArray()) {
            JsonArray loggers = loggingConfig["loggers"].asArray();
            for (const auto& loggerConfig : loggers) {
                auto logger = configureLogger(loggerConfig);
                if (logger) {
                    LoggerRegistry::instance().registerLogger(logger->getName(), logger);
                    m_managed_loggers.push_back(logger);
                }
            }
        }
    }

    /**
     * @brief Configuration change callback
     */
    void onConfigChanged(const std::string& key, const JsonValue& value) {
        if (key.find("logging") == 0) {
            // Reload logging configuration
            loadConfiguration();
        }
    }

public:
    LoggerModule(bool watch_config = true)
        : ModuleBase("LoggerModule", "1.0.0", 900) // High priority
        , m_config_manager(nullptr)
        , m_watch_config(watch_config) {}

    bool initialize(Application& app) override {
        if (m_initialized) {
            return true;
        }

        // Get ConfigurationManager from ServiceLocator
        auto serviceLocator = app.getServiceLocator();
        if (serviceLocator) {
            m_config_manager = serviceLocator->resolve<ConfigurationManager>().get();
        }

        // Load configuration
        loadConfiguration();

        // Watch for configuration changes if enabled
        if (m_watch_config && m_config_manager) {
            m_config_manager->watch("logging", [this](const std::string& key, const JsonValue& value) {
                onConfigChanged(key, value);
            });
        }

        m_initialized = true;

        // Log initialization
        MCF_INFO("LoggerModule initialized");

        return true;
    }

    void shutdown() override {
        if (!m_initialized) {
            return;
        }

        MCF_INFO("LoggerModule shutting down");

        // Flush all loggers
        LoggerRegistry::instance().flushAll();

        m_managed_loggers.clear();
        m_config_manager = nullptr;
        m_initialized = false;
    }

    // LoggerModule doesn't need real-time updates
    // It's event-driven (logs are written immediately)
    // If periodic flushing is needed, implement IRealtimeUpdatable

    /**
     * @brief Get a logger by name (convenience method)
     */
    std::shared_ptr<Logger> getLogger(const std::string& name) const {
        return LoggerRegistry::instance().getLogger(name);
    }

    /**
     * @brief Get the default logger
     */
    std::shared_ptr<Logger> getDefaultLogger() const {
        return LoggerRegistry::instance().getDefaultLogger();
    }

    /**
     * @brief Create a logger programmatically
     */
    std::shared_ptr<Logger> createLogger(const std::string& name,
                                         LogLevel level = LogLevel::Info,
                                         bool add_console = true,
                                         bool add_file = false,
                                         const std::string& file_path = "") {
        auto logger = std::make_shared<Logger>(name, level);

        if (add_console) {
            logger->addSink(std::make_shared<ConsoleSink>(true, level));
        }

        if (add_file && !file_path.empty()) {
            try {
                logger->addSink(std::make_shared<FileSink>(file_path, false, level));
            } catch (const std::exception&) {
                // Failed to create file sink
            }
        }

        LoggerRegistry::instance().registerLogger(name, logger);
        m_managed_loggers.push_back(logger);

        return logger;
    }

    /**
     * @brief Reload configuration from ConfigurationManager
     */
    void reloadConfiguration() {
        loadConfiguration();
    }

    /**
     * @brief Flush all managed loggers
     */
    void flushAll() {
        for (auto& logger : m_managed_loggers) {
            logger->flush();
        }
    }
};

} // namespace mcf

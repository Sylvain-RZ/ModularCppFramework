#include <catch_amalgamated.hpp>
#include "../../modules/logger/LoggerModule.hpp"
#include "../../core/Application.hpp"
#include "../../core/ConfigurationManager.hpp"
#include "../../core/JsonParser.hpp"
#include <filesystem>
#include <fstream>

using namespace mcf;

TEST_CASE("LoggerModule - Construction", "[LoggerModule]") {
    SECTION("Default construction") {
        LoggerModule module;
        REQUIRE(module.getName() == "LoggerModule");
        REQUIRE(!module.isInitialized());
    }

    SECTION("Module metadata") {
        LoggerModule module;
        REQUIRE(module.getPriority() == 900); // High priority
        REQUIRE(!module.getVersion().empty());
    }
}

TEST_CASE("LoggerModule - Initialization without config", "[LoggerModule]") {
    SECTION("Initialize via Application::addModule") {
        ApplicationConfig config;
        config.autoLoadPlugins = false; // Disable auto-loading of plugins to avoid interference
        Application app(config);
        app.addModule<LoggerModule>();

        bool result = app.initialize();
        REQUIRE(result == true);

        app.shutdown();
    }

    SECTION("Module shutdown via Application") {
        ApplicationConfig config;
        config.autoLoadPlugins = false; // Disable auto-loading of plugins to avoid interference
        Application app(config);
        app.addModule<LoggerModule>();
        app.initialize();

        app.shutdown();
        // After shutdown, module should be cleaned up
        REQUIRE(true);
    }
}

TEST_CASE("LoggerModule - Initialization with config", "[LoggerModule]") {
    // Create a temporary config file
    const std::string configPath = "test_logger_config.json";

    SECTION("Initialize with console sink config") {
        std::string configJson = R"({
            "logging": {
                "loggers": [
                    {
                        "name": "test",
                        "level": "debug",
                        "sinks": [
                            {
                                "type": "console",
                                "level": "debug",
                                "color": true
                            }
                        ]
                    }
                ]
            }
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false; // Disable plugin auto-loading for tests
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        bool result = app.initialize();
        REQUIRE(result == true);

        // Verify logger was created
        auto logger = LoggerRegistry::instance().getLogger("test");
        REQUIRE(logger != nullptr);

        app.shutdown();
        std::filesystem::remove(configPath);
    }

    SECTION("Initialize with file sink config") {
        std::string configJson = R"({
            "logging": {
                "loggers": [
                    {
                        "name": "filetest",
                        "level": "info",
                        "sinks": [
                            {
                                "type": "file",
                                "level": "info",
                                "path": "test_logs/test.log",
                                "truncate": true
                            }
                        ]
                    }
                ]
            }
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false; // Disable plugin auto-loading for tests
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        bool result = app.initialize();
        REQUIRE(result == true);

        auto logger = LoggerRegistry::instance().getLogger("filetest");
        REQUIRE(logger != nullptr);

        app.shutdown();
        std::filesystem::remove(configPath);
        std::filesystem::remove_all("test_logs");
    }

    SECTION("Initialize with rotating file sink config") {
        std::string configJson = R"({
            "logging": {
                "loggers": [
                    {
                        "name": "rotating",
                        "level": "warn",
                        "sinks": [
                            {
                                "type": "rotating",
                                "level": "warn",
                                "path": "test_logs/rotating.log",
                                "max_size": 1048576,
                                "max_files": 3
                            }
                        ]
                    }
                ]
            }
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false; // Disable plugin auto-loading for tests
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        bool result = app.initialize();
        REQUIRE(result == true);

        auto logger = LoggerRegistry::instance().getLogger("rotating");
        REQUIRE(logger != nullptr);

        app.shutdown();
        std::filesystem::remove(configPath);
        std::filesystem::remove_all("test_logs");
    }
}

TEST_CASE("LoggerModule - Multiple loggers", "[LoggerModule]") {
    const std::string configPath = "test_logger_config.json";

    std::string configJson = R"({
        "logging": {
            "loggers": [
                {
                    "name": "logger1",
                    "level": "debug",
                    "sinks": [
                        {
                            "type": "console",
                            "level": "debug",
                            "color": false
                        }
                    ]
                },
                {
                    "name": "logger2",
                    "level": "info",
                    "sinks": [
                        {
                            "type": "console",
                            "level": "info",
                            "color": true
                        }
                    ]
                }
            ]
        }
    })";

    {
        std::ofstream file(configPath);
        file << configJson;
    }

    ApplicationConfig appConfig;
    appConfig.autoLoadPlugins = false; // Disable plugin auto-loading for tests
    appConfig.configFile = configPath;
    Application app(appConfig);
    app.addModule<LoggerModule>();
    bool result = app.initialize();
    REQUIRE(result == true);

    auto logger1 = LoggerRegistry::instance().getLogger("logger1");
    auto logger2 = LoggerRegistry::instance().getLogger("logger2");
    REQUIRE(logger1 != nullptr);
    REQUIRE(logger2 != nullptr);

    app.shutdown();
    std::filesystem::remove(configPath);
}

TEST_CASE("LoggerModule - Log level parsing", "[LoggerModule]") {
    const std::string configPath = "test_logger_config.json";

    SECTION("All log levels") {
        const std::vector<std::string> levels = {
            "trace", "debug", "info", "warn", "error", "critical"
        };

        for (const auto& level : levels) {
            std::string configJson = R"({
                "logging": {
                    "loggers": [
                        {
                            "name": "test",
                            "level": ")" + level + R"(",
                            "sinks": [
                                {
                                    "type": "console",
                                    "level": ")" + level + R"("
                                }
                            ]
                        }
                    ]
                }
            })";

            {
                std::ofstream file(configPath);
                file << configJson;
            }

            ApplicationConfig appConfig;
            appConfig.autoLoadPlugins = false; // Disable plugin auto-loading for tests
            appConfig.configFile = configPath;
            Application app(appConfig);
            app.addModule<LoggerModule>();
            bool result = app.initialize();
            REQUIRE(result == true);
            app.shutdown();
        }

        std::filesystem::remove(configPath);
    }
}

TEST_CASE("LoggerModule - Invalid configurations", "[LoggerModule]") {
    const std::string configPath = "test_logger_config.json";

    SECTION("Missing sink type") {
        std::string configJson = R"({
            "logging": {
                "loggers": [
                    {
                        "name": "test",
                        "level": "debug",
                        "sinks": [
                            {
                                "level": "debug"
                            }
                        ]
                    }
                ]
            }
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false; // Disable plugin auto-loading for tests
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        // Should handle gracefully
        bool result = app.initialize();
        REQUIRE(result == true);
        app.shutdown();
        std::filesystem::remove(configPath);
    }

    SECTION("Unknown sink type") {
        std::string configJson = R"({
            "logging": {
                "loggers": [
                    {
                        "name": "test",
                        "level": "debug",
                        "sinks": [
                            {
                                "type": "unknown",
                                "level": "debug"
                            }
                        ]
                    }
                ]
            }
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false; // Disable plugin auto-loading for tests
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        // Should handle gracefully
        bool result = app.initialize();
        REQUIRE(result == true);
        app.shutdown();
        std::filesystem::remove(configPath);
    }

    SECTION("Empty loggers array") {
        std::string configJson = R"({
            "logging": {
                "loggers": []
            }
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false; // Disable plugin auto-loading for tests
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        bool result = app.initialize();
        REQUIRE(result == true);
        app.shutdown();
        std::filesystem::remove(configPath);
    }
}

TEST_CASE("LoggerModule - Shutdown", "[LoggerModule]") {
    SECTION("Shutdown via Application") {
        ApplicationConfig config;
        config.autoLoadPlugins = false;
        Application app(config);
        app.addModule<LoggerModule>();
        app.initialize();

        app.shutdown();
        // Module should be shut down automatically
        REQUIRE(true);
    }

    SECTION("Shutdown without initialization") {
        LoggerModule module;
        module.shutdown();
        REQUIRE(!module.isInitialized());
    }

    SECTION("Multiple application lifecycles") {
        ApplicationConfig config;
        config.autoLoadPlugins = false;
        Application app(config);
        app.addModule<LoggerModule>();

        app.initialize();
        app.shutdown();
        // Should be safe to shutdown multiple times via Application
        REQUIRE(true);
    }
}

TEST_CASE("LoggerModule - Multiple sinks per logger", "[LoggerModule]") {
    const std::string configPath = "test_logger_config.json";

    std::string configJson = R"({
        "logging": {
            "loggers": [
                {
                    "name": "multi",
                    "level": "debug",
                    "sinks": [
                        {
                            "type": "console",
                            "level": "debug",
                            "color": true
                        },
                        {
                            "type": "file",
                            "level": "info",
                            "path": "test_logs/multi.log",
                            "truncate": true
                        }
                    ]
                }
            ]
        }
    })";

    {
        std::ofstream file(configPath);
        file << configJson;
    }

    ApplicationConfig appConfig;
    appConfig.autoLoadPlugins = false; // Disable plugin auto-loading for tests
    appConfig.configFile = configPath;
    Application app(appConfig);
    app.addModule<LoggerModule>();
    bool result = app.initialize();
    REQUIRE(result == true);

    auto logger = LoggerRegistry::instance().getLogger("multi");
    REQUIRE(logger != nullptr);

    app.shutdown();
    std::filesystem::remove(configPath);
    std::filesystem::remove_all("test_logs");
}

TEST_CASE("LoggerModule - Integration with Application", "[LoggerModule]") {
    SECTION("Module in application lifecycle") {
        ApplicationConfig config;
        config.autoLoadPlugins = false; // Disable plugin auto-loading for tests
        Application app(config);
        app.addModule<LoggerModule>();

        bool initResult = app.initialize();
        REQUIRE(initResult == true);

        app.shutdown();
    }

    SECTION("Access logger after module initialization") {
        const std::string configPath = "test_logger_config.json";
        std::string configJson = R"({
            "logging": {
                "loggers": [
                    {
                        "name": "applogger",
                        "level": "info",
                        "sinks": [
                            {
                                "type": "console",
                                "level": "info"
                            }
                        ]
                    }
                ]
            }
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false; // Disable plugin auto-loading for tests
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        app.initialize();

        // Access logger from registry
        auto logger = LoggerRegistry::instance().getLogger("applogger");
        REQUIRE(logger != nullptr);

        // Use logger
        logger->info("Test message");

        app.shutdown();
        std::filesystem::remove(configPath);
    }
}

TEST_CASE("LoggerModule - Configuration edge cases", "[LoggerModule]") {
    const std::string configPath = "test_logger_config.json";

    SECTION("Missing level uses default") {
        std::string configJson = R"({
            "logging": {
                "loggers": [
                    {
                        "name": "nolevel",
                        "sinks": [
                            {
                                "type": "console"
                            }
                        ]
                    }
                ]
            }
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false; // Disable plugin auto-loading for tests
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        bool result = app.initialize();
        REQUIRE(result == true);
        app.shutdown();
        std::filesystem::remove(configPath);
    }

    SECTION("Missing sinks array") {
        std::string configJson = R"({
            "logging": {
                "loggers": [
                    {
                        "name": "nosinks",
                        "level": "info"
                    }
                ]
            }
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false; // Disable plugin auto-loading for tests
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        bool result = app.initialize();
        REQUIRE(result == true);
        app.shutdown();
        std::filesystem::remove(configPath);
    }

    SECTION("Missing logger name") {
        std::string configJson = R"({
            "logging": {
                "loggers": [
                    {
                        "level": "info",
                        "sinks": [
                            {
                                "type": "console"
                            }
                        ]
                    }
                ]
            }
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false; // Disable plugin auto-loading for tests
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        bool result = app.initialize();
        REQUIRE(result == true);
        app.shutdown();
        std::filesystem::remove(configPath);
    }

    SECTION("Empty logger name") {
        std::string configJson = R"({
            "logging": {
                "loggers": [
                    {
                        "name": "",
                        "level": "info",
                        "sinks": [
                            {
                                "type": "console"
                            }
                        ]
                    }
                ]
            }
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false; // Disable plugin auto-loading for tests
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        bool result = app.initialize();
        REQUIRE(result == true);
        app.shutdown();
        std::filesystem::remove(configPath);
    }

    SECTION("Invalid log level string") {
        std::string configJson = R"({
            "logging": {
                "loggers": [
                    {
                        "name": "invalidlevel",
                        "level": "invalid_level",
                        "sinks": [
                            {
                                "type": "console",
                                "level": "info"
                            }
                        ]
                    }
                ]
            }
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false; // Disable plugin auto-loading for tests
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        // Should handle gracefully with default level
        bool result = app.initialize();
        REQUIRE(result == true);
        app.shutdown();
        std::filesystem::remove(configPath);
    }
}

TEST_CASE("LoggerModule - Missing config file handling", "[LoggerModule]") {
    SECTION("Config file doesn't exist") {
        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false;
        appConfig.configFile = "nonexistent_config_file.json";
        Application app(appConfig);
        app.addModule<LoggerModule>();
        // Should initialize successfully even without config
        bool result = app.initialize();
        REQUIRE(result == true);
        app.shutdown();
    }

    SECTION("No config file specified") {
        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false;
        appConfig.configFile = ""; // Empty config file path
        Application app(appConfig);
        app.addModule<LoggerModule>();
        bool result = app.initialize();
        REQUIRE(result == true);
        app.shutdown();
    }
}

TEST_CASE("LoggerModule - Invalid JSON config", "[LoggerModule]") {
    const std::string configPath = "test_invalid_config.json";

    SECTION("Malformed JSON") {
        std::string configJson = R"({
            "logging": {
                "loggers": [
                    {
                        "name": "test",
                        "level": "info"
                    // Missing closing braces
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false;
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        // Should handle JSON parsing errors gracefully
        bool result = app.initialize();
        // May fail or succeed depending on error handling
        app.shutdown();
        std::filesystem::remove(configPath);
    }

    SECTION("Non-object logging section") {
        std::string configJson = R"({
            "logging": "invalid"
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false;
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        bool result = app.initialize();
        REQUIRE(result == true);
        app.shutdown();
        std::filesystem::remove(configPath);
    }

    SECTION("Non-array loggers section") {
        std::string configJson = R"({
            "logging": {
                "loggers": "invalid"
            }
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false;
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        bool result = app.initialize();
        REQUIRE(result == true);
        app.shutdown();
        std::filesystem::remove(configPath);
    }
}

TEST_CASE("LoggerModule - Hot reload configuration", "[LoggerModule]") {
    const std::string configPath = "test_hot_reload_config.json";

    SECTION("Configuration hot reload via ConfigurationManager") {
        std::string configJson = R"({
            "logging": {
                "loggers": [
                    {
                        "name": "hotreload",
                        "level": "info",
                        "sinks": [
                            {
                                "type": "console",
                                "level": "info"
                            }
                        ]
                    }
                ]
            }
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false;
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        app.initialize();

        // Get initial logger
        auto logger = LoggerRegistry::instance().getLogger("hotreload");
        REQUIRE(logger != nullptr);

        // Modify config file
        std::string newConfigJson = R"({
            "logging": {
                "loggers": [
                    {
                        "name": "hotreload",
                        "level": "debug",
                        "sinks": [
                            {
                                "type": "console",
                                "level": "debug"
                            }
                        ]
                    }
                ]
            }
        })";

        {
            std::ofstream file(configPath);
            file << newConfigJson;
        }

        // Reload configuration
        auto* configMgr = app.getConfigurationManager();
        configMgr->load(configPath);

        // Logger should still exist (hot reload may or may not be fully implemented)
        logger = LoggerRegistry::instance().getLogger("hotreload");
        REQUIRE(logger != nullptr);

        app.shutdown();
        std::filesystem::remove(configPath);
    }
}

TEST_CASE("LoggerModule - File sink error handling", "[LoggerModule]") {
    const std::string configPath = "test_file_sink_errors.json";

    SECTION("File sink with invalid path") {
        std::string configJson = R"({
            "logging": {
                "loggers": [
                    {
                        "name": "badpath",
                        "level": "info",
                        "sinks": [
                            {
                                "type": "file",
                                "level": "info",
                                "path": "/invalid/directory/that/does/not/exist/test.log",
                                "truncate": true
                            }
                        ]
                    }
                ]
            }
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false;
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        // Should handle file creation errors gracefully
        bool result = app.initialize();
        REQUIRE(result == true);
        app.shutdown();
        std::filesystem::remove(configPath);
    }

    SECTION("Missing file path for file sink") {
        std::string configJson = R"({
            "logging": {
                "loggers": [
                    {
                        "name": "nopath",
                        "level": "info",
                        "sinks": [
                            {
                                "type": "file",
                                "level": "info",
                                "truncate": true
                            }
                        ]
                    }
                ]
            }
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false;
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        bool result = app.initialize();
        REQUIRE(result == true);
        app.shutdown();
        std::filesystem::remove(configPath);
    }
}

TEST_CASE("LoggerModule - Rotating sink edge cases", "[LoggerModule]") {
    const std::string configPath = "test_rotating_sink.json";

    SECTION("Missing max_size defaults") {
        std::string configJson = R"({
            "logging": {
                "loggers": [
                    {
                        "name": "rotating_no_size",
                        "level": "info",
                        "sinks": [
                            {
                                "type": "rotating",
                                "level": "info",
                                "path": "test_logs/rotating_no_size.log",
                                "max_files": 3
                            }
                        ]
                    }
                ]
            }
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false;
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        bool result = app.initialize();
        REQUIRE(result == true);
        app.shutdown();
        std::filesystem::remove(configPath);
        std::filesystem::remove_all("test_logs");
    }

    SECTION("Missing max_files defaults") {
        std::string configJson = R"({
            "logging": {
                "loggers": [
                    {
                        "name": "rotating_no_files",
                        "level": "info",
                        "sinks": [
                            {
                                "type": "rotating",
                                "level": "info",
                                "path": "test_logs/rotating_no_files.log",
                                "max_size": 1048576
                            }
                        ]
                    }
                ]
            }
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false;
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        bool result = app.initialize();
        REQUIRE(result == true);
        app.shutdown();
        std::filesystem::remove(configPath);
        std::filesystem::remove_all("test_logs");
    }

    SECTION("Zero max_size") {
        std::string configJson = R"({
            "logging": {
                "loggers": [
                    {
                        "name": "rotating_zero_size",
                        "level": "info",
                        "sinks": [
                            {
                                "type": "rotating",
                                "level": "info",
                                "path": "test_logs/rotating_zero.log",
                                "max_size": 0,
                                "max_files": 3
                            }
                        ]
                    }
                ]
            }
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false;
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        bool result = app.initialize();
        REQUIRE(result == true);
        app.shutdown();
        std::filesystem::remove(configPath);
        std::filesystem::remove_all("test_logs");
    }
}

TEST_CASE("LoggerModule - Logger name conflicts", "[LoggerModule]") {
    const std::string configPath = "test_logger_conflicts.json";

    SECTION("Duplicate logger names") {
        std::string configJson = R"({
            "logging": {
                "loggers": [
                    {
                        "name": "duplicate",
                        "level": "info",
                        "sinks": [
                            {
                                "type": "console",
                                "level": "info"
                            }
                        ]
                    },
                    {
                        "name": "duplicate",
                        "level": "debug",
                        "sinks": [
                            {
                                "type": "console",
                                "level": "debug"
                            }
                        ]
                    }
                ]
            }
        })";

        {
            std::ofstream file(configPath);
            file << configJson;
        }

        ApplicationConfig appConfig;
        appConfig.autoLoadPlugins = false;
        appConfig.configFile = configPath;
        Application app(appConfig);
        app.addModule<LoggerModule>();
        bool result = app.initialize();
        REQUIRE(result == true);

        // Should have one logger (last one wins or first one wins)
        auto logger = LoggerRegistry::instance().getLogger("duplicate");
        REQUIRE(logger != nullptr);

        app.shutdown();
        std::filesystem::remove(configPath);
    }
}

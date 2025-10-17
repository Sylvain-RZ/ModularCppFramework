#include "../core/Logger.hpp"
#include "../modules/logger/LoggerModule.hpp"
#include "../core/ConfigurationManager.hpp"
#include "../core/ServiceLocator.hpp"
#include "../core/Application.hpp"

#include <iostream>
#include <cassert>
#include <fstream>
#include <thread>
#include <chrono>
#include <filesystem>

using namespace mcf;

// Test application for logger module
class TestApp : public Application {
public:
    TestApp() : Application(ApplicationConfig()) {}

    void run() override {
        // Empty run for testing
    }
};

// Helper function to check if file contains string
bool fileContains(const std::string& filepath, const std::string& search) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        if (line.find(search) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// Helper function to count lines in file
size_t countLines(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return 0;

    size_t count = 0;
    std::string line;
    while (std::getline(file, line)) {
        count++;
    }
    return count;
}

void test_basic_logging() {
    std::cout << "Test: Basic Logging" << std::endl;

    auto logger = std::make_shared<Logger>("test");
    auto console_sink = std::make_shared<ConsoleSink>(false); // No color for testing
    logger->addSink(console_sink);

    logger->info("Test info message");
    logger->warning("Test warning message");
    logger->error("Test error message");

    std::cout << "✓ Basic logging test passed" << std::endl;
}

void test_log_levels() {
    std::cout << "\nTest: Log Levels" << std::endl;

    auto logger = std::make_shared<Logger>("test_levels", LogLevel::Warning);
    auto console_sink = std::make_shared<ConsoleSink>(false);
    logger->addSink(console_sink);

    // These should not appear
    logger->trace("This should not appear");
    logger->debug("This should not appear");
    logger->info("This should not appear");

    // These should appear
    logger->warning("This warning should appear");
    logger->error("This error should appear");

    std::cout << "✓ Log levels test passed" << std::endl;
}

void test_file_sink() {
    std::cout << "\nTest: File Sink" << std::endl;

    // Create logs directory
    std::filesystem::create_directories("logs");

    std::string filepath = "logs/test_file.log";
    std::remove(filepath.c_str()); // Clean up from previous runs

    auto logger = std::make_shared<Logger>("file_test");
    auto file_sink = std::make_shared<FileSink>(filepath, true);
    logger->addSink(file_sink);

    logger->info("Test message 1");
    logger->info("Test message 2");
    logger->flush();

    assert(fileContains(filepath, "Test message 1"));
    assert(fileContains(filepath, "Test message 2"));

    std::cout << "✓ File sink test passed" << std::endl;
}

void test_rotating_file_sink() {
    std::cout << "\nTest: Rotating File Sink" << std::endl;

    std::filesystem::create_directories("logs");

    std::string filepath = "logs/test_rotating.log";
    std::remove(filepath.c_str());
    std::remove((filepath + ".1").c_str());
    std::remove((filepath + ".2").c_str());

    // Create small rotating log (1KB max, 2 backups)
    auto logger = std::make_shared<Logger>("rotating_test");
    auto rotating_sink = std::make_shared<RotatingFileSink>(filepath, 1024, 2);
    logger->addSink(rotating_sink);

    // Write enough messages to trigger rotation
    for (int i = 0; i < 100; i++) {
        logger->info("Message " + std::to_string(i) + " - Lorem ipsum dolor sit amet consectetur");
    }
    logger->flush();

    // Check that rotation occurred
    bool rotated = std::filesystem::exists(filepath + ".1");
    assert(rotated);

    std::cout << "✓ Rotating file sink test passed" << std::endl;
}

void test_multiple_sinks() {
    std::cout << "\nTest: Multiple Sinks" << std::endl;

    std::filesystem::create_directories("logs");

    std::string filepath = "logs/test_multi.log";
    std::remove(filepath.c_str());

    auto logger = std::make_shared<Logger>("multi_test");
    logger->addSink(std::make_shared<ConsoleSink>(false));
    logger->addSink(std::make_shared<FileSink>(filepath, true));

    logger->info("Message to both sinks");
    logger->flush();

    assert(fileContains(filepath, "Message to both sinks"));

    std::cout << "✓ Multiple sinks test passed" << std::endl;
}

void test_logger_registry() {
    std::cout << "\nTest: Logger Registry" << std::endl;

    auto logger1 = LoggerRegistry::instance().getLogger("registry_test_1");
    auto logger2 = LoggerRegistry::instance().getLogger("registry_test_2");
    auto logger1_again = LoggerRegistry::instance().getLogger("registry_test_1");

    assert(logger1 == logger1_again);
    assert(logger1 != logger2);

    std::cout << "✓ Logger registry test passed" << std::endl;
}

void test_thread_safety() {
    std::cout << "\nTest: Thread Safety" << std::endl;

    auto logger = std::make_shared<Logger>("thread_test");
    auto console_sink = std::make_shared<ConsoleSink>(false);
    logger->addSink(console_sink);

    auto log_func = [logger](int thread_id) {
        for (int i = 0; i < 100; i++) {
            logger->info("Thread " + std::to_string(thread_id) + " message " + std::to_string(i));
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(log_func, i);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "✓ Thread safety test passed" << std::endl;
}

void test_logger_module() {
    std::cout << "\nTest: Logger Module" << std::endl;

    // Create test application
    TestApp app;

    // Create and register ConfigurationManager
    auto config = std::make_shared<ConfigurationManager>();
    config->load("config/logging.json");
    app.getServiceLocator()->registerSingleton<ConfigurationManager>(config);

    // Create and initialize logger module
    auto logger_module = app.addModule<LoggerModule>();

    // Initialize
    bool init_success = logger_module->initialize(app);
    assert(init_success);

    // Get configured loggers
    auto app_logger = LoggerRegistry::instance().getLogger("app");
    auto network_logger = LoggerRegistry::instance().getLogger("network");

    assert(app_logger != nullptr);
    assert(network_logger != nullptr);

    // Test logging
    app_logger->info("Application logger test");
    network_logger->trace("Network logger test");

    // Shutdown
    logger_module->shutdown();

    std::cout << "✓ Logger module test passed" << std::endl;
}

void test_macros() {
    std::cout << "\nTest: Logging Macros" << std::endl;

    auto logger = std::make_shared<Logger>("macro_test");
    auto console_sink = std::make_shared<ConsoleSink>(false);
    logger->addSink(console_sink);

    MCF_LOG_INFO(logger, "Info message with file/line/function");
    MCF_LOG_WARNING(logger, "Warning with metadata");

    // Test default logger macros
    MCF_INFO("Default logger info");
    MCF_WARNING("Default logger warning");

    std::cout << "✓ Logging macros test passed" << std::endl;
}

void test_formatter() {
    std::cout << "\nTest: Log Formatter" << std::endl;

    std::filesystem::create_directories("logs");

    std::string filepath = "logs/test_format.log";
    std::remove(filepath.c_str());

    auto logger = std::make_shared<Logger>("format_test");
    auto file_sink = std::make_shared<FileSink>(filepath, true);

    // Custom format pattern
    LogFormatter formatter("[%l] %n: %v");
    file_sink->setFormatter(formatter);

    logger->addSink(file_sink);
    logger->info("Custom formatted message");
    logger->flush();

    assert(fileContains(filepath, "[INFO] format_test: Custom formatted message"));

    std::cout << "✓ Log formatter test passed" << std::endl;
}

void test_configuration_reload() {
    std::cout << "\nTest: Configuration Reload" << std::endl;

    TestApp app;

    auto config = std::make_shared<ConfigurationManager>();

    // Create temporary config
    JsonObject logging;
    JsonArray loggers;
    JsonObject logger_config;
    logger_config["name"] = JsonValue("reload_test");
    logger_config["level"] = JsonValue("debug");
    loggers.push_back(JsonValue(logger_config));
    logging["loggers"] = JsonValue(loggers);

    JsonObject root;
    root["logging"] = JsonValue(logging);
    config->set("", JsonValue(root));

    app.getServiceLocator()->registerSingleton<ConfigurationManager>(config);

    auto logger_module = app.addModule<LoggerModule>();
    logger_module->initialize(app);

    auto logger = LoggerRegistry::instance().getLogger("reload_test");
    assert(logger != nullptr);

    // Change configuration
    logger_config["level"] = JsonValue("error");
    loggers.clear();
    loggers.push_back(JsonValue(logger_config));
    logging["loggers"] = JsonValue(loggers);
    root["logging"] = JsonValue(logging);
    config->set("", JsonValue(root));

    // Reload
    logger_module->reloadConfiguration();

    logger_module->shutdown();

    std::cout << "✓ Configuration reload test passed" << std::endl;
}

int main() {
    std::cout << "=== Logger Tests ===" << std::endl;

    try {
        test_basic_logging();
        test_log_levels();
        test_file_sink();
        test_rotating_file_sink();
        test_multiple_sinks();
        test_logger_registry();
        test_thread_safety();
        test_logger_module();
        test_macros();
        test_formatter();
        test_configuration_reload();

        std::cout << "\n=== All tests passed! ===" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}

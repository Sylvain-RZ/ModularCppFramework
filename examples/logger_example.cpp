#include "../core/Application.hpp"
#include "../modules/logger/LoggerModule.hpp"
#include "../core/Logger.hpp"

#include <thread>
#include <chrono>

using namespace mcf;

/**
 * Example application demonstrating the Logger Module
 */
class LoggerExampleApp : public Application {
private:
    std::shared_ptr<Logger> m_appLogger;
    std::shared_ptr<Logger> m_performanceLogger;
    int m_frameCount = 0;

public:
    LoggerExampleApp() : Application(ApplicationConfig()) {}

    bool initialize() override {
        std::cout << "Initializing Logger Example Application...\n" << std::endl;

        // Load configuration
        getConfigurationManager()->load("config/logging.json");

        // Add logger module (high priority = 900)
        auto logger_module = addModule<LoggerModule>();

        // Initialize base application
        if (!Application::initialize()) {
            return false;
        }

        // Get configured loggers
        m_appLogger = LoggerRegistry::instance().getLogger("app");
        m_performanceLogger = LoggerRegistry::instance().getLogger("performance");

        // Log initialization complete
        MCF_LOG_INFO(m_appLogger, "Application initialized successfully");
        m_appLogger->info("Logger module is ready");

        return true;
    }

    void run() override {
        MCF_LOG_INFO(m_appLogger, "Starting main loop");

        // Simulate application loop
        for (int i = 0; i < 10; i++) {
            processFrame(0.016f); // ~60 FPS

            // Simulate frame time
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        MCF_LOG_INFO(m_appLogger, "Main loop completed");
    }

    void processFrame(float deltaTime) {
        m_frameCount++;

        // Log at different levels based on frame
        if (m_frameCount % 10 == 0) {
            m_appLogger->info("Frame " + std::to_string(m_frameCount) + " processed");
        }

        if (m_frameCount % 3 == 0) {
            m_appLogger->debug("Debug: Processing frame " + std::to_string(m_frameCount));
        }

        // Performance logging
        float frameTime = deltaTime * 1000.0f; // Convert to ms
        m_performanceLogger->debug("Frame time: " + std::to_string(frameTime) + "ms");

        // Simulate warnings and errors
        if (m_frameCount == 5) {
            m_appLogger->warning("Warning: High memory usage detected");
        }

        if (m_frameCount == 8) {
            m_appLogger->error("Error: Failed to load texture (simulated)");
        }
    }

    void shutdown() override {
        MCF_LOG_INFO(m_appLogger, "Shutting down application");

        // Flush all logs before shutdown
        LoggerRegistry::instance().flushAll();

        Application::shutdown();
    }
};

/**
 * Demonstrate direct logger usage without module
 */
void demonstrateDirectUsage() {
    std::cout << "\n=== Direct Logger Usage Demo ===" << std::endl;

    // Create a custom logger
    auto logger = std::make_shared<Logger>("demo");

    // Add console sink with colors
    auto console = std::make_shared<ConsoleSink>(true, LogLevel::Trace);
    logger->addSink(console);

    // Add file sink
    try {
        auto file = std::make_shared<FileSink>("logs/demo.log", false, LogLevel::Debug);
        logger->addSink(file);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create file sink: " << e.what() << std::endl;
    }

    // Log at different levels
    logger->trace("This is a trace message");
    logger->debug("This is a debug message");
    logger->info("This is an info message");
    logger->warning("This is a warning message");
    logger->error("This is an error message");
    logger->critical("This is a critical message");

    // Flush
    logger->flush();

    std::cout << "\n✓ Direct usage demo complete" << std::endl;
}

/**
 * Demonstrate thread-safe logging
 */
void demonstrateThreadSafety() {
    std::cout << "\n=== Thread Safety Demo ===" << std::endl;

    auto logger = LoggerRegistry::instance().getLogger("thread_test");
    logger->addSink(std::make_shared<ConsoleSink>(false, LogLevel::Info));

    auto worker = [logger](int id, int count) {
        for (int i = 0; i < count; i++) {
            logger->info("Thread " + std::to_string(id) + ": Message " + std::to_string(i));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 3; i++) {
        threads.emplace_back(worker, i, 5);
    }

    for (auto& t : threads) {
        t.join();
    }

    logger->flush();

    std::cout << "\n✓ Thread safety demo complete" << std::endl;
}

/**
 * Demonstrate different log formatters
 */
void demonstrateFormatters() {
    std::cout << "\n=== Formatter Demo ===" << std::endl;

    auto logger = std::make_shared<Logger>("formatter_test");

    // Console with default format
    auto console1 = std::make_shared<ConsoleSink>(false);
    console1->setFormatter(LogFormatter("[%l] %n: %v"));
    logger->addSink(console1);

    logger->info("Message with simple format");

    // Clear and add new formatter
    logger->clearSinks();

    auto console2 = std::make_shared<ConsoleSink>(false);
    console2->setFormatter(LogFormatter("%l | %v"));
    logger->addSink(console2);

    logger->warning("Warning with minimal format");

    std::cout << "\n✓ Formatter demo complete" << std::endl;
}

/**
 * Demonstrate default logger macros
 */
void demonstrateDefaultLogger() {
    std::cout << "\n=== Default Logger Macros Demo ===" << std::endl;

    MCF_TRACE("This is a trace from default logger");
    MCF_DEBUG("This is a debug from default logger");
    MCF_INFO("This is an info from default logger");
    MCF_WARNING("This is a warning from default logger");
    MCF_ERROR("This is an error from default logger");
    MCF_CRITICAL("This is a critical from default logger");

    std::cout << "\n✓ Default logger demo complete" << std::endl;
}

int main() {
    std::cout << "╔═══════════════════════════════════════╗" << std::endl;
    std::cout << "║   ModularCppFramework Logger Module Demo ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════╝" << std::endl;

    try {
        // Demo 1: Direct usage
        demonstrateDirectUsage();

        // Demo 2: Default logger macros
        demonstrateDefaultLogger();

        // Demo 3: Thread safety
        demonstrateThreadSafety();

        // Demo 4: Formatters
        demonstrateFormatters();

        // Demo 5: Full application with module
        std::cout << "\n=== Application with Logger Module ===" << std::endl;
        LoggerExampleApp app;

        if (!app.initialize()) {
            std::cerr << "Failed to initialize application" << std::endl;
            return 1;
        }

        app.run();
        app.shutdown();

        std::cout << "\n╔═══════════════════════════════════════╗" << std::endl;
        std::cout << "║   All demos completed successfully!  ║" << std::endl;
        std::cout << "╚═══════════════════════════════════════╝" << std::endl;

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}

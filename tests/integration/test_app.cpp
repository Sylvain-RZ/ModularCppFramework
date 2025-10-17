#include "../../core/Application.hpp"
#include "../../core/IModule.hpp"
#include "../../core/EventBus.hpp"

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

using namespace mcf;

// Simple logger module
class LoggerModule : public ModuleBase {
public:
    LoggerModule()
        : ModuleBase("Logger", "1.0.0", 1000) {}  // High priority

    bool initialize(Application& app) override {
        std::cout << "[LoggerModule] Initializing logger..." << std::endl;
        m_initialized = true;
        return true;
    }

    void shutdown() override {
        std::cout << "[LoggerModule] Shutting down logger..." << std::endl;
        m_initialized = false;
    }
};

// Test application
class TestApplication : public Application {
private:
    std::atomic<int> m_startEventCount{0};
    std::atomic<int> m_shutdownEventCount{0};
    EventHandle m_startEventHandle{0};
    EventHandle m_shutdownEventHandle{0};

public:
    explicit TestApplication(const ApplicationConfig& config)
        : Application(config) {
        // Add modules
        addModule<LoggerModule>();
    }

protected:
    bool onInitialize() override {
        std::cout << "=== Test Application Initializing ===" << std::endl;
        std::cout << "App Name: " << m_config.name << std::endl;
        std::cout << "Version: " << m_config.version << std::endl;

        // Subscribe to events - store handles for cleanup
        m_startEventHandle = getEventBus()->subscribe("app.started", [this](const Event& e) {
            m_startEventCount++;
            std::cout << "[EventBus] Received app.started event" << std::endl;
        });

        m_shutdownEventHandle = getEventBus()->subscribe("app.shutdown", [this](const Event& e) {
            m_shutdownEventCount++;
            std::cout << "[EventBus] Received app.shutdown event" << std::endl;
        });

        // Publish startup event
        Event startEvent("app.started");
        getEventBus()->publish("app.started", startEvent);

        return true;
    }

    void onShutdown() override {
        std::cout << "=== Test Application Shutting Down ===" << std::endl;

        // Publish shutdown event
        Event shutdownEvent("app.shutdown");
        getEventBus()->publish("app.shutdown", shutdownEvent);

        // Verify events were received
        std::cout << "[Test] Start events received: " << m_startEventCount << std::endl;
        std::cout << "[Test] Shutdown events received: " << m_shutdownEventCount << std::endl;

        // Unsubscribe from events to prevent dangling references
        if (m_startEventHandle) {
            getEventBus()->unsubscribe(m_startEventHandle);
        }
        if (m_shutdownEventHandle) {
            getEventBus()->unsubscribe(m_shutdownEventHandle);
        }
    }

public:
    void run() override {
        // Initialize if needed
        if (!isInitialized()) {
            if (!initialize()) {
                return;
            }
        }

        m_running = true;

        std::cout << "Application running..." << std::endl;
        std::cout << "Plugins loaded: " << getPluginManager().getPluginCount() << std::endl;

        // For test purposes, just run for a short time
        // Real applications would have their own event loop
    }

    int getStartEventCount() const { return m_startEventCount; }
    int getShutdownEventCount() const { return m_shutdownEventCount; }
};

// Global app pointer for signal handler
TestApplication* g_app = nullptr;

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    if (g_app) {
        g_app->stop();
    }
}

int main(int argc, char** argv) {
    // Setup signal handlers for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "========================================" << std::endl;
    std::cout << "  ModularCppFramework Integration Test" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Configure application
    ApplicationConfig config;
    config.name = "TestApp";
    config.version = "1.0.0";
    config.pluginDirectory = "./plugins";
    config.autoLoadPlugins = true;
    config.autoInitPlugins = true;

    // Create application
    TestApplication app(config);
    g_app = &app;

    int exitCode = 0;

    try {
        std::cout << "Initializing application..." << std::endl;
        if (!app.initialize()) {
            std::cerr << "Failed to initialize application!" << std::endl;
            return 1;
        }

        std::cout << "Application initialized successfully!" << std::endl;
        std::cout << std::endl;

        // Test application lifecycle
        app.run();

        // Small delay to let async operations complete
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Verify test results
        std::cout << "\n=== Test Verification ===" << std::endl;

        bool testsPassed = true;

        if (app.getStartEventCount() != 1) {
            std::cerr << "[FAIL] Expected 1 start event, got " << app.getStartEventCount() << std::endl;
            testsPassed = false;
        } else {
            std::cout << "[PASS] Start event received correctly" << std::endl;
        }

        if (!app.isInitialized()) {
            std::cerr << "[FAIL] Application should be initialized" << std::endl;
            testsPassed = false;
        } else {
            std::cout << "[PASS] Application is initialized" << std::endl;
        }

        // Test plugin system
        size_t pluginCount = app.getPluginManager().getPluginCount();
        std::cout << "[INFO] Loaded " << pluginCount << " plugin(s)" << std::endl;

        // Shutdown
        app.shutdown();

        if (app.isInitialized()) {
            std::cerr << "[FAIL] Application should not be initialized after shutdown" << std::endl;
            testsPassed = false;
        } else {
            std::cout << "[PASS] Application shutdown correctly" << std::endl;
        }

        if (app.getShutdownEventCount() != 1) {
            std::cerr << "[FAIL] Expected 1 shutdown event, got " << app.getShutdownEventCount() << std::endl;
            testsPassed = false;
        } else {
            std::cout << "[PASS] Shutdown event received correctly" << std::endl;
        }

        std::cout << std::endl;

        if (testsPassed) {
            std::cout << "========================================" << std::endl;
            std::cout << "  All tests PASSED!" << std::endl;
            std::cout << "========================================" << std::endl;
        } else {
            std::cout << "========================================" << std::endl;
            std::cout << "  Some tests FAILED!" << std::endl;
            std::cout << "========================================" << std::endl;
            exitCode = 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        exitCode = 1;
    }

    g_app = nullptr;
    return exitCode;
}

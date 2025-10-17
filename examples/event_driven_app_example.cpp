/**
 * @file event_driven_app_example.cpp
 * @brief Example of an event-driven application WITHOUT RealtimeModule
 *
 * This example demonstrates:
 * - Using the framework without real-time updates
 * - Pure event-driven architecture
 * - No FPS, no deltaTime, no game loop
 * - Suitable for: REST APIs, CLI tools, batch processors, services
 */

#include "../core/Application.hpp"
#include "../core/IPlugin.hpp"
#include "../core/IEventDriven.hpp"
#include <iostream>
#include <thread>
#include <chrono>

/**
 * @brief Simple event-driven plugin example
 *
 * This plugin doesn't implement IRealtimeUpdatable because it doesn't
 * need frame-based updates. It's purely event-driven.
 */
class EventDrivenPlugin : public mcf::IPlugin, public mcf::IEventDriven {
private:
    bool m_initialized = false;
    mcf::PluginMetadata m_metadata;
    mcf::PluginContext m_context;
    int m_eventCount = 0;

public:
    EventDrivenPlugin() {
        m_metadata.name = "EventDrivenPlugin";
        m_metadata.version = "1.0.0";
        m_metadata.description = "Example event-driven plugin";
    }

    std::string getName() const override { return m_metadata.name; }
    std::string getVersion() const override { return m_metadata.version; }
    const mcf::PluginMetadata& getMetadata() const override { return m_metadata; }
    bool isInitialized() const override { return m_initialized; }

    const char* getEventDrivenDescription() const override {
        return "Handles custom events via EventBus";
    }

    bool initialize(mcf::PluginContext& context) override {
        if (m_initialized) {
            return true;
        }

        m_context = context;
        std::cout << "[EventDrivenPlugin] Initializing...\n";

        // Subscribe to events (the only way this plugin receives notifications)
        if (m_context.getEventBus()) {
            m_context.getEventBus()->subscribe("task.received",
                [this](const mcf::Event& e) {
                    m_eventCount++;
                    std::cout << "[EventDrivenPlugin] Task received! (Total: "
                              << m_eventCount << ")\n";
                },
                100
            );

            m_context.getEventBus()->subscribe("app.shutdown",
                [this](const mcf::Event& e) {
                    std::cout << "[EventDrivenPlugin] Shutdown event received. "
                              << "Processed " << m_eventCount << " tasks.\n";
                }
            );
        }

        m_initialized = true;
        std::cout << "[EventDrivenPlugin] Ready (event-driven mode)\n";
        return true;
    }

    void shutdown() override {
        if (!m_initialized) {
            return;
        }
        std::cout << "[EventDrivenPlugin] Shutting down...\n";
        m_initialized = false;
    }

    static const char* getManifestJson() {
        return R"({"name": "EventDrivenPlugin", "version": "1.0.0"})";
    }
};

/**
 * @brief Event-driven application (no real-time loop)
 */
class EventDrivenApp : public mcf::Application {
private:
    int m_taskCounter = 0;

public:
    EventDrivenApp() : Application() {
        auto config = mcf::ApplicationConfig();
        config.name = "EventDrivenApp";
        config.version = "1.0.0";
        config.autoLoadPlugins = false; // We'll add our plugin manually
    }

    bool onInitialize() override {
        std::cout << "[EventDrivenApp] Initializing event-driven application...\n";
        std::cout << "[EventDrivenApp] No RealtimeModule = no FPS overhead!\n";
        return true;
    }

    void onShutdown() override {
        std::cout << "[EventDrivenApp] Shutting down...\n";
        mcf::Event shutdownEvent("app.shutdown");
        getEventBus()->publish("app.shutdown", shutdownEvent);
    }

    void run() override {
        if (!isInitialized()) {
            if (!initialize()) {
                std::cerr << "[EventDrivenApp] Failed to initialize!\n";
                return;
            }
        }

        std::cout << "[EventDrivenApp] Running event-driven mode...\n";
        std::cout << "[EventDrivenApp] Simulating periodic task processing\n";
        std::cout << "[EventDrivenApp] Press Ctrl+C to exit\n\n";

        // Simulate an event-driven application that processes tasks
        // (In a real app, this would be: HTTP requests, message queue, etc.)
        while (isRunning()) {
            // Simulate receiving a task every 2 seconds
            std::this_thread::sleep_for(std::chrono::seconds(2));

            m_taskCounter++;
            std::cout << "\n[EventDrivenApp] New task arrived (#" << m_taskCounter << ")\n";

            // Publish task event (our plugin will handle it)
            mcf::Event taskEvent("task.received");
            getEventBus()->publish("task.received", taskEvent);

            // Process event queue
            getEventBus()->processQueue();

            // In a real application, you might:
            // - Wait on a message queue
            // - Handle HTTP requests
            // - Process database queries
            // - React to file system changes
            // etc.

            // Exit after 5 tasks for demo purposes
            if (m_taskCounter >= 5) {
                std::cout << "\n[EventDrivenApp] Demo complete, exiting...\n";
                stop();
            }
        }

        std::cout << "[EventDrivenApp] Event loop exited\n";
    }
};

int main() {
    std::cout << "========================================\n";
    std::cout << "  Event-Driven Application Example\n";
    std::cout << "  (No Real-Time Module)\n";
    std::cout << "========================================\n\n";

    try {
        EventDrivenApp app;

        // Manually add our event-driven plugin
        // (In production, you might still use dynamic loading)
        auto& pluginManager = app.getPluginManager();
        pluginManager.initialize(
            app.getEventBus(),
            app.getServiceLocator(),
            &app,
            app.getResourceManager(),
            nullptr, // No thread pool needed for this example
            app.getConfigurationManager()
        );

        // Note: In a real scenario, you'd load plugins dynamically
        // This is just for demonstration

        app.run();

        std::cout << "\n[Main] Application exited successfully\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[Main] Fatal error: " << e.what() << "\n";
        return 1;
    }
}

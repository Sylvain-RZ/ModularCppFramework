/**
 * @file realtime_app_example.cpp
 * @brief Example of a real-time application using RealtimeModule
 *
 * This example demonstrates:
 * - Using RealtimeModule for frame-based updates
 * - Creating plugins that implement IRealtimeUpdatable
 * - FPS tracking and delta time
 * - Clean separation between core framework and real-time concerns
 */

#include "../core/Application.hpp"
#include "../modules/realtime/RealtimeModule.hpp"
#include <iostream>
#include <csignal>

// Global pointer for signal handling
mcf::Application* g_app = nullptr;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n[App] Received interrupt signal, shutting down...\n";
        if (g_app) {
            g_app->stop();
        }
    }
}

/**
 * @brief Real-time application example
 */
class RealtimeApp : public mcf::Application {
private:
    mcf::RealtimeModule* m_realtimeModule = nullptr;

public:
    RealtimeApp() : Application() {
        auto config = mcf::ApplicationConfig();
        config.name = "RealtimeApp";
        config.version = "1.0.0";
        config.pluginDirectory = "./plugins";
        config.autoLoadPlugins = true;
        config.autoInitPlugins = true;

        // Add RealtimeModule BEFORE initialization
        mcf::RealtimeConfig rtConfig;
        rtConfig.targetFPS = 60;
        rtConfig.printFPS = true;
        rtConfig.fpsUpdateInterval = 2.0f; // Print FPS every 2 seconds
        m_realtimeModule = addModule<mcf::RealtimeModule>(rtConfig);
    }

    bool onInitialize() override {
        std::cout << "[RealtimeApp] Initializing real-time application...\n";
        std::cout << "[RealtimeApp] RealtimeModule configured with target FPS: 60\n";

        // Publish startup event
        mcf::Event startEvent("app.started");
        getEventBus()->publish("app.started", startEvent);

        return true;
    }

    void onShutdown() override {
        std::cout << "[RealtimeApp] Shutting down real-time application...\n";

        // Publish shutdown event
        mcf::Event shutdownEvent("app.shutdown");
        getEventBus()->publish("app.shutdown", shutdownEvent);
    }

    void run() override {
        // Initialize if needed
        if (!isInitialized()) {
            if (!initialize()) {
                std::cerr << "[RealtimeApp] Failed to initialize!\n";
                return;
            }
        }

        std::cout << "[RealtimeApp] Starting real-time loop...\n";
        std::cout << "[RealtimeApp] Press Ctrl+C to exit\n";

        // Run the realtime loop
        if (m_realtimeModule) {
            m_realtimeModule->run();
        }

        std::cout << "[RealtimeApp] Real-time loop exited\n";
    }
};

int main(int argc, char** argv) {
    std::cout << "===================================\n";
    std::cout << "  Real-Time Application Example\n";
    std::cout << "===================================\n\n";

    // Setup signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    try {
        // Create application
        RealtimeApp app;
        g_app = &app;

        // Run application
        app.run();

        // Cleanup
        g_app = nullptr;

        std::cout << "\n[Main] Application exited successfully\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[Main] Fatal error: " << e.what() << "\n";
        return 1;
    }
}

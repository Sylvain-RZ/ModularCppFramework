/**
 * @file hot_reload_demo.cpp
 * @brief Demonstration of plugin hot reloading functionality
 *
 * This example shows how to:
 * - Enable hot reload monitoring
 * - Load plugins with hot reload support
 * - Trigger manual reloads
 * - Watch for automatic file changes
 *
 * To test hot reload:
 * 1. Run this program
 * 2. Modify plugins/hot_reload_example/HotReloadExamplePlugin.cpp
 *    (e.g., change the m_message string)
 * 3. Rebuild: cd build && make hot_reload_example
 * 4. Watch the plugin reload automatically!
 */

#include "../core/Application.hpp"
#include "../core/IRealtimeUpdatable.hpp"
#include "../modules/realtime/RealtimeModule.hpp"
#include "../core/EventBus.hpp"
#include <iostream>
#include <csignal>

// Global pointer for signal handling
mcf::Application* g_app = nullptr;

void signalHandler(int signal) {
    if (signal == SIGINT && g_app) {
        std::cout << "\n[HotReloadDemo] Caught SIGINT, shutting down..." << std::endl;
        g_app->stop();
    }
}

class HotReloadDemoApp : public mcf::Application, public mcf::IRealtimeUpdatable {
private:
    bool m_enableManualReload = false;
    float m_reloadTimer = 0.0f;
    mcf::RealtimeModule* m_realtimeModule = nullptr;

public:
    HotReloadDemoApp() : Application(createConfig()) {}

    static mcf::ApplicationConfig createConfig() {
        mcf::ApplicationConfig config;
        config.name = "Hot Reload Demo";
        config.version = "1.0.0";
        config.pluginDirectory = "./plugins";
        config.autoLoadPlugins = true;
        config.autoInitPlugins = true;
        return config;
    }

protected:
    bool onInitialize() override {
        std::cout << "=== Hot Reload Demo ===" << std::endl;
        std::cout << "This demo shows plugin hot reloading in action." << std::endl;
        std::cout << std::endl;
        std::cout << "Instructions:" << std::endl;
        std::cout << "1. The HotReloadExample plugin is now running" << std::endl;
        std::cout << "2. Edit plugins/hot_reload_example/HotReloadExamplePlugin.cpp" << std::endl;
        std::cout << "3. Change the m_message string to something new" << std::endl;
        std::cout << "4. Rebuild: cd build && make hot_reload_example" << std::endl;
        std::cout << "5. Watch the plugin reload automatically!" << std::endl;
        std::cout << std::endl;
        std::cout << "The plugin state (counter, reload count) will persist across reloads." << std::endl;
        std::cout << "Press Ctrl+C to exit." << std::endl;
        std::cout << "=========================" << std::endl;
        std::cout << std::endl;

        // Add RealtimeModule for frame-based updates
        mcf::RealtimeConfig rtConfig;
        rtConfig.targetFPS = 60;
        rtConfig.printFPS = true;
        rtConfig.fpsUpdateInterval = 5.0f;
        m_realtimeModule = addModule<mcf::RealtimeModule>(rtConfig);

        // Enable hot reload with 1-second polling interval
        m_pluginManager.enableHotReload(std::chrono::milliseconds(1000));

        std::cout << "[HotReloadDemo] Hot reload enabled!" << std::endl;
        std::cout << "[HotReloadDemo] Watching for plugin file changes..." << std::endl;

        return true;
    }

    void onShutdown() override {
        std::cout << "[HotReloadDemo] Disabling hot reload..." << std::endl;
        m_pluginManager.disableHotReload();

        std::cout << "[HotReloadDemo] Shutdown complete." << std::endl;
    }

    // IRealtimeUpdatable implementation
    void onRealtimeUpdate(float deltaTime) override {
        // Optional: Trigger manual reload every 30 seconds for testing
        if (m_enableManualReload) {
            m_reloadTimer += deltaTime;

            if (m_reloadTimer >= 30.0f) {
                std::cout << "[HotReloadDemo] Triggering manual reload..." << std::endl;
                m_pluginManager.reloadPlugin("HotReloadExample");
                m_reloadTimer = 0.0f;
            }
        }
    }

public:
    void run() override {
        if (!isInitialized()) {
            if (!initialize()) {
                std::cerr << "[HotReloadDemo] Initialization failed!" << std::endl;
                return;
            }
        }

        // Run the realtime loop
        if (m_realtimeModule) {
            m_realtimeModule->run();
        }
    }
};

int main() {
    try {
        // Setup signal handler for graceful shutdown
        std::signal(SIGINT, signalHandler);

        // Create and run application
        HotReloadDemoApp app;
        g_app = &app;

        if (!app.initialize()) {
            std::cerr << "[HotReloadDemo] Failed to initialize application!" << std::endl;
            return 1;
        }

        std::cout << "[HotReloadDemo] Application initialized successfully!" << std::endl;
        std::cout << "[HotReloadDemo] Running main loop..." << std::endl;

        app.run();

        std::cout << "[HotReloadDemo] Application exited normally." << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[HotReloadDemo] Exception: " << e.what() << std::endl;
        return 1;
    }
}

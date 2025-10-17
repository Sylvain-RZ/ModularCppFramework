/**
 * @file profiling_example.cpp
 * @brief Example demonstrating the profiling and metrics system
 *
 * This example shows:
 * - How to enable profiling
 * - Using ProfilingModule for automatic frame profiling
 * - Manual profiling with macros (MCF_PROFILE_SCOPE, etc.)
 * - Collecting counters, gauges, and timings
 * - Auto-export of metrics to JSON/CSV
 * - Zero overhead when profiling is disabled
 */

#include "../core/Application.hpp"
#include "../modules/realtime/RealtimeModule.hpp"
#include "../modules/profiling/ProfilingModule.hpp"
#include "../modules/profiling/ProfilingMacros.hpp"
#include <iostream>
#include <csignal>
#include <random>
#include <thread>

// Global pointer for signal handling
mcf::Application* g_app = nullptr;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n[ProfilingExample] Received interrupt signal, shutting down...\n";
        if (g_app) {
            g_app->stop();
        }
    }
}

/**
 * @brief Simulate some work with variable duration
 */
void simulateWork(const std::string& workName, double minMs, double maxMs) {
    MCF_PROFILE_SCOPE(workName);  // Auto-profile this function

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(minMs, maxMs);

    double duration = dis(gen);
    std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int>(duration * 1000)));

    // Increment work counter
    MCF_PROFILE_INCREMENT("work_items_processed");
}

/**
 * @brief Simulate database query
 */
void queryDatabase() {
    MCF_PROFILE_FUNCTION();  // Auto-profile entire function

    simulateWork("db_query", 0.5, 5.0);

    MCF_PROFILE_INCREMENT("database_queries");
}

/**
 * @brief Simulate API call
 */
void callExternalAPI() {
    MCF_PROFILE_FUNCTION();

    simulateWork("api_call", 1.0, 10.0);

    MCF_PROFILE_COUNTER("api_calls_total", 1.0);
}

/**
 * @brief Example application with profiling enabled
 */
class ProfilingExampleApp : public mcf::Application {
private:
    mcf::RealtimeModule* m_realtimeModule = nullptr;
    mcf::ProfilingModule* m_profilingModule = nullptr;

    int m_frameCounter = 0;
    std::random_device m_rd;
    std::mt19937 m_gen;

public:
    ProfilingExampleApp() : m_gen(m_rd()) {
        // Configure profiling
        auto profilingConfig = mcf::ProfilingConfig::createDevelopment();
        profilingConfig.enabled = true;
        profilingConfig.profileFrames = true;
        profilingConfig.timingThresholdMs = 0.1;  // Only record timings > 0.1ms
        profilingConfig.autoExportEnabled = true;
        profilingConfig.autoExportIntervalSeconds = 10.0;  // Export every 10 seconds
        profilingConfig.exportFormat = "json";
        profilingConfig.exportPath = "./metrics";

        // Add modules BEFORE initialization
        m_profilingModule = addModule<mcf::ProfilingModule>(profilingConfig);

        // Configure realtime
        mcf::RealtimeConfig rtConfig;
        rtConfig.targetFPS = 30;  // Lower FPS for demo (easier to see metrics)
        rtConfig.printFPS = true;
        rtConfig.fpsUpdateInterval = 5.0f;

        m_realtimeModule = addModule<mcf::RealtimeModule>(rtConfig);
    }

    bool onInitialize() override {
        std::cout << "========================================\n";
        std::cout << "  Profiling & Metrics Example\n";
        std::cout << "========================================\n\n";
        std::cout << "[ProfilingExample] Initializing...\n";

        #ifdef MCF_PROFILING_ENABLED
        std::cout << "[ProfilingExample] Profiling macros: ENABLED\n";
        #else
        std::cout << "[ProfilingExample] Profiling macros: DISABLED (zero overhead)\n";
        #endif

        std::cout << "[ProfilingExample] Press Ctrl+C to exit and see final statistics\n\n";

        return true;
    }

    void onShutdown() override {
        std::cout << "\n[ProfilingExample] Shutting down...\n";
    }

    void run() override {
        if (!isInitialized()) {
            if (!initialize()) {
                std::cerr << "[ProfilingExample] Failed to initialize!\n";
                return;
            }
        }

        // Custom update loop with profiling
        std::cout << "[ProfilingExample] Starting profiled application loop...\n\n";

        while (isRunning()) {
            MCF_PROFILE_SCOPE("main_loop_iteration");

            m_frameCounter++;

            // Simulate various operations
            {
                MCF_PROFILE_SCOPE("business_logic");

                // Simulate some calculations
                if (m_frameCounter % 5 == 0) {
                    queryDatabase();
                }

                if (m_frameCounter % 7 == 0) {
                    callExternalAPI();
                }

                // Record some metrics
                std::uniform_int_distribution<> memDis(50, 150);
                int memoryUsage = memDis(m_gen);
                MCF_PROFILE_GAUGE_CAT("memory_usage_mb", memoryUsage, "memory");

                std::uniform_int_distribution<> cpuDis(10, 90);
                int cpuUsage = cpuDis(m_gen);
                MCF_PROFILE_GAUGE_CAT("cpu_usage_percent", cpuUsage, "system");
            }

            // Update profiling module for auto-export and frame profiling
            if (m_profilingModule) {
                MCF_PROFILE_START(profilingTimer, "profiling_module_update");

                float deltaTime = 1.0f / 30.0f;  // Approximate
                m_profilingModule->onRealtimeUpdate(deltaTime);

                MCF_PROFILE_END(profilingTimer);
            }

            // Sleep to simulate ~30 FPS
            std::this_thread::sleep_for(std::chrono::milliseconds(33));

            // Stop after 100 frames for demo
            if (m_frameCounter >= 100) {
                std::cout << "\n[ProfilingExample] Reached 100 frames, stopping...\n";
                stop();
            }
        }

        std::cout << "[ProfilingExample] Loop exited\n";
    }
};

int main() {
    // Setup signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    try {
        // Create application
        ProfilingExampleApp app;
        g_app = &app;

        // Run application
        app.run();

        // Cleanup
        g_app = nullptr;

        std::cout << "\n[Main] Application exited successfully\n";
        std::cout << "Check ./metrics/ directory for exported metrics\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[Main] Fatal error: " << e.what() << "\n";
        return 1;
    }
}

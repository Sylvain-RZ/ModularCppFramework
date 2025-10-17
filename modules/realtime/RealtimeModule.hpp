#pragma once

#include "../../core/IModule.hpp"
#include "../../core/IRealtimeUpdatable.hpp"
#include "../../core/IPlugin.hpp"
#include <chrono>
#include <thread>
#include <vector>
#include <memory>

namespace mcf {

// Forward declaration
class Application;
class PluginManager;

/**
 * @brief Configuration for realtime module
 */
struct RealtimeConfig {
    int targetFPS = 60;           // Target frames per second (0 = unlimited)
    bool vsync = false;            // VSync enabled (requires platform support)
    bool printFPS = false;         // Print FPS to console periodically
    float fpsUpdateInterval = 1.0f; // How often to update FPS counter (seconds)
};

/**
 * @brief Module that provides real-time update loop functionality
 *
 * This optional module adds game-loop style functionality to the framework:
 * - Frame-based updates with delta time
 * - FPS tracking and limiting
 * - Automatic detection and updating of IRealtimeUpdatable components
 *
 * Use this module when building:
 * - Games
 * - Real-time simulations
 * - Interactive visualizations
 * - Applications with animations
 *
 * Don't use this module for:
 * - REST APIs / web services
 * - CLI tools
 * - Batch processing applications
 * - Pure event-driven systems
 *
 * Usage:
 * @code
 * class MyApp : public mcf::Application {
 *     void setup() {
 *         mcf::RealtimeConfig config;
 *         config.targetFPS = 60;
 *         addModule<mcf::RealtimeModule>(config);
 *     }
 * };
 * @endcode
 */
class RealtimeModule : public ModuleBase {
private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    RealtimeConfig m_config;
    Application* m_app = nullptr;
    PluginManager* m_pluginManager = nullptr;

    // Time tracking
    TimePoint m_lastFrameTime;
    float m_deltaTime = 0.0f;
    float m_fps = 0.0f;
    int m_frameCount = 0;
    float m_fpsUpdateTimer = 0.0f;

    // Running state
    bool m_running = false;
    bool m_paused = false;

    // Cached references to updatable components
    std::vector<IRealtimeUpdatable*> m_updatableModules;
    std::vector<IRealtimeUpdatable*> m_updatablePlugins;
    bool m_cacheNeedsRefresh = true;

public:
    explicit RealtimeModule(const RealtimeConfig& config = RealtimeConfig())
        : ModuleBase("RealtimeModule", "1.0.0", 50) // Lower priority = runs after most modules
        , m_config(config) {}

    bool initialize(Application& app) override;
    void shutdown() override;

    /**
     * @brief Start the realtime loop
     *
     * This blocks until stop() is called. Typically called from
     * your application's run() method.
     */
    void run();

    /**
     * @brief Stop the realtime loop
     */
    void stop() { m_running = false; }

    /**
     * @brief Pause updates (continues running but skips frame processing)
     */
    void pause() { m_paused = true; }

    /**
     * @brief Resume updates after pause
     */
    void resume() {
        m_paused = false;
        m_lastFrameTime = Clock::now(); // Reset to avoid large delta
    }

    /**
     * @brief Check if currently running
     */
    bool isRunning() const { return m_running; }

    /**
     * @brief Check if currently paused
     */
    bool isPaused() const { return m_paused; }

    /**
     * @brief Get current delta time in seconds
     */
    float getDeltaTime() const { return m_deltaTime; }

    /**
     * @brief Get current frames per second
     */
    float getFPS() const { return m_fps; }

    /**
     * @brief Get realtime configuration
     */
    const RealtimeConfig& getConfig() const { return m_config; }

    /**
     * @brief Update configuration (e.g., change target FPS at runtime)
     */
    void setConfig(const RealtimeConfig& config) { m_config = config; }

    /**
     * @brief Manually refresh the cache of updatable components
     *
     * Called automatically when plugins are loaded/unloaded.
     * Can be called manually if you add modules dynamically.
     */
    void refreshUpdatableCache();

private:
    /**
     * @brief Process a single frame
     */
    void processFrame(float deltaTime);

    /**
     * @brief Update FPS counter
     */
    void updateFPS();
};

} // namespace mcf

#include "RealtimeModule.hpp"
#include "../../core/Application.hpp"
#include "../../core/PluginManager.hpp"
#include <iostream>

namespace mcf {

bool RealtimeModule::initialize(Application& app) {
    if (m_initialized) {
        return true;
    }

    m_app = &app;
    m_pluginManager = &app.getPluginManager();
    m_lastFrameTime = Clock::now();

    // Register callbacks for plugin load/unload to refresh cache
    // Note: This would require adding callback support to PluginManager
    // For now, we'll refresh cache at the start of each frame

    m_initialized = true;
    return true;
}

void RealtimeModule::shutdown() {
    if (!m_initialized) {
        return;
    }

    if (m_running) {
        stop();
    }

    m_app = nullptr;
    m_pluginManager = nullptr;
    m_updatableModules.clear();
    m_updatablePlugins.clear();

    m_initialized = false;
}

void RealtimeModule::run() {
    if (!m_initialized) {
        std::cerr << "RealtimeModule: Cannot run, module not initialized\n";
        return;
    }

    m_running = true;
    m_lastFrameTime = Clock::now();

    while (m_running) {
        // Calculate delta time
        auto currentTime = Clock::now();
        std::chrono::duration<float> elapsed = currentTime - m_lastFrameTime;
        m_deltaTime = elapsed.count();
        m_lastFrameTime = currentTime;

        // Update FPS counter
        updateFPS();

        // Process frame
        if (!m_paused) {
            processFrame(m_deltaTime);
        }

        // Frame rate limiting
        if (m_config.targetFPS > 0 && !m_config.vsync) {
            float targetFrameTime = 1.0f / m_config.targetFPS;
            if (m_deltaTime < targetFrameTime) {
                auto sleepDuration = std::chrono::duration<float>(
                    targetFrameTime - m_deltaTime
                );
                std::this_thread::sleep_for(sleepDuration);
            }
        }
    }
}

void RealtimeModule::processFrame(float deltaTime) {
    // Refresh cache if needed (e.g., plugins were loaded/unloaded)
    if (m_cacheNeedsRefresh) {
        refreshUpdatableCache();
    }

    // Update all realtime-updatable modules
    for (auto* updatable : m_updatableModules) {
        updatable->onRealtimeUpdate(deltaTime);
    }

    // Update all realtime-updatable plugins
    for (auto* updatable : m_updatablePlugins) {
        updatable->onRealtimeUpdate(deltaTime);
    }
}

void RealtimeModule::refreshUpdatableCache() {
    m_updatableModules.clear();
    m_updatablePlugins.clear();

    // Scan modules for IRealtimeUpdatable
    if (m_app) {
        // Note: This requires adding a method to Application to iterate modules
        // For now, we'll document this limitation
        // TODO: Add Application::getModules() or Application::forEachModule()
    }

    // Scan plugins for IRealtimeUpdatable
    if (m_pluginManager) {
        auto plugins = m_pluginManager->getLoadedPlugins();
        for (const auto& pluginName : plugins) {
            auto* plugin = m_pluginManager->getPlugin(pluginName);
            if (plugin && plugin->isInitialized()) {
                // Try to cast to IRealtimeUpdatable
                if (auto* updatable = dynamic_cast<IRealtimeUpdatable*>(plugin)) {
                    m_updatablePlugins.push_back(updatable);
                }
            }
        }
    }

    m_cacheNeedsRefresh = false;
}

void RealtimeModule::updateFPS() {
    m_frameCount++;
    m_fpsUpdateTimer += m_deltaTime;

    if (m_fpsUpdateTimer >= m_config.fpsUpdateInterval) {
        m_fps = m_frameCount / m_fpsUpdateTimer;

        if (m_config.printFPS) {
            std::cout << "FPS: " << static_cast<int>(m_fps) << "\n";
        }

        m_frameCount = 0;
        m_fpsUpdateTimer = 0.0f;
    }
}

} // namespace mcf

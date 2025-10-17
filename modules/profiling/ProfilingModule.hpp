#pragma once

#include "../../core/IModule.hpp"
#include "../../core/IRealtimeUpdatable.hpp"
#include "MetricsCollector.hpp"
#include "ProfilingConfig.hpp"
#include <chrono>
#include <string>

namespace mcf {

// Forward declarations
class Application;
class ConfigurationManager;

/**
 * @brief Module that integrates profiling into the framework
 *
 * Features:
 * - Automatic frame profiling when used with RealtimeModule
 * - Periodic metric export
 * - Integration with ConfigurationManager for JSON config
 * - Zero overhead when disabled
 *
 * Usage:
 * @code
 * ProfilingConfig config = ProfilingConfig::createDevelopment();
 * app.addModule<ProfilingModule>(config);
 * @endcode
 */
class ProfilingModule : public ModuleBase, public IRealtimeUpdatable {
private:
    ProfilingConfig m_config;
    Application* m_app = nullptr;
    ConfigurationManager* m_configManager = nullptr;

    // Auto-export timer
    float m_exportTimer = 0.0f;
    uint64_t m_exportCount = 0;

    // Frame profiling
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;
    uint64_t m_frameCount = 0;

public:
    explicit ProfilingModule(const ProfilingConfig& config = ProfilingConfig::createDefault())
        : ModuleBase("ProfilingModule", "1.0.0", 10)  // High priority (early init)
        , m_config(config) {}

    bool initialize(Application& app) override;
    void shutdown() override;

    // IRealtimeUpdatable - for frame profiling and auto-export
    void onRealtimeUpdate(float deltaTime) override;

    /**
     * @brief Load configuration from ConfigurationManager
     */
    void loadConfigFromJson();

    /**
     * @brief Save configuration to ConfigurationManager
     */
    void saveConfigToJson();

    /**
     * @brief Export metrics now
     */
    void exportMetrics();

    /**
     * @brief Get profiling configuration
     */
    const ProfilingConfig& getConfig() const { return m_config; }

    /**
     * @brief Update configuration at runtime
     */
    void setConfig(const ProfilingConfig& config);

    /**
     * @brief Get metrics collector instance
     */
    MetricsCollector& getCollector() { return MetricsCollector::getInstance(); }

    /**
     * @brief Print current statistics to console
     */
    void printStatistics();

private:
    std::string generateExportFilename() const;
};

} // namespace mcf

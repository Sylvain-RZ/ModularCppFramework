#include "ProfilingModule.hpp"
#include "../../core/Application.hpp"
#include "../../core/ConfigurationManager.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <sys/stat.h>

namespace mcf {

bool ProfilingModule::initialize(Application& app) {
    if (m_initialized) {
        return true;
    }

    m_app = &app;
    m_configManager = app.getConfigurationManager();

    // Try to load config from JSON
    if (m_configManager) {
        loadConfigFromJson();
    }

    // Initialize the metrics collector
    MetricsCollector::getInstance().initialize(m_config);

    if (m_config.enabled) {
        std::cout << "[ProfilingModule] Profiling ENABLED\n";
        std::cout << "[ProfilingModule] Counters: " << (m_config.enableCounters ? "ON" : "OFF") << "\n";
        std::cout << "[ProfilingModule] Gauges: " << (m_config.enableGauges ? "ON" : "OFF") << "\n";
        std::cout << "[ProfilingModule] Timings: " << (m_config.enableTimings ? "ON" : "OFF") << "\n";
        std::cout << "[ProfilingModule] Auto-export: " << (m_config.autoExportEnabled ? "ON" : "OFF") << "\n";

        if (m_config.autoExportEnabled) {
            std::cout << "[ProfilingModule] Export interval: "
                      << m_config.autoExportIntervalSeconds << "s\n";
            std::cout << "[ProfilingModule] Export path: " << m_config.exportPath << "\n";

            // Create export directory if it doesn't exist
#ifdef _WIN32
            mkdir(m_config.exportPath.c_str());
#else
            mkdir(m_config.exportPath.c_str(), 0755);
#endif
        }
    } else {
        std::cout << "[ProfilingModule] Profiling DISABLED (zero overhead)\n";
    }

    m_lastFrameTime = std::chrono::high_resolution_clock::now();
    m_initialized = true;
    return true;
}

void ProfilingModule::shutdown() {
    if (!m_initialized) {
        return;
    }

    // Final export before shutdown
    if (m_config.enabled && m_config.autoExportEnabled) {
        std::cout << "[ProfilingModule] Final metrics export...\n";
        exportMetrics();
    }

    // Print final statistics
    if (m_config.enabled) {
        printStatistics();
    }

    m_initialized = false;
}

void ProfilingModule::onRealtimeUpdate(float deltaTime) {
    if (!m_config.enabled) {
        return;
    }

    // Profile frame time
    if (m_config.profileFrames) {
        auto now = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(
            now - m_lastFrameTime
        ).count();
        double frameMs = frameDuration / 1000.0;

        MetricsCollector::getInstance().recordTiming(
            "frame_time",
            frameMs,
            "performance",
            "ms"
        );

        m_lastFrameTime = now;
        m_frameCount++;

        // Also record FPS as a gauge
        if (frameMs > 0.0) {
            double fps = 1000.0 / frameMs;
            MetricsCollector::getInstance().recordGauge(
                "fps",
                fps,
                "performance",
                "fps"
            );
        }
    }

    // Auto-export timer
    if (m_config.autoExportEnabled) {
        m_exportTimer += deltaTime;

        if (m_exportTimer >= m_config.autoExportIntervalSeconds) {
            exportMetrics();
            m_exportTimer = 0.0f;
        }
    }
}

void ProfilingModule::loadConfigFromJson() {
    if (!m_configManager) {
        return;
    }

    // Load profiling configuration from JSON
    if (m_configManager->has("profiling.enabled")) {
        m_config.enabled = m_configManager->getBool("profiling.enabled");
    }

    if (m_configManager->has("profiling.enableCounters")) {
        m_config.enableCounters = m_configManager->getBool("profiling.enableCounters");
    }

    if (m_configManager->has("profiling.enableGauges")) {
        m_config.enableGauges = m_configManager->getBool("profiling.enableGauges");
    }

    if (m_configManager->has("profiling.enableTimings")) {
        m_config.enableTimings = m_configManager->getBool("profiling.enableTimings");
    }

    if (m_configManager->has("profiling.timingThresholdMs")) {
        // ConfigurationManager doesn't have getDouble, use getInt and cast
        m_config.timingThresholdMs = static_cast<double>(m_configManager->getInt("profiling.timingThresholdMs"));
    }

    if (m_configManager->has("profiling.autoExportEnabled")) {
        m_config.autoExportEnabled = m_configManager->getBool("profiling.autoExportEnabled");
    }

    if (m_configManager->has("profiling.autoExportIntervalSeconds")) {
        // ConfigurationManager doesn't have getDouble, use getInt and cast
        m_config.autoExportIntervalSeconds = static_cast<double>(m_configManager->getInt("profiling.autoExportIntervalSeconds"));
    }

    if (m_configManager->has("profiling.exportPath")) {
        m_config.exportPath = m_configManager->getString("profiling.exportPath");
    }

    if (m_configManager->has("profiling.exportFormat")) {
        m_config.exportFormat = m_configManager->getString("profiling.exportFormat");
    }

    if (m_configManager->has("profiling.profileFrames")) {
        m_config.profileFrames = m_configManager->getBool("profiling.profileFrames");
    }
}

void ProfilingModule::saveConfigToJson() {
    if (!m_configManager) {
        return;
    }

    m_configManager->set("profiling.enabled", m_config.enabled);
    m_configManager->set("profiling.enableCounters", m_config.enableCounters);
    m_configManager->set("profiling.enableGauges", m_config.enableGauges);
    m_configManager->set("profiling.enableTimings", m_config.enableTimings);
    m_configManager->set("profiling.timingThresholdMs", m_config.timingThresholdMs);
    m_configManager->set("profiling.autoExportEnabled", m_config.autoExportEnabled);
    m_configManager->set("profiling.autoExportIntervalSeconds", m_config.autoExportIntervalSeconds);
    m_configManager->set("profiling.exportPath", m_config.exportPath);
    m_configManager->set("profiling.exportFormat", m_config.exportFormat);
    m_configManager->set("profiling.profileFrames", m_config.profileFrames);
}

std::string ProfilingModule::generateExportFilename() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << m_config.exportPath;
    if (!m_config.exportPath.empty() && m_config.exportPath.back() != '/') {
        oss << "/";
    }
    oss << "metrics_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");

    if (m_config.exportFormat == "json") {
        oss << ".json";
    } else if (m_config.exportFormat == "csv") {
        oss << ".csv";
    } else if (m_config.exportFormat == "stats") {
        oss << "_stats.json";
    }

    return oss.str();
}

void ProfilingModule::exportMetrics() {
    auto& collector = MetricsCollector::getInstance();

    std::string filename = generateExportFilename();

    bool success = collector.saveToFile(filename, m_config.exportFormat);

    if (success) {
        m_exportCount++;
        std::cout << "[ProfilingModule] Exported metrics #" << m_exportCount
                  << " to: " << filename << "\n";

        // Also export statistics
        std::string statsFilename = filename;
        size_t dotPos = statsFilename.find_last_of('.');
        if (dotPos != std::string::npos) {
            statsFilename = statsFilename.substr(0, dotPos) + "_stats.json";
        } else {
            statsFilename += "_stats.json";
        }

        collector.saveToFile(statsFilename, "stats");
    } else {
        std::cerr << "[ProfilingModule] Failed to export metrics to: " << filename << "\n";
    }
}

void ProfilingModule::setConfig(const ProfilingConfig& config) {
    m_config = config;
    MetricsCollector::getInstance().initialize(config);

    if (m_configManager) {
        saveConfigToJson();
    }
}

void ProfilingModule::printStatistics() {
    std::cout << "\n[ProfilingModule] ===== Final Statistics =====\n";
    std::cout << "[ProfilingModule] Frames profiled: " << m_frameCount << "\n";
    std::cout << "[ProfilingModule] Exports: " << m_exportCount << "\n";

    MetricsCollector::getInstance().printToConsole();
}

} // namespace mcf

#pragma once

#include <string>
#include <vector>
#include <unordered_set>

namespace mcf {

/**
 * @brief Configuration for profiling system
 *
 * Can be loaded from JSON configuration file
 */
struct ProfilingConfig {
    // Global enable/disable
    bool enabled = false;

    // Enable specific metric types
    bool enableCounters = true;
    bool enableGauges = true;
    bool enableTimings = true;
    bool enableHistograms = true;

    // Timing settings
    bool autoProfileFunctions = false;  // Auto-profile all function calls (expensive!)
    double timingThresholdMs = 0.0;     // Only record timings above this threshold

    // Histogram settings
    uint32_t histogramBuckets = 10;
    double histogramMinValue = 0.0;
    double histogramMaxValue = 100.0;

    // Sampling (for high-frequency metrics)
    bool enableSampling = false;
    uint32_t sampleRate = 100;  // Record 1 out of N samples

    // Memory limits
    uint64_t maxMetricsInMemory = 10000;  // Prevent memory explosion
    bool autoFlushWhenFull = true;

    // Export settings
    bool autoExportEnabled = false;
    double autoExportIntervalSeconds = 60.0;
    std::string exportFormat = "json";  // "json", "csv", "console"
    std::string exportPath = "./metrics/";

    // Category filters (empty = all categories)
    std::unordered_set<std::string> enabledCategories;  // If empty, all enabled
    std::unordered_set<std::string> disabledCategories;

    // Thread-safety
    bool threadSafe = true;  // Use locks (disable for single-threaded apps)

    // Integration with RealtimeModule
    bool profileFrames = false;      // Profile each frame's duration
    bool profileModuleUpdates = false; // Profile each module's update
    bool profilePluginUpdates = false; // Profile each plugin's update

    /**
     * @brief Check if a category is enabled
     */
    bool isCategoryEnabled(const std::string& category) const {
        // If in disabled list, reject
        if (disabledCategories.count(category) > 0) {
            return false;
        }

        // If enabled list is empty, allow all
        if (enabledCategories.empty()) {
            return true;
        }

        // Check if in enabled list
        return enabledCategories.count(category) > 0;
    }

    /**
     * @brief Load configuration from JSON-like structure
     * (Actual JSON parsing would use a JSON library)
     */
    static ProfilingConfig createDefault() {
        ProfilingConfig config;
        config.enabled = false;  // Disabled by default for production
        return config;
    }

    static ProfilingConfig createDevelopment() {
        ProfilingConfig config;
        config.enabled = true;
        config.autoExportEnabled = true;
        config.autoExportIntervalSeconds = 30.0;
        config.profileFrames = true;
        config.profileModuleUpdates = true;
        config.profilePluginUpdates = true;
        return config;
    }

    static ProfilingConfig createProduction() {
        ProfilingConfig config;
        config.enabled = false;  // Completely disabled
        return config;
    }
};

} // namespace mcf

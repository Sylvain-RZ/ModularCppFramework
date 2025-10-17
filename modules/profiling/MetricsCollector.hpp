#pragma once

#include "ProfilingTypes.hpp"
#include "ProfilingConfig.hpp"
#include <mutex>
#include <unordered_map>
#include <vector>
#include <memory>
#include <atomic>

namespace mcf {

/**
 * @brief Thread-safe metrics collection engine
 *
 * Singleton pattern for global access.
 * Zero overhead when profiling is disabled at compile time.
 */
class MetricsCollector {
private:
    ProfilingConfig m_config;
    mutable std::mutex m_mutex;

    // Storage for different metric types
    std::unordered_map<std::string, std::vector<MetricData>> m_metrics;
    std::unordered_map<std::string, MetricStatistics> m_statistics;

    // Sampling state
    std::atomic<uint64_t> m_sampleCounter{0};

    // Memory management
    std::atomic<uint64_t> m_totalMetricsRecorded{0};

    MetricsCollector() = default;

public:
    static MetricsCollector& getInstance();

    // Prevent copying
    MetricsCollector(const MetricsCollector&) = delete;
    MetricsCollector& operator=(const MetricsCollector&) = delete;

    /**
     * @brief Initialize with configuration
     */
    void initialize(const ProfilingConfig& config);

    /**
     * @brief Check if profiling is enabled
     */
    bool isEnabled() const { return m_config.enabled; }

    /**
     * @brief Check if a specific metric type is enabled
     */
    bool isMetricTypeEnabled(MetricType type) const;

    /**
     * @brief Record a counter metric (incrementing value)
     */
    void recordCounter(const std::string& name, double value,
                      const std::string& category = "general",
                      const std::string& unit = "count");

    /**
     * @brief Record a gauge metric (current value)
     */
    void recordGauge(const std::string& name, double value,
                    const std::string& category = "general",
                    const std::string& unit = "");

    /**
     * @brief Record a timing metric (duration in milliseconds)
     */
    void recordTiming(const std::string& name, double durationMs,
                     const std::string& category = "performance",
                     const std::string& unit = "ms");

    /**
     * @brief Increment a counter by 1
     */
    void incrementCounter(const std::string& name,
                         const std::string& category = "general");

    /**
     * @brief Update statistics for a metric
     */
    void updateStatistics(const std::string& name, double value);

    /**
     * @brief Get statistics for a specific metric
     */
    MetricStatistics getStatistics(const std::string& name) const;

    /**
     * @brief Get all collected metrics
     */
    std::vector<MetricData> getAllMetrics() const;

    /**
     * @brief Get metrics by category
     */
    std::vector<MetricData> getMetricsByCategory(const std::string& category) const;

    /**
     * @brief Get all statistics
     */
    std::unordered_map<std::string, MetricStatistics> getAllStatistics() const;

    /**
     * @brief Clear all collected metrics
     */
    void clear();

    /**
     * @brief Export metrics to string (JSON format)
     */
    std::string exportToJson() const;

    /**
     * @brief Export metrics to CSV format
     */
    std::string exportToCsv() const;

    /**
     * @brief Export statistics to string
     */
    std::string exportStatisticsToJson() const;

    /**
     * @brief Print metrics to console
     */
    void printToConsole() const;

    /**
     * @brief Save metrics to file
     */
    bool saveToFile(const std::string& filename, const std::string& format = "json") const;

    /**
     * @brief Get configuration
     */
    const ProfilingConfig& getConfig() const { return m_config; }

    /**
     * @brief Get total number of metrics recorded
     */
    uint64_t getTotalMetricsRecorded() const { return m_totalMetricsRecorded.load(); }

private:
    bool shouldSample();
    bool checkMemoryLimit();
    void flushIfNeeded();
    std::string metricsToJson(const std::vector<MetricData>& metrics) const;
    std::string metricsToCsv(const std::vector<MetricData>& metrics) const;
};

} // namespace mcf

#include "MetricsCollector.hpp"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace mcf {

MetricsCollector& MetricsCollector::getInstance() {
    static MetricsCollector instance;
    return instance;
}

void MetricsCollector::initialize(const ProfilingConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
    m_metrics.clear();
    m_statistics.clear();
    m_totalMetricsRecorded.store(0);
    m_sampleCounter.store(0);
}

bool MetricsCollector::isMetricTypeEnabled(MetricType type) const {
    if (!m_config.enabled) {
        return false;
    }

    switch (type) {
        case MetricType::Counter: return m_config.enableCounters;
        case MetricType::Gauge: return m_config.enableGauges;
        case MetricType::Timing: return m_config.enableTimings;
        case MetricType::Histogram: return m_config.enableHistograms;
        default: return false;
    }
}

bool MetricsCollector::shouldSample() {
    if (!m_config.enableSampling) {
        return true;  // Always record when sampling is disabled
    }

    uint64_t count = m_sampleCounter.fetch_add(1);
    return (count % m_config.sampleRate) == 0;
}

bool MetricsCollector::checkMemoryLimit() {
    if (m_totalMetricsRecorded.load() >= m_config.maxMetricsInMemory) {
        if (m_config.autoFlushWhenFull) {
            flushIfNeeded();
        }
        return false;
    }
    return true;
}

void MetricsCollector::flushIfNeeded() {
    // In a real implementation, this would save to disk and clear memory
    // For now, we just clear the oldest metrics
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& [name, metrics] : m_metrics) {
        if (metrics.size() > m_config.maxMetricsInMemory / 2) {
            metrics.erase(metrics.begin(), metrics.begin() + metrics.size() / 2);
        }
    }
}

void MetricsCollector::recordCounter(const std::string& name, double value,
                                    const std::string& category,
                                    const std::string& unit) {
    if (!isMetricTypeEnabled(MetricType::Counter)) return;
    if (!m_config.isCategoryEnabled(category)) return;
    if (!shouldSample()) return;
    if (!checkMemoryLimit()) return;

    MetricData data;
    data.name = name;
    data.type = MetricType::Counter;
    data.value = value;
    data.timestamp = std::chrono::system_clock::now();
    data.unit = unit;
    data.category = category;

    if (m_config.threadSafe) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_metrics[name].push_back(data);
        updateStatistics(name, value);
    } else {
        m_metrics[name].push_back(data);
        updateStatistics(name, value);
    }

    m_totalMetricsRecorded.fetch_add(1);
}

void MetricsCollector::recordGauge(const std::string& name, double value,
                                  const std::string& category,
                                  const std::string& unit) {
    if (!isMetricTypeEnabled(MetricType::Gauge)) return;
    if (!m_config.isCategoryEnabled(category)) return;
    if (!shouldSample()) return;
    if (!checkMemoryLimit()) return;

    MetricData data;
    data.name = name;
    data.type = MetricType::Gauge;
    data.value = value;
    data.timestamp = std::chrono::system_clock::now();
    data.unit = unit;
    data.category = category;

    if (m_config.threadSafe) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_metrics[name].push_back(data);
        updateStatistics(name, value);
    } else {
        m_metrics[name].push_back(data);
        updateStatistics(name, value);
    }

    m_totalMetricsRecorded.fetch_add(1);
}

void MetricsCollector::recordTiming(const std::string& name, double durationMs,
                                   const std::string& category,
                                   const std::string& unit) {
    if (!isMetricTypeEnabled(MetricType::Timing)) return;
    if (!m_config.isCategoryEnabled(category)) return;
    if (durationMs < m_config.timingThresholdMs) return;  // Filter out fast timings
    if (!shouldSample()) return;
    if (!checkMemoryLimit()) return;

    MetricData data;
    data.name = name;
    data.type = MetricType::Timing;
    data.value = durationMs;
    data.timestamp = std::chrono::system_clock::now();
    data.unit = unit;
    data.category = category;

    if (m_config.threadSafe) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_metrics[name].push_back(data);
        updateStatistics(name, durationMs);
    } else {
        m_metrics[name].push_back(data);
        updateStatistics(name, durationMs);
    }

    m_totalMetricsRecorded.fetch_add(1);
}

void MetricsCollector::incrementCounter(const std::string& name,
                                       const std::string& category) {
    recordCounter(name, 1.0, category, "count");
}

void MetricsCollector::updateStatistics(const std::string& name, double value) {
    // This should be called from within a locked section
    auto& stats = m_statistics[name];

    if (stats.name.empty()) {
        stats.name = name;
    }

    stats.count++;
    stats.sum += value;
    stats.min = std::min(stats.min, value);
    stats.max = std::max(stats.max, value);

    // Update running mean
    stats.mean = stats.sum / stats.count;

    // For stddev, we'd need to track sum of squares
    // Simplified calculation here
}

MetricStatistics MetricsCollector::getStatistics(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_statistics.find(name);
    if (it != m_statistics.end()) {
        return it->second;
    }
    return MetricStatistics{};
}

std::vector<MetricData> MetricsCollector::getAllMetrics() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<MetricData> result;
    for (const auto& [name, metrics] : m_metrics) {
        result.insert(result.end(), metrics.begin(), metrics.end());
    }

    // Sort by timestamp
    std::sort(result.begin(), result.end(),
        [](const MetricData& a, const MetricData& b) {
            return a.timestamp < b.timestamp;
        });

    return result;
}

std::vector<MetricData> MetricsCollector::getMetricsByCategory(const std::string& category) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<MetricData> result;
    for (const auto& [name, metrics] : m_metrics) {
        for (const auto& metric : metrics) {
            if (metric.category == category) {
                result.push_back(metric);
            }
        }
    }

    return result;
}

std::unordered_map<std::string, MetricStatistics> MetricsCollector::getAllStatistics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_statistics;
}

void MetricsCollector::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metrics.clear();
    m_statistics.clear();
    m_totalMetricsRecorded.store(0);
}

std::string MetricsCollector::metricsToJson(const std::vector<MetricData>& metrics) const {
    std::ostringstream oss;
    oss << "{\n  \"metrics\": [\n";

    for (size_t i = 0; i < metrics.size(); ++i) {
        const auto& m = metrics[i];

        oss << "    {\n";
        oss << "      \"name\": \"" << m.name << "\",\n";
        oss << "      \"type\": \"";
        switch (m.type) {
            case MetricType::Counter: oss << "counter"; break;
            case MetricType::Gauge: oss << "gauge"; break;
            case MetricType::Timing: oss << "timing"; break;
            case MetricType::Histogram: oss << "histogram"; break;
        }
        oss << "\",\n";
        oss << "      \"value\": " << std::fixed << std::setprecision(3) << m.value << ",\n";
        oss << "      \"unit\": \"" << m.unit << "\",\n";
        oss << "      \"category\": \"" << m.category << "\"\n";
        oss << "    }";

        if (i < metrics.size() - 1) {
            oss << ",";
        }
        oss << "\n";
    }

    oss << "  ]\n}\n";
    return oss.str();
}

std::string MetricsCollector::exportToJson() const {
    auto metrics = getAllMetrics();
    return metricsToJson(metrics);
}

std::string MetricsCollector::metricsToCsv(const std::vector<MetricData>& metrics) const {
    std::ostringstream oss;
    oss << "name,type,value,unit,category\n";

    for (const auto& m : metrics) {
        oss << m.name << ",";
        switch (m.type) {
            case MetricType::Counter: oss << "counter"; break;
            case MetricType::Gauge: oss << "gauge"; break;
            case MetricType::Timing: oss << "timing"; break;
            case MetricType::Histogram: oss << "histogram"; break;
        }
        oss << "," << std::fixed << std::setprecision(3) << m.value << ",";
        oss << m.unit << "," << m.category << "\n";
    }

    return oss.str();
}

std::string MetricsCollector::exportToCsv() const {
    auto metrics = getAllMetrics();
    return metricsToCsv(metrics);
}

std::string MetricsCollector::exportStatisticsToJson() const {
    auto stats = getAllStatistics();

    std::ostringstream oss;
    oss << "{\n  \"statistics\": [\n";

    size_t i = 0;
    for (const auto& [name, stat] : stats) {
        oss << "    {\n";
        oss << "      \"name\": \"" << name << "\",\n";
        oss << "      \"count\": " << stat.count << ",\n";
        oss << "      \"sum\": " << std::fixed << std::setprecision(3) << stat.sum << ",\n";
        oss << "      \"min\": " << stat.min << ",\n";
        oss << "      \"max\": " << stat.max << ",\n";
        oss << "      \"mean\": " << stat.mean << "\n";
        oss << "    }";

        if (++i < stats.size()) {
            oss << ",";
        }
        oss << "\n";
    }

    oss << "  ]\n}\n";
    return oss.str();
}

void MetricsCollector::printToConsole() const {
    auto stats = getAllStatistics();

    std::cout << "\n===== Profiling Statistics =====\n";
    std::cout << "Total metrics recorded: " << m_totalMetricsRecorded.load() << "\n\n";

    for (const auto& [name, stat] : stats) {
        std::cout << name << ":\n";
        std::cout << "  Count: " << stat.count << "\n";
        std::cout << "  Min:   " << std::fixed << std::setprecision(3) << stat.min << "\n";
        std::cout << "  Max:   " << stat.max << "\n";
        std::cout << "  Mean:  " << stat.mean << "\n";
        std::cout << "  Sum:   " << stat.sum << "\n\n";
    }
}

bool MetricsCollector::saveToFile(const std::string& filename, const std::string& format) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    if (format == "json") {
        file << exportToJson();
    } else if (format == "csv") {
        file << exportToCsv();
    } else if (format == "stats") {
        file << exportStatisticsToJson();
    } else {
        return false;
    }

    return true;
}

} // namespace mcf

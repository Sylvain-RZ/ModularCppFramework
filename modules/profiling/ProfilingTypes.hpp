#pragma once

#include <string>
#include <chrono>
#include <cstdint>
#include <vector>
#include <limits>

namespace mcf {

/**
 * @brief Types of metrics that can be collected
 */
enum class MetricType {
    Counter,      // Monotonically increasing value (e.g., total requests)
    Gauge,        // Value that can go up or down (e.g., memory usage)
    Timing,       // Duration measurements (e.g., function execution time)
    Histogram     // Distribution of values (e.g., frame time distribution)
};

/**
 * @brief Single metric data point
 */
struct MetricData {
    std::string name;
    MetricType type;
    double value;
    std::chrono::system_clock::time_point timestamp;
    std::string unit;  // "ms", "bytes", "count", etc.
    std::string category; // "performance", "memory", "network", etc.
};

/**
 * @brief Histogram bucket for distribution tracking
 */
struct HistogramBucket {
    double lowerBound;
    double upperBound;
    uint64_t count;
};

/**
 * @brief Aggregated statistics for a metric
 */
struct MetricStatistics {
    std::string name;
    uint64_t count = 0;
    double sum = 0.0;
    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::min();
    double mean = 0.0;
    double stddev = 0.0;
    std::vector<HistogramBucket> histogram;
};

/**
 * @brief Scoped timer for RAII-based profiling
 *
 * Usage:
 * @code
 * {
 *     ScopedTimer timer("my_function");
 *     // code to profile
 * } // timer automatically records duration
 * @endcode
 */
class ScopedTimer {
private:
    std::string m_name;
    std::chrono::high_resolution_clock::time_point m_start;
    bool m_active;

public:
    explicit ScopedTimer(const std::string& name);
    ~ScopedTimer();

    // Prevent copying
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

    // Allow moving
    ScopedTimer(ScopedTimer&& other) noexcept;
    ScopedTimer& operator=(ScopedTimer&& other) noexcept;

    // Manually stop the timer
    void stop();

    // Get elapsed time without stopping
    double elapsed() const;
};

} // namespace mcf

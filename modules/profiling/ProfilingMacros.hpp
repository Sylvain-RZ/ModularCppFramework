#pragma once

#include "MetricsCollector.hpp"
#include "ProfilingTypes.hpp"

/**
 * @file ProfilingMacros.hpp
 * @brief Zero-overhead profiling macros
 *
 * When MCF_PROFILING_ENABLED is not defined, all macros compile to nothing.
 * This ensures zero performance impact in production builds.
 *
 * Usage in CMake:
 * For debug builds:   target_compile_definitions(target PRIVATE MCF_PROFILING_ENABLED)
 * For release builds: (don't define MCF_PROFILING_ENABLED)
 */

// Check if profiling is enabled at compile time
#ifdef MCF_PROFILING_ENABLED

    /**
     * @brief Record a counter metric
     * @param name Metric name
     * @param value Counter value
     */
    #define MCF_PROFILE_COUNTER(name, value) \
        do { \
            if (mcf::MetricsCollector::getInstance().isEnabled()) { \
                mcf::MetricsCollector::getInstance().recordCounter(name, value); \
            } \
        } while(0)

    /**
     * @brief Record a counter with category
     */
    #define MCF_PROFILE_COUNTER_CAT(name, value, category) \
        do { \
            if (mcf::MetricsCollector::getInstance().isEnabled()) { \
                mcf::MetricsCollector::getInstance().recordCounter(name, value, category); \
            } \
        } while(0)

    /**
     * @brief Increment a counter by 1
     */
    #define MCF_PROFILE_INCREMENT(name) \
        do { \
            if (mcf::MetricsCollector::getInstance().isEnabled()) { \
                mcf::MetricsCollector::getInstance().incrementCounter(name); \
            } \
        } while(0)

    /**
     * @brief Record a gauge metric
     */
    #define MCF_PROFILE_GAUGE(name, value) \
        do { \
            if (mcf::MetricsCollector::getInstance().isEnabled()) { \
                mcf::MetricsCollector::getInstance().recordGauge(name, value); \
            } \
        } while(0)

    /**
     * @brief Record a gauge with category
     */
    #define MCF_PROFILE_GAUGE_CAT(name, value, category) \
        do { \
            if (mcf::MetricsCollector::getInstance().isEnabled()) { \
                mcf::MetricsCollector::getInstance().recordGauge(name, value, category); \
            } \
        } while(0)

    /**
     * @brief Profile a scope (RAII timer)
     * @param name Scope name
     *
     * Usage:
     * @code
     * void myFunction() {
     *     MCF_PROFILE_SCOPE("myFunction");
     *     // function code
     * }
     * @endcode
     */
    #define MCF_PROFILE_SCOPE(name) \
        mcf::ScopedTimer __mcf_scoped_timer_##__LINE__(name)

    /**
     * @brief Profile a function (place at start of function)
     */
    #define MCF_PROFILE_FUNCTION() \
        MCF_PROFILE_SCOPE(__FUNCTION__)

    /**
     * @brief Start a manual timer
     * @param varname Variable name for the timer
     * @param name Timer name
     */
    #define MCF_PROFILE_START(varname, name) \
        auto varname = std::chrono::high_resolution_clock::now(); \
        std::string varname##_name = name

    /**
     * @brief End a manual timer
     * @param varname Variable name of the timer
     */
    #define MCF_PROFILE_END(varname) \
        do { \
            if (mcf::MetricsCollector::getInstance().isEnabled()) { \
                auto __end = std::chrono::high_resolution_clock::now(); \
                auto __duration = std::chrono::duration_cast<std::chrono::microseconds>(__end - varname).count(); \
                mcf::MetricsCollector::getInstance().recordTiming( \
                    varname##_name, \
                    __duration / 1000.0 \
                ); \
            } \
        } while(0)

    /**
     * @brief Record timing manually (in milliseconds)
     */
    #define MCF_PROFILE_TIMING(name, durationMs) \
        do { \
            if (mcf::MetricsCollector::getInstance().isEnabled()) { \
                mcf::MetricsCollector::getInstance().recordTiming(name, durationMs); \
            } \
        } while(0)

#else
    // Production builds - all macros compile to nothing (zero overhead)
    #define MCF_PROFILE_COUNTER(name, value) ((void)0)
    #define MCF_PROFILE_COUNTER_CAT(name, value, category) ((void)0)
    #define MCF_PROFILE_INCREMENT(name) ((void)0)
    #define MCF_PROFILE_GAUGE(name, value) ((void)0)
    #define MCF_PROFILE_GAUGE_CAT(name, value, category) ((void)0)
    #define MCF_PROFILE_SCOPE(name) ((void)0)
    #define MCF_PROFILE_FUNCTION() ((void)0)
    #define MCF_PROFILE_START(varname, name) ((void)0)
    #define MCF_PROFILE_END(varname) ((void)0)
    #define MCF_PROFILE_TIMING(name, durationMs) ((void)0)

#endif // MCF_PROFILING_ENABLED

namespace mcf {

/**
 * @brief Helper to check if profiling is enabled (works even without MCF_PROFILING_ENABLED)
 */
inline bool isProfilingEnabled() {
#ifdef MCF_PROFILING_ENABLED
    return MetricsCollector::getInstance().isEnabled();
#else
    return false;
#endif
}

} // namespace mcf

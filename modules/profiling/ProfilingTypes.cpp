#include "ProfilingTypes.hpp"
#include "MetricsCollector.hpp"

namespace mcf {

ScopedTimer::ScopedTimer(const std::string& name)
    : m_name(name)
    , m_start(std::chrono::high_resolution_clock::now())
    , m_active(true) {
}

ScopedTimer::~ScopedTimer() {
    if (m_active) {
        stop();
    }
}

ScopedTimer::ScopedTimer(ScopedTimer&& other) noexcept
    : m_name(std::move(other.m_name))
    , m_start(other.m_start)
    , m_active(other.m_active) {
    other.m_active = false;
}

ScopedTimer& ScopedTimer::operator=(ScopedTimer&& other) noexcept {
    if (this != &other) {
        if (m_active) {
            stop();
        }
        m_name = std::move(other.m_name);
        m_start = other.m_start;
        m_active = other.m_active;
        other.m_active = false;
    }
    return *this;
}

void ScopedTimer::stop() {
    if (!m_active) {
        return;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start).count();
    double durationMs = duration / 1000.0;

    MetricsCollector::getInstance().recordTiming(m_name, durationMs);

    m_active = false;
}

double ScopedTimer::elapsed() const {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start).count();
    return duration / 1000.0;  // Return in milliseconds
}

} // namespace mcf

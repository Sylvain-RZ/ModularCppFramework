#pragma once

namespace mcf {

/**
 * @brief Optional trait interface for components that need real-time updates
 *
 * Components (plugins, modules, services) can optionally implement this interface
 * to receive frame-based updates with delta time. This is typically used by:
 * - Game objects that need per-frame updates
 * - Animation systems
 * - Physics simulations
 * - Real-time visualizations
 *
 * If a component doesn't need regular updates (e.g., event-driven services,
 * configuration managers, etc.), it should NOT implement this interface.
 */
class IRealtimeUpdatable {
public:
    virtual ~IRealtimeUpdatable() = default;

    /**
     * @brief Called every frame by the realtime module
     * @param deltaTime Time elapsed since last frame in seconds
     */
    virtual void onRealtimeUpdate(float deltaTime) = 0;
};

} // namespace mcf

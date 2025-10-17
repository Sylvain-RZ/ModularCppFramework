#pragma once

namespace mcf {

/**
 * @brief Optional trait interface for event-driven components
 *
 * Components can implement this interface to declare they are purely event-driven
 * and don't require regular polling or updates. This is a marker interface that
 * helps document the component's architecture and can be used for optimization
 * (e.g., skipping unnecessary update loops).
 *
 * Typical use cases:
 * - REST API handlers
 * - Message queue consumers
 * - Database connection pools
 * - Async I/O services
 * - Event processors
 *
 * Event-driven components typically:
 * 1. Subscribe to events during initialization
 * 2. React to events when they occur
 * 3. Don't need regular update() calls
 */
class IEventDriven {
public:
    virtual ~IEventDriven() = default;

    /**
     * @brief Get the component's event-driven nature description
     * @return Human-readable description of what events this component handles
     *
     * Example: "Handles HTTP requests via EventBus", "Processes database queries"
     */
    virtual const char* getEventDrivenDescription() const {
        return "Event-driven component";
    }
};

} // namespace mcf

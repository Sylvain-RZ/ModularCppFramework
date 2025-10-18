#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <typeindex>
#include <vector>
#include <any>

namespace mcf {

/**
 * @brief Base class for all events
 */
struct Event {
    virtual ~Event() = default;

    /**
     * @brief Name identifier for the event
     */
    std::string name;

    /**
     * @brief Type-erased event data payload
     */
    std::any data;

    /**
     * @brief Default constructor
     */
    Event() = default;

    /**
     * @brief Construct an event with a name
     * @param eventName The name identifier for the event
     */
    explicit Event(const std::string& eventName) : name(eventName) {}

    /**
     * @brief Construct an event with a name and data
     * @tparam T Type of the event data
     * @param eventName The name identifier for the event
     * @param eventData The data payload for the event
     */
    template<typename T>
    Event(const std::string& eventName, const T& eventData)
        : name(eventName), data(eventData) {}
};

/**
 * @brief Handle to an event subscription, used for unsubscribing
 */
using EventHandle = size_t;

/**
 * @brief Event callback function type
 */
using EventCallback = std::function<void(const Event&)>;

/**
 * @brief Subscriber information
 */
struct Subscriber {
    /**
     * @brief Unique handle for this subscription
     */
    EventHandle handle;

    /**
     * @brief Callback function to invoke when event is published
     */
    EventCallback callback;

    /**
     * @brief Priority of this subscriber (higher priority = called first)
     */
    int priority = 0;

    /**
     * @brief Whether this subscription should be removed after one execution
     */
    bool once = false;

    /**
     * @brief Optional plugin identifier for cleanup when plugin is unloaded
     */
    std::string pluginId;

    /**
     * @brief Construct a subscriber
     * @param h Unique handle for this subscription
     * @param cb Callback function to invoke
     * @param prio Priority value (higher = called first)
     * @param o Whether this is a one-time subscription
     * @param pid Optional plugin identifier
     */
    Subscriber(EventHandle h, EventCallback cb, int prio = 0, bool o = false, std::string pid = "")
        : handle(h), callback(std::move(cb)), priority(prio), once(o), pluginId(std::move(pid)) {}
};

/**
 * @brief Event bus for publish-subscribe pattern communication
 *
 * Thread-safe event dispatcher allowing loose coupling between
 * plugins and modules.
 */
class EventBus {
private:
    // Map of event type to list of subscribers
    std::map<std::type_index, std::vector<Subscriber>> m_typedSubscribers;

    // Map of event name to list of subscribers
    std::map<std::string, std::vector<Subscriber>> m_namedSubscribers;

    // Thread safety
    mutable std::mutex m_mutex;

    // Handle counter
    EventHandle m_nextHandle = 1;

    // Event queue for deferred dispatch
    std::vector<std::shared_ptr<Event>> m_eventQueue;
    std::mutex m_queueMutex;

public:
    EventBus() = default;
    ~EventBus() = default;

    // Non-copyable
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    /**
     * @brief Subscribe to typed events
     * @tparam T Event type
     * @param callback Function to call when event is published
     * @param priority Higher priority callbacks are invoked first
     * @return Handle for unsubscribing
     */
    template<typename T>
    EventHandle subscribe(EventCallback callback, int priority = 0) {
        std::lock_guard<std::mutex> lock(m_mutex);
        EventHandle handle = m_nextHandle++;

        auto& subscribers = m_typedSubscribers[std::type_index(typeid(T))];
        subscribers.emplace_back(handle, std::move(callback), priority, false);

        // Sort by priority (descending)
        std::sort(subscribers.begin(), subscribers.end(),
                 [](const Subscriber& a, const Subscriber& b) {
                     return a.priority > b.priority;
                 });

        return handle;
    }

    /**
     * @brief Subscribe to named events
     * @param eventName Name of the event
     * @param callback Function to call when event is published
     * @param priority Higher priority callbacks are invoked first
     * @return Handle for unsubscribing
     */
    EventHandle subscribe(const std::string& eventName,
                         EventCallback callback,
                         int priority = 0) {
        std::lock_guard<std::mutex> lock(m_mutex);
        EventHandle handle = m_nextHandle++;

        auto& subscribers = m_namedSubscribers[eventName];
        subscribers.emplace_back(handle, std::move(callback), priority, false);

        // Sort by priority (descending)
        std::sort(subscribers.begin(), subscribers.end(),
                 [](const Subscriber& a, const Subscriber& b) {
                     return a.priority > b.priority;
                 });

        return handle;
    }

    /**
     * @brief Subscribe to named events with plugin tracking
     * @param eventName Name of the event
     * @param callback Function to call when event is published
     * @param priority Higher priority callbacks are invoked first
     * @param pluginId Plugin identifier for cleanup
     * @return Handle for unsubscribing
     */
    EventHandle subscribeWithPlugin(const std::string& eventName,
                                    EventCallback callback,
                                    int priority,
                                    const std::string& pluginId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        EventHandle handle = m_nextHandle++;

        auto& subscribers = m_namedSubscribers[eventName];
        subscribers.emplace_back(handle, std::move(callback), priority, false, pluginId);

        // Sort by priority (descending)
        std::sort(subscribers.begin(), subscribers.end(),
                 [](const Subscriber& a, const Subscriber& b) {
                     return a.priority > b.priority;
                 });

        return handle;
    }

    /**
     * @brief Subscribe to typed events for one-time execution
     * @tparam T Event type
     * @param callback Function to call when event is published
     * @param priority Higher priority callbacks are invoked first
     * @return Handle for unsubscribing
     */
    template<typename T>
    EventHandle subscribeOnce(EventCallback callback, int priority = 0) {
        std::lock_guard<std::mutex> lock(m_mutex);
        EventHandle handle = m_nextHandle++;

        auto& subscribers = m_typedSubscribers[std::type_index(typeid(T))];
        subscribers.emplace_back(handle, std::move(callback), priority, true);

        std::sort(subscribers.begin(), subscribers.end(),
                 [](const Subscriber& a, const Subscriber& b) {
                     return a.priority > b.priority;
                 });

        return handle;
    }

    /**
     * @brief Subscribe to named events for one-time execution
     * @param eventName Name of the event
     * @param callback Function to call when event is published
     * @param priority Higher priority callbacks are invoked first
     * @return Handle for unsubscribing
     */
    EventHandle subscribeOnce(const std::string& eventName,
                             EventCallback callback,
                             int priority = 0) {
        std::lock_guard<std::mutex> lock(m_mutex);
        EventHandle handle = m_nextHandle++;

        auto& subscribers = m_namedSubscribers[eventName];
        subscribers.emplace_back(handle, std::move(callback), priority, true);

        std::sort(subscribers.begin(), subscribers.end(),
                 [](const Subscriber& a, const Subscriber& b) {
                     return a.priority > b.priority;
                 });

        return handle;
    }

    /**
     * @brief Unsubscribe from events
     * @param handle Handle returned by subscribe()
     */
    void unsubscribe(EventHandle handle) {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Remove from typed subscribers
        for (auto& [type, subscribers] : m_typedSubscribers) {
            subscribers.erase(
                std::remove_if(subscribers.begin(), subscribers.end(),
                              [handle](const Subscriber& s) {
                                  return s.handle == handle;
                              }),
                subscribers.end()
            );
        }

        // Remove from named subscribers
        for (auto& [name, subscribers] : m_namedSubscribers) {
            subscribers.erase(
                std::remove_if(subscribers.begin(), subscribers.end(),
                              [handle](const Subscriber& s) {
                                  return s.handle == handle;
                              }),
                subscribers.end()
            );
        }
    }

    /**
     * @brief Unsubscribe all events for a specific plugin
     * @param pluginId Plugin identifier
     * @return Number of subscriptions removed
     */
    size_t unsubscribePlugin(const std::string& pluginId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t count = 0;

        // Remove from typed subscribers
        for (auto& [type, subscribers] : m_typedSubscribers) {
            auto oldSize = subscribers.size();
            subscribers.erase(
                std::remove_if(subscribers.begin(), subscribers.end(),
                              [&pluginId](const Subscriber& s) {
                                  return s.pluginId == pluginId;
                              }),
                subscribers.end()
            );
            count += (oldSize - subscribers.size());
        }

        // Remove from named subscribers
        for (auto& [name, subscribers] : m_namedSubscribers) {
            auto oldSize = subscribers.size();
            subscribers.erase(
                std::remove_if(subscribers.begin(), subscribers.end(),
                              [&pluginId](const Subscriber& s) {
                                  return s.pluginId == pluginId;
                              }),
                subscribers.end()
            );
            count += (oldSize - subscribers.size());
        }

        return count;
    }

    /**
     * @brief Publish a typed event synchronously
     * @tparam T Event type
     * @param event The event to publish
     */
    template<typename T>
    void publish(const T& event) {
        std::vector<Subscriber> subscribersCopy;
        std::vector<EventHandle> onceHandles;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_typedSubscribers.find(std::type_index(typeid(T)));
            if (it != m_typedSubscribers.end()) {
                subscribersCopy = it->second;

                // Collect once-only subscribers
                for (const auto& sub : subscribersCopy) {
                    if (sub.once) {
                        onceHandles.push_back(sub.handle);
                    }
                }
            }
        }

        // Invoke callbacks outside of lock
        Event baseEvent;
        baseEvent.data = event;

        for (const auto& subscriber : subscribersCopy) {
            subscriber.callback(baseEvent);
        }

        // Remove once-only subscribers
        for (auto handle : onceHandles) {
            unsubscribe(handle);
        }
    }

    /**
     * @brief Publish a named event synchronously
     * @param eventName Name of the event to publish
     * @param event The event to publish
     */
    void publish(const std::string& eventName, const Event& event) {
        std::vector<Subscriber> subscribersCopy;
        std::vector<EventHandle> onceHandles;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_namedSubscribers.find(eventName);
            if (it != m_namedSubscribers.end()) {
                subscribersCopy = it->second;

                for (const auto& sub : subscribersCopy) {
                    if (sub.once) {
                        onceHandles.push_back(sub.handle);
                    }
                }
            }
        }

        for (const auto& subscriber : subscribersCopy) {
            subscriber.callback(event);
        }

        for (auto handle : onceHandles) {
            unsubscribe(handle);
        }
    }

    /**
     * @brief Queue an event for deferred dispatch
     * @param event Shared pointer to the event to queue
     */
    void queueEvent(std::shared_ptr<Event> event) {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_eventQueue.push_back(std::move(event));
    }

    /**
     * @brief Dispatch all queued events
     */
    void processQueue() {
        std::vector<std::shared_ptr<Event>> queueCopy;

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            queueCopy = std::move(m_eventQueue);
            m_eventQueue.clear();
        }

        for (const auto& event : queueCopy) {
            if (!event->name.empty()) {
                publish(event->name, *event);
            }
        }
    }

    /**
     * @brief Clear all subscribers
     */
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_typedSubscribers.clear();
        m_namedSubscribers.clear();
    }

    /**
     * @brief Get number of subscribers for a named event
     * @param eventName Name of the event to query
     * @return Number of subscribers for the specified event
     */
    size_t subscriberCount(const std::string& eventName) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_namedSubscribers.find(eventName);
        return (it != m_namedSubscribers.end()) ? it->second.size() : 0;
    }
};

} // namespace mcf

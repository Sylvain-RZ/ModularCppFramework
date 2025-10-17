/**
 * @file test_eventbus_catch2.cpp
 * @brief Unit tests for EventBus using Catch2 v3
 */

#define CATCH_CONFIG_MAIN
#include <catch_amalgamated.hpp>

#include "../../core/EventBus.hpp"
#include <string>
#include <vector>
#include <thread>
#include <atomic>

using namespace mcf;

TEST_CASE("EventBus - Basic subscribe and publish", "[eventbus][core]") {
    EventBus bus;
    bool called = false;

    bus.subscribe("test.event", [&](const Event& e) {
        called = true;
    });

    Event event("test.event");
    bus.publish("test.event", event);

    REQUIRE(called);
}

TEST_CASE("EventBus - Multiple subscribers", "[eventbus][core]") {
    EventBus bus;
    int callCount = 0;

    bus.subscribe("test.event", [&](const Event&) { callCount++; });
    bus.subscribe("test.event", [&](const Event&) { callCount++; });
    bus.subscribe("test.event", [&](const Event&) { callCount++; });

    Event event("test.event");
    bus.publish("test.event", event);

    REQUIRE(callCount == 3);
}

TEST_CASE("EventBus - Priority ordering", "[eventbus][core]") {
    EventBus bus;
    std::string order;

    bus.subscribe("test.event", [&](const Event&) { order += "B"; }, 50);  // Medium
    bus.subscribe("test.event", [&](const Event&) { order += "A"; }, 100); // High
    bus.subscribe("test.event", [&](const Event&) { order += "C"; }, 10);  // Low

    Event event("test.event");
    bus.publish("test.event", event);

    REQUIRE(order == "ABC");
}

TEST_CASE("EventBus - Event data transmission", "[eventbus][core]") {
    EventBus bus;
    int receivedValue = 0;

    bus.subscribe("test.event", [&](const Event& e) {
        receivedValue = std::any_cast<int>(e.data);
    });

    Event event("test.event", 42);
    bus.publish("test.event", event);

    REQUIRE(receivedValue == 42);
}

TEST_CASE("EventBus - Unsubscribe", "[eventbus][core]") {
    EventBus bus;
    int callCount = 0;

    auto handle = bus.subscribe("test.event", [&](const Event&) { callCount++; });

    SECTION("Callback is called before unsubscribe") {
        Event event("test.event");
        bus.publish("test.event", event);
        REQUIRE(callCount == 1);
    }

    SECTION("Callback is not called after unsubscribe") {
        Event event("test.event");
        bus.publish("test.event", event);
        REQUIRE(callCount == 1);

        bus.unsubscribe(handle);
        bus.publish("test.event", event);
        REQUIRE(callCount == 1);
    }
}

TEST_CASE("EventBus - Subscribe once", "[eventbus][core]") {
    EventBus bus;
    int callCount = 0;

    bus.subscribeOnce("test.event", [&](const Event&) { callCount++; });

    Event event("test.event");
    bus.publish("test.event", event);
    REQUIRE(callCount == 1);

    bus.publish("test.event", event);
    REQUIRE(callCount == 1);
}

TEST_CASE("EventBus - Plugin-aware subscription", "[eventbus][core][hot-reload]") {
    EventBus bus;
    int plugin1Calls = 0;
    int plugin2Calls = 0;

    SECTION("Multiple subscriptions from same plugin") {
        bus.subscribeWithPlugin("test.event", [&](const Event&) { plugin1Calls++; }, 0, "Plugin1");
        bus.subscribeWithPlugin("test.event", [&](const Event&) { plugin1Calls++; }, 0, "Plugin1");
        bus.subscribeWithPlugin("test.event", [&](const Event&) { plugin2Calls++; }, 0, "Plugin2");

        Event event("test.event");
        bus.publish("test.event", event);
        REQUIRE(plugin1Calls == 2);
        REQUIRE(plugin2Calls == 1);
    }

    SECTION("Unsubscribe all for specific plugin") {
        bus.subscribeWithPlugin("test.event", [&](const Event&) { plugin1Calls++; }, 0, "Plugin1");
        bus.subscribeWithPlugin("test.event", [&](const Event&) { plugin1Calls++; }, 0, "Plugin1");
        bus.subscribeWithPlugin("test.event", [&](const Event&) { plugin2Calls++; }, 0, "Plugin2");

        size_t removed = bus.unsubscribePlugin("Plugin1");
        REQUIRE(removed == 2);

        Event event("test.event");
        bus.publish("test.event", event);
        REQUIRE(plugin1Calls == 0);
        REQUIRE(plugin2Calls == 1);
    }
}

TEST_CASE("EventBus - Clear all subscribers", "[eventbus][core]") {
    EventBus bus;
    int callCount = 0;

    bus.subscribe("event1", [&](const Event&) { callCount++; });
    bus.subscribe("event2", [&](const Event&) { callCount++; });

    bus.clear();

    Event event("event1");
    bus.publish("event1", event);
    bus.publish("event2", event);

    REQUIRE(callCount == 0);
}

TEST_CASE("EventBus - Subscriber count", "[eventbus][core]") {
    EventBus bus;

    REQUIRE(bus.subscriberCount("test.event") == 0);

    bus.subscribe("test.event", [](const Event&) {});
    REQUIRE(bus.subscriberCount("test.event") == 1);

    bus.subscribe("test.event", [](const Event&) {});
    REQUIRE(bus.subscriberCount("test.event") == 2);
}

TEST_CASE("EventBus - Event queue", "[eventbus][core]") {
    EventBus bus;
    int callCount = 0;

    bus.subscribe("test.event", [&](const Event&) { callCount++; });

    SECTION("Queued events are not processed immediately") {
        auto event = std::make_shared<Event>("test.event");
        bus.queueEvent(event);
        REQUIRE(callCount == 0);
    }

    SECTION("processQueue processes all queued events") {
        auto event = std::make_shared<Event>("test.event");
        bus.queueEvent(event);
        bus.processQueue();
        REQUIRE(callCount == 1);
    }
}

TEST_CASE("EventBus - Thread safety", "[eventbus][core][threading]") {
    EventBus bus;
    std::atomic<int> callCount{0};

    // Subscribe from multiple threads
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&bus, &callCount]() {
            bus.subscribe("test.event", [&](const Event&) {
                callCount++;
            });
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    Event event("test.event");
    bus.publish("test.event", event);

    REQUIRE(callCount == 10);
}

TEST_CASE("EventBus - Complex scenarios", "[eventbus][core]") {
    EventBus bus;

    SECTION("Mixed priorities with one-time subscribers") {
        std::string order;

        bus.subscribe("test", [&](const Event&) { order += "A"; }, 100);
        bus.subscribeOnce("test", [&](const Event&) { order += "B"; }, 50);
        bus.subscribe("test", [&](const Event&) { order += "C"; }, 10);

        Event event("test");
        bus.publish("test", event);
        REQUIRE(order == "ABC");

        // Second publish - B should not be called
        order.clear();
        bus.publish("test", event);
        REQUIRE(order == "AC");
    }

    SECTION("Event data with different types") {
        int intValue = 0;
        std::string strValue;

        bus.subscribe("int.event", [&](const Event& e) {
            intValue = std::any_cast<int>(e.data);
        });

        bus.subscribe("str.event", [&](const Event& e) {
            strValue = std::any_cast<std::string>(e.data);
        });

        Event intEvent("int.event", 123);
        Event strEvent("str.event", std::string("hello"));

        bus.publish("int.event", intEvent);
        bus.publish("str.event", strEvent);

        REQUIRE(intValue == 123);
        REQUIRE(strValue == "hello");
    }
}

// Benchmarks (optional, requires Catch2 benchmarking support)
TEST_CASE("EventBus - Performance benchmarks", "[.benchmark][eventbus]") {
    EventBus bus;

    BENCHMARK("Subscribe 100 handlers") {
        EventBus localBus;
        for (int i = 0; i < 100; ++i) {
            localBus.subscribe("test", [](const Event&) {});
        }
        return localBus.subscriberCount("test");
    };

    BENCHMARK("Publish to 100 handlers") {
        EventBus localBus;
        for (int i = 0; i < 100; ++i) {
            localBus.subscribe("test", [](const Event&) {});
        }
        Event event("test");
        return [&]() { localBus.publish("test", event); };
    };
}

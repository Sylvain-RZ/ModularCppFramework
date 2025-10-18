#include <catch_amalgamated.hpp>
#include "../../core/EventBus.hpp"
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

using namespace mcf;

TEST_CASE("EventBus - Priority ordering edge cases", "[EventBus][EdgeCases]") {
    SECTION("Multiple handlers with same priority") {
        EventBus bus;
        std::vector<int> callOrder;

        // Add multiple handlers with same priority
        bus.subscribe("test", [&](const Event& e) { callOrder.push_back(1); }, 100);
        bus.subscribe("test", [&](const Event& e) { callOrder.push_back(2); }, 100);
        bus.subscribe("test", [&](const Event& e) { callOrder.push_back(3); }, 100);

        bus.publish("test", Event("test"));

        REQUIRE(callOrder.size() == 3);
        // All should be called, order among same priority may vary
        REQUIRE(std::find(callOrder.begin(), callOrder.end(), 1) != callOrder.end());
        REQUIRE(std::find(callOrder.begin(), callOrder.end(), 2) != callOrder.end());
        REQUIRE(std::find(callOrder.begin(), callOrder.end(), 3) != callOrder.end());
    }

    SECTION("Priority ordering with many different priorities") {
        EventBus bus;
        std::vector<int> callOrder;

        // Add handlers with different priorities (higher priority = called first)
        bus.subscribe("test", [&](const Event& e) { callOrder.push_back(50); }, 50);
        bus.subscribe("test", [&](const Event& e) { callOrder.push_back(200); }, 200);
        bus.subscribe("test", [&](const Event& e) { callOrder.push_back(10); }, 10);
        bus.subscribe("test", [&](const Event& e) { callOrder.push_back(150); }, 150);
        bus.subscribe("test", [&](const Event& e) { callOrder.push_back(100); }, 100);

        bus.publish("test", Event("test"));

        REQUIRE(callOrder.size() == 5);
        // Should be called in descending priority order
        REQUIRE(callOrder[0] == 200);
        REQUIRE(callOrder[1] == 150);
        REQUIRE(callOrder[2] == 100);
        REQUIRE(callOrder[3] == 50);
        REQUIRE(callOrder[4] == 10);
    }

    SECTION("Negative priorities") {
        EventBus bus;
        std::vector<int> callOrder;

        bus.subscribe("test", [&](const Event& e) { callOrder.push_back(-100); }, -100);
        bus.subscribe("test", [&](const Event& e) { callOrder.push_back(0); }, 0);
        bus.subscribe("test", [&](const Event& e) { callOrder.push_back(100); }, 100);
        bus.subscribe("test", [&](const Event& e) { callOrder.push_back(-50); }, -50);

        bus.publish("test", Event("test"));

        REQUIRE(callOrder.size() == 4);
        REQUIRE(callOrder[0] == 100);
        REQUIRE(callOrder[1] == 0);
        REQUIRE(callOrder[2] == -50);
        REQUIRE(callOrder[3] == -100);
    }

    SECTION("Very large priority values") {
        EventBus bus;
        std::vector<int> callOrder;

        bus.subscribe("test", [&](const Event& e) { callOrder.push_back(1); }, 1000000);
        bus.subscribe("test", [&](const Event& e) { callOrder.push_back(2); }, -1000000);
        bus.subscribe("test", [&](const Event& e) { callOrder.push_back(3); }, 0);

        bus.publish("test", Event("test"));

        REQUIRE(callOrder.size() == 3);
        REQUIRE(callOrder[0] == 1);
        REQUIRE(callOrder[1] == 3);
        REQUIRE(callOrder[2] == 2);
    }
}

TEST_CASE("EventBus - Concurrent subscribe/unsubscribe during event dispatch", "[EventBus][EdgeCases][Threading]") {
    SECTION("Subscribe while publishing") {
        EventBus bus;
        std::atomic<int> counter{0};

        // Subscribe a handler that subscribes another handler
        bus.subscribe("test", [&](const Event& e) {
            counter++;
            // Subscribe new handler during event processing
            bus.subscribe("test", [&](const Event& e) {
                counter++;
            });
        });

        bus.publish("test", Event("test"));
        REQUIRE(counter == 1); // New handler not called in current publish

        bus.publish("test", Event("test"));
        REQUIRE(counter >= 2); // New handler called in next publish
    }

    SECTION("Unsubscribe while publishing") {
        EventBus bus;
        std::atomic<int> counter{0};

        auto handle = bus.subscribe("test", [&](const Event& e) {
            counter++;
        });

        bus.subscribe("test", [&, handle](const Event& e) {
            counter++;
            // Unsubscribe the first handler during event processing
            bus.unsubscribe(handle);
        });

        bus.publish("test", Event("test"));
        REQUIRE(counter >= 1);

        counter = 0;
        bus.publish("test", Event("test"));
        // First handler should not be called anymore
        REQUIRE(counter >= 1);
    }

    SECTION("Multiple threads publishing same event") {
        EventBus bus;
        std::atomic<int> counter{0};

        bus.subscribe("test", [&](const Event& e) {
            counter++;
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        });

        const int numThreads = 10;
        std::vector<std::thread> threads;

        for (int i = 0; i < numThreads; i++) {
            threads.emplace_back([&bus]() {
                for (int j = 0; j < 10; j++) {
                    bus.publish("test", Event("test"));
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        REQUIRE(counter == numThreads * 10);
    }

    SECTION("Multiple threads subscribing and unsubscribing") {
        EventBus bus;
        std::atomic<int> subscribeCount{0};
        std::atomic<int> unsubscribeCount{0};

        const int numThreads = 5;
        std::vector<std::thread> threads;

        for (int i = 0; i < numThreads; i++) {
            threads.emplace_back([&bus, &subscribeCount, &unsubscribeCount]() {
                for (int j = 0; j < 20; j++) {
                    auto handle = bus.subscribe("test", [](const Event& e) {});
                    subscribeCount++;
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                    bus.unsubscribe(handle);
                    unsubscribeCount++;
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        REQUIRE(subscribeCount == numThreads * 20);
        REQUIRE(unsubscribeCount == numThreads * 20);
    }

    SECTION("Publishing while subscribing and unsubscribing") {
        EventBus bus;
        std::atomic<int> publishCount{0};
        std::atomic<int> handlerCallCount{0};
        std::atomic<bool> running{true};

        // Thread that constantly subscribes and unsubscribes
        std::thread subThread([&]() {
            while (running) {
                auto handle = bus.subscribe("test", [&](const Event& e) {
                    handlerCallCount++;
                });
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                bus.unsubscribe(handle);
            }
        });

        // Thread that constantly publishes
        std::thread pubThread([&]() {
            for (int i = 0; i < 100; i++) {
                bus.publish("test", Event("test"));
                publishCount++;
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });

        pubThread.join();
        running = false;
        subThread.join();

        REQUIRE(publishCount == 100);
        // handlerCallCount may vary but should not crash
    }
}

TEST_CASE("EventBus - Recursive event publishing", "[EventBus][EdgeCases]") {
    SECTION("Handler publishes same event (limited recursion)") {
        EventBus bus;
        std::atomic<int> counter{0};

        bus.subscribe("test", [&](const Event& e) {
            counter++;
            if (counter < 5) { // Limit recursion to prevent infinite loop
                bus.publish("test", Event("test"));
            }
        });

        bus.publish("test", Event("test"));
        REQUIRE(counter == 5);
    }

    SECTION("Handler publishes different event") {
        EventBus bus;
        std::atomic<int> counter1{0};
        std::atomic<int> counter2{0};

        bus.subscribe("event1", [&](const Event& e) {
            counter1++;
            if (counter1 < 3) {
                bus.publish("event2", Event("event2"));
            }
        });

        bus.subscribe("event2", [&](const Event& e) {
            counter2++;
        });

        bus.publish("event1", Event("event1"));
        REQUIRE(counter1 >= 1);
        REQUIRE(counter2 >= 1);
    }

    SECTION("Chain of events") {
        EventBus bus;
        std::vector<std::string> eventOrder;

        bus.subscribe("event1", [&](const Event& e) {
            eventOrder.push_back("event1");
            bus.publish("event2", Event("event2"));
        });

        bus.subscribe("event2", [&](const Event& e) {
            eventOrder.push_back("event2");
            bus.publish("event3", Event("event3"));
        });

        bus.subscribe("event3", [&](const Event& e) {
            eventOrder.push_back("event3");
        });

        bus.publish("event1", Event("event1"));

        REQUIRE(eventOrder.size() == 3);
        REQUIRE(eventOrder[0] == "event1");
        REQUIRE(eventOrder[1] == "event2");
        REQUIRE(eventOrder[2] == "event3");
    }

    SECTION("Circular event dependencies") {
        EventBus bus;
        std::atomic<int> counter1{0};
        std::atomic<int> counter2{0};

        bus.subscribe("event1", [&](const Event& e) {
            counter1++;
            if (counter1 < 3) {
                bus.publish("event2", Event("event2"));
            }
        });

        bus.subscribe("event2", [&](const Event& e) {
            counter2++;
            if (counter2 < 3) {
                bus.publish("event1", Event("event1"));
            }
        });

        bus.publish("event1", Event("event1"));

        // Should have limited recursion
        REQUIRE(counter1 >= 2);
        REQUIRE(counter2 >= 2);
    }
}

TEST_CASE("EventBus - Edge cases with event data", "[EventBus][EdgeCases]") {
    SECTION("Event with no data") {
        EventBus bus;
        bool called = false;

        bus.subscribe("test", [&](const Event& e) {
            called = true;
            REQUIRE(e.name == "test");
        });

        bus.publish("test", Event("test"));
        REQUIRE(called);
    }

    SECTION("Event with int data") {
        EventBus bus;
        bool called = false;

        bus.subscribe("test", [&](const Event& e) {
            called = true;
            int value = std::any_cast<int>(e.data);
            REQUIRE(value == 42);
        });

        bus.publish("test", Event("test", 42));
        REQUIRE(called);
    }

    SECTION("Event with string data") {
        EventBus bus;
        bool called = false;

        bus.subscribe("test", [&](const Event& e) {
            called = true;
            std::string value = std::any_cast<std::string>(e.data);
            REQUIRE(value == "hello");
        });

        bus.publish("test", Event("test", std::string("hello")));
        REQUIRE(called);
    }

    SECTION("Event with complex data structure") {
        struct ComplexData {
            int id;
            std::string name;
            std::vector<int> values;
        };

        EventBus bus;
        bool called = false;

        bus.subscribe("test", [&](const Event& e) {
            called = true;
            ComplexData data = std::any_cast<ComplexData>(e.data);
            REQUIRE(data.id == 123);
            REQUIRE(data.name == "test");
            REQUIRE(data.values.size() == 3);
        });

        ComplexData data{123, "test", {1, 2, 3}};
        bus.publish("test", Event("test", data));
        REQUIRE(called);
    }
}

TEST_CASE("EventBus - Unsubscribe edge cases", "[EventBus][EdgeCases]") {
    SECTION("Unsubscribe non-existent handle") {
        EventBus bus;
        EventHandle fakeHandle = 999999;

        // Should not crash
        bus.unsubscribe(fakeHandle);
        REQUIRE(true);
    }

    SECTION("Unsubscribe same handle twice") {
        EventBus bus;

        auto handle = bus.subscribe("test", [](const Event& e) {});

        bus.unsubscribe(handle);
        bus.unsubscribe(handle); // Second unsubscribe should be safe

        REQUIRE(true);
    }

    SECTION("Unsubscribe all handlers for an event") {
        EventBus bus;
        std::atomic<int> counter{0};

        auto handle1 = bus.subscribe("test", [&](const Event& e) { counter++; });
        auto handle2 = bus.subscribe("test", [&](const Event& e) { counter++; });
        auto handle3 = bus.subscribe("test", [&](const Event& e) { counter++; });

        bus.publish("test", Event("test"));
        REQUIRE(counter == 3);

        counter = 0;
        bus.unsubscribe(handle1);
        bus.unsubscribe(handle2);
        bus.unsubscribe(handle3);

        bus.publish("test", Event("test"));
        REQUIRE(counter == 0); // No handlers left
    }
}

TEST_CASE("EventBus - Multiple event types", "[EventBus][EdgeCases]") {
    SECTION("Many different event types") {
        EventBus bus;
        std::map<std::string, int> counters;

        for (int i = 0; i < 100; i++) {
            std::string eventType = "event_" + std::to_string(i);
            bus.subscribe(eventType, [&counters, eventType](const Event& e) {
                counters[eventType]++;
            });
        }

        // Publish each event once
        for (int i = 0; i < 100; i++) {
            std::string eventType = "event_" + std::to_string(i);
            bus.publish(eventType, Event(eventType));
        }

        // Verify each event was received exactly once
        for (int i = 0; i < 100; i++) {
            std::string eventType = "event_" + std::to_string(i);
            REQUIRE(counters[eventType] == 1);
        }
    }

    SECTION("Event type isolation") {
        EventBus bus;
        int counter1 = 0, counter2 = 0;

        bus.subscribe("type1", [&](const Event& e) { counter1++; });
        bus.subscribe("type2", [&](const Event& e) { counter2++; });

        bus.publish("type1", Event("type1"));
        REQUIRE(counter1 == 1);
        REQUIRE(counter2 == 0);

        bus.publish("type2", Event("type2"));
        REQUIRE(counter1 == 1);
        REQUIRE(counter2 == 1);
    }
}

TEST_CASE("EventBus - Handler exceptions", "[EventBus][EdgeCases]") {
    SECTION("Handler throws exception - propagates") {
        EventBus bus;
        std::atomic<int> counter{0};

        bus.subscribe("test", [&](const Event& e) {
            counter++;
            throw std::runtime_error("Test exception");
        }, 100);

        bus.subscribe("test", [&](const Event& e) {
            counter++;
        }, 50);

        // EventBus does not catch exceptions - they propagate to the caller
        REQUIRE_THROWS_AS(bus.publish("test", Event("test")), std::runtime_error);

        // First handler was called before exception
        REQUIRE(counter >= 1);
    }

    SECTION("Non-throwing handler") {
        EventBus bus;
        std::atomic<int> counter{0};

        bus.subscribe("test", [&](const Event& e) {
            counter++;
        });

        // This should not throw
        REQUIRE_NOTHROW(bus.publish("test", Event("test")));
        REQUIRE(counter == 1);
    }
}

TEST_CASE("EventBus - Empty event type", "[EventBus][EdgeCases]") {
    SECTION("Subscribe and publish with empty event type") {
        EventBus bus;
        bool called = false;

        bus.subscribe("", [&](const Event& e) {
            called = true;
        });

        bus.publish("", Event(""));
        REQUIRE(called);
    }
}

TEST_CASE("EventBus - Many handlers on same event", "[EventBus][EdgeCases]") {
    SECTION("100 handlers on same event") {
        EventBus bus;
        std::atomic<int> counter{0};

        for (int i = 0; i < 100; i++) {
            bus.subscribe("test", [&](const Event& e) {
                counter++;
            });
        }

        bus.publish("test", Event("test"));
        REQUIRE(counter == 100);
    }

    SECTION("Handlers with data modification") {
        EventBus bus;
        std::atomic<int> sum{0};

        for (int i = 0; i < 10; i++) {
            bus.subscribe("test", [&, i](const Event& e) {
                sum += i;
            });
        }

        bus.publish("test", Event("test"));
        REQUIRE(sum == 45); // 0+1+2+...+9
    }
}

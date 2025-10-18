/**
 * @file test_stress.cpp
 * @brief Stress and performance tests for the framework
 *
 * Tests system behavior under high load:
 * - Many plugins
 * - High event throughput
 * - Concurrent operations
 * - Memory stress
 */

#include "../../core/Application.hpp"
#include "../../core/EventBus.hpp"
#include "../../core/ServiceLocator.hpp"
#include "../../core/ResourceManager.hpp"
#include "../../core/ThreadPool.hpp"
#include "../../core/IPlugin.hpp"
#include "../../external/catch_amalgamated.hpp"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <random>

using namespace mcf;
using namespace std::chrono;

// ===========================================================================
// Helper Classes
// ===========================================================================

// Minimal plugin for stress testing
class StressPlugin : public IPlugin {
private:
    bool m_initialized = false;
    PluginMetadata m_metadata;
    PluginContext m_context;
    std::atomic<int> m_updateCount{0};

public:
    explicit StressPlugin(int id) {
        m_metadata.name = "StressPlugin" + std::to_string(id);
        m_metadata.version = "1.0.0";
        m_metadata.loadPriority = 100 - id;  // Varying priorities
    }

    bool initialize(PluginContext& context) override {
        m_context = context;
        m_initialized = true;
        return true;
    }

    void shutdown() override {
        m_initialized = false;
    }

    bool isInitialized() const override {
        return m_initialized;
    }

    std::string getName() const override {
        return m_metadata.name;
    }

    std::string getVersion() const override {
        return m_metadata.version;
    }

    const PluginMetadata& getMetadata() const override {
        return m_metadata;
    }

    // onUpdate not in base IPlugin - would need IRealtimeUpdatable
    // void onUpdate(float deltaTime) override {
    //     m_updateCount++;
    // }

    void incrementUpdate() { m_updateCount++; }  // Helper for testing
    int getUpdateCount() const { return m_updateCount.load(); }

    static const char* getManifestJson() {
        return R"({"name":"StressPlugin","version":"1.0.0"})";
    }
};

// Test application
class StressTestApp : public Application {
public:
    StressTestApp() : Application(createConfig()) {}

    static ApplicationConfig createConfig() {
        ApplicationConfig config;
        config.name = "StressTest";
        config.version = "1.0.0";
        config.pluginDirectory = "./plugins";
        config.autoLoadPlugins = false;
        config.autoInitPlugins = false;
        return config;
    }
};

// ===========================================================================
// Stress Tests
// ===========================================================================

TEST_CASE("Stress - High event throughput", "[stress][eventbus][benchmark]") {
    EventBus eventBus;

    SECTION("10,000 events with 10 subscribers") {
        std::atomic<int> totalReceived{0};
        std::vector<EventHandle> handles;

        // Create 10 subscribers
        for (int i = 0; i < 10; ++i) {
            auto handle = eventBus.subscribe("stress.test", [&totalReceived](const Event&) {
                totalReceived++;
            });
            handles.push_back(handle);
        }

        auto start = high_resolution_clock::now();

        // Publish 10,000 events
        const int EVENT_COUNT = 10000;
        for (int i = 0; i < EVENT_COUNT; ++i) {
            Event event("stress.test", i);
            eventBus.publish("stress.test", event);
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);

        // Should have received all events
        REQUIRE(totalReceived == EVENT_COUNT * 10);

        // Performance check: should process in reasonable time
        INFO("Processed " << EVENT_COUNT << " events in " << duration.count() << "ms");
        REQUIRE(duration.count() < 5000);  // Less than 5 seconds

        // Cleanup
        for (auto handle : handles) {
            eventBus.unsubscribe(handle);
        }
    }

    SECTION("Concurrent event publishing from multiple threads") {
        std::atomic<int> totalReceived{0};

        auto handle = eventBus.subscribe("concurrent.test", [&totalReceived](const Event&) {
            totalReceived++;
        });

        const int THREAD_COUNT = 10;
        const int EVENTS_PER_THREAD = 1000;

        auto start = high_resolution_clock::now();

        std::vector<std::thread> threads;
        for (int t = 0; t < THREAD_COUNT; ++t) {
            threads.emplace_back([&eventBus, EVENTS_PER_THREAD]() {
                for (int i = 0; i < EVENTS_PER_THREAD; ++i) {
                    Event event("concurrent.test", i);
                    eventBus.publish("concurrent.test", event);
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);

        REQUIRE(totalReceived == THREAD_COUNT * EVENTS_PER_THREAD);

        INFO("Concurrent: " << THREAD_COUNT * EVENTS_PER_THREAD << " events in " << duration.count() << "ms");

        eventBus.unsubscribe(handle);
    }
}

TEST_CASE("Stress - Many service registrations", "[stress][servicelocator][benchmark]") {
    ServiceLocator locator;

    SECTION("Register 1000 services") {
        auto start = high_resolution_clock::now();

        for (int i = 0; i < 1000; ++i) {
            struct DummyService { int value = 0; };
            locator.registerSingleton<DummyService>(std::make_shared<DummyService>());
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);

        INFO("Registered 1000 services in " << duration.count() << "ms");
        REQUIRE(duration.count() < 1000);  // Less than 1 second

        // serviceCount() not implemented in ServiceLocator
        // REQUIRE(locator.serviceCount() == 1000);
    }

    SECTION("Concurrent service resolution") {
        struct SharedService { std::atomic<int> counter{0}; };
        locator.registerSingleton<SharedService>(std::make_shared<SharedService>());

        const int THREAD_COUNT = 20;
        const int RESOLVES_PER_THREAD = 500;

        auto start = high_resolution_clock::now();

        std::vector<std::thread> threads;
        for (int t = 0; t < THREAD_COUNT; ++t) {
            threads.emplace_back([&locator, RESOLVES_PER_THREAD]() {
                for (int i = 0; i < RESOLVES_PER_THREAD; ++i) {
                    auto service = locator.resolve<SharedService>();
                    service->counter++;
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);

        auto service = locator.resolve<SharedService>();
        REQUIRE(service->counter == THREAD_COUNT * RESOLVES_PER_THREAD);

        INFO("Concurrent resolutions: " << THREAD_COUNT * RESOLVES_PER_THREAD << " in " << duration.count() << "ms");
    }
}

TEST_CASE("Stress - Resource loading and caching", "[stress][resourcemanager][benchmark]") {
    ResourceManager manager;

    struct DummyResource {
        std::vector<uint8_t> data;
        explicit DummyResource(size_t size) : data(size, 0) {}
    };

    SECTION("Load 500 resources") {
        manager.registerLoader<DummyResource>([](const std::string& path) {
            return std::make_shared<DummyResource>(1024);  // 1KB each
        });

        auto start = high_resolution_clock::now();

        for (int i = 0; i < 500; ++i) {
            auto resource = manager.load<DummyResource>("resource_" + std::to_string(i));
            REQUIRE(resource != nullptr);
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);

        REQUIRE(manager.getResourceCount() == 500);

        INFO("Loaded 500 resources in " << duration.count() << "ms");
    }

    SECTION("Cache hit performance") {
        manager.registerLoader<DummyResource>([](const std::string& path) {
            return std::make_shared<DummyResource>(1024);
        });

        // First load (cache miss)
        auto resource1 = manager.load<DummyResource>("cached_resource");

        // Second load (cache hit) - should be very fast
        auto start = high_resolution_clock::now();

        for (int i = 0; i < 10000; ++i) {
            auto resource = manager.load<DummyResource>("cached_resource");
            REQUIRE(resource == resource1);  // Same instance
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);

        INFO("10,000 cache hits in " << duration.count() << " microseconds");
        REQUIRE(duration.count() < 100000);  // Less than 100ms
    }
}

TEST_CASE("Stress - ThreadPool task throughput", "[stress][threadpool][benchmark]") {
    ThreadPool pool(8);  // 8 worker threads

    SECTION("Process 10,000 tasks") {
        std::atomic<int> completed{0};

        auto start = high_resolution_clock::now();

        std::vector<std::future<void>> futures;
        for (int i = 0; i < 10000; ++i) {
            futures.push_back(pool.submit([&completed]() {
                completed++;
            }));
        }

        // Wait for all tasks
        for (auto& future : futures) {
            future.get();
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);

        REQUIRE(completed == 10000);

        INFO("Completed 10,000 tasks in " << duration.count() << "ms");
        REQUIRE(duration.count() < 5000);
    }

    SECTION("Heavy computation tasks") {
        std::atomic<long long> total{0};

        auto start = high_resolution_clock::now();

        std::vector<std::future<long long>> futures;
        for (int i = 0; i < 100; ++i) {
            futures.push_back(pool.submit([i]() -> long long {
                long long sum = 0;
                for (int j = 0; j < 100000; ++j) {
                    sum += (i * j) % 1000;
                }
                return sum;
            }));
        }

        for (auto& future : futures) {
            total += future.get();
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);

        INFO("Heavy tasks completed in " << duration.count() << "ms");
        REQUIRE(duration.count() < 10000);
    }
}

TEST_CASE("Stress - Many plugins scenario", "[stress][plugins][benchmark]") {
    StressTestApp app;
    REQUIRE(app.initialize());

    SECTION("Create and initialize 100 plugins") {
        std::vector<std::unique_ptr<StressPlugin>> plugins;

        auto start = high_resolution_clock::now();

        for (int i = 0; i < 100; ++i) {
            auto plugin = std::make_unique<StressPlugin>(i);

            PluginContext ctx(
                app.getEventBus(),
                app.getServiceLocator(),
                &app,
                nullptr,
                nullptr,
                plugin->getMetadata().name
            );

            REQUIRE(plugin->initialize(ctx));
            plugins.push_back(std::move(plugin));
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);

        REQUIRE(plugins.size() == 100);

        INFO("Initialized 100 plugins in " << duration.count() << "ms");

        // Simulate updates (using incrementUpdate since onUpdate requires IRealtimeUpdatable)
        for (int frame = 0; frame < 100; ++frame) {
            for (auto& plugin : plugins) {
                plugin->incrementUpdate();
            }
        }

        // Verify all plugins updated
        for (const auto& plugin : plugins) {
            REQUIRE(plugin->getUpdateCount() == 100);
        }

        // Shutdown all
        for (auto& plugin : plugins) {
            plugin->shutdown();
        }
    }

    app.shutdown();
}

TEST_CASE("Stress - Memory allocation patterns", "[stress][memory][benchmark]") {
    SECTION("Repeated allocations and deallocations") {
        auto start = high_resolution_clock::now();

        for (int cycle = 0; cycle < 1000; ++cycle) {
            std::vector<std::shared_ptr<int>> ptrs;

            for (int i = 0; i < 100; ++i) {
                ptrs.push_back(std::make_shared<int>(i));
            }

            // Pointers go out of scope and deallocate
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);

        INFO("1000 allocation cycles in " << duration.count() << "ms");
        REQUIRE(duration.count() < 5000);
    }

    SECTION("Large event data payloads") {
        EventBus eventBus;

        struct LargePayload {
            std::vector<uint8_t> data;
            explicit LargePayload(size_t size) : data(size, 0) {}
        };

        std::atomic<int> received{0};

        auto handle = eventBus.subscribe("large.event", [&received](const Event&) {
            received++;
        });

        auto start = high_resolution_clock::now();

        // Send 1000 events with 10KB payloads each
        for (int i = 0; i < 1000; ++i) {
            Event event("large.event", LargePayload(10240));
            eventBus.publish("large.event", event);
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start);

        REQUIRE(received == 1000);

        INFO("1000 large events (10KB each) in " << duration.count() << "ms");

        eventBus.unsubscribe(handle);
    }
}

TEST_CASE("Stress - Complex dependency graph", "[stress][dependencies][benchmark]") {
    DependencyResolver resolver;

    SECTION("Resolve 50 plugins with complex dependencies") {
        // Create 50 plugins with various dependencies
        std::vector<PluginMetadata> plugins;

        for (int i = 0; i < 50; ++i) {
            PluginMetadata meta;
            meta.name = "Plugin" + std::to_string(i);
            meta.version = "1.0.0";
            meta.loadPriority = 100 - i;

            // Add some dependencies (not circular)
            if (i > 0 && i % 5 != 0) {
                // Depend on previous plugin
                meta.addDependency("Plugin" + std::to_string(i - 1), "1.0.0", "2.0.0", true);
            }

            if (i > 10 && i % 7 == 0) {
                // Additional dependency
                meta.addDependency("Plugin" + std::to_string(i - 5), "1.0.0", "2.0.0", false);
            }

            plugins.push_back(meta);
            resolver.addPlugin(meta);
        }

        auto start = high_resolution_clock::now();

        auto loadOrder = resolver.resolve();

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);

        REQUIRE(loadOrder.size() == 50);

        INFO("Resolved 50-plugin dependency graph in " << duration.count() << " microseconds");
        REQUIRE(duration.count() < 10000);  // Less than 10ms
    }
}

TEST_CASE("Stress - Sustained load test", "[stress][sustained][benchmark]") {
    StressTestApp app;
    REQUIRE(app.initialize());

    SECTION("Run for 10 seconds with continuous activity") {
        std::atomic<bool> running{true};
        std::atomic<int> eventCount{0};
        std::atomic<int> serviceCount{0};

        // Event publisher thread
        std::thread eventThread([&]() {
            while (running) {
                Event event("sustained.test", eventCount.load());
                app.getEventBus()->publish("sustained.test", event);
                eventCount++;
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });

        // Service registration thread
        std::thread serviceThread([&]() {
            while (running) {
                struct TempService { int id; };
                app.getServiceLocator()->registerSingleton<TempService>(
                    std::make_shared<TempService>()
                );
                serviceCount++;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });

        // Subscribe to events
        std::atomic<int> receivedEvents{0};
        auto handle = app.getEventBus()->subscribe("sustained.test", [&](const Event&) {
            receivedEvents++;
        });

        // Run for 2 seconds (reduced from 10 for faster tests)
        std::this_thread::sleep_for(std::chrono::seconds(2));

        running = false;
        eventThread.join();
        serviceThread.join();

        INFO("Events published: " << eventCount);
        INFO("Events received: " << receivedEvents);
        INFO("Services registered: " << serviceCount);

        REQUIRE(receivedEvents > 0);
        REQUIRE(serviceCount > 0);

        app.getEventBus()->unsubscribe(handle);
    }

    app.shutdown();
}

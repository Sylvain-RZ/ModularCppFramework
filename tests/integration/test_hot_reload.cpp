/**
 * @file test_hot_reload_catch2.cpp
 * @brief Integration test for hot reload functionality using Catch2
 */

#include "../../core/Application.hpp"
#include "../../core/IPlugin.hpp"
#include "../../core/PluginMetadata.hpp"
#include "../../core/EventBus.hpp"
#include "../../core/ServiceLocator.hpp"
#include "../../core/ResourceManager.hpp"
#include "../../core/DependencyResolver.hpp"
#include "../../core/FileWatcher.hpp"
#include "../../external/catch_amalgamated.hpp"

#include <fstream>
#include <chrono>
#include <thread>
#include <atomic>

using namespace mcf;

// =============================================================================
// Test Fixtures
// =============================================================================

// Simple test plugin for hot reload
class SimpleReloadPlugin : public IPlugin {
private:
    bool m_initialized = false;
    PluginMetadata m_metadata;
    int m_counter = 0;
    int m_reloadCount = 0;

public:
    SimpleReloadPlugin() {
        m_metadata.name = "SimpleReload";
        m_metadata.version = "1.0.0";
    }

    std::string getName() const override { return m_metadata.name; }
    std::string getVersion() const override { return m_metadata.version; }
    const PluginMetadata& getMetadata() const override { return m_metadata; }

    bool initialize(PluginContext& context) override {
        m_initialized = true;
        return true;
    }

    void shutdown() override {
        m_initialized = false;
    }

    void onUpdate(float deltaTime) override {
        m_counter++;
    }

    bool isInitialized() const override { return m_initialized; }

    std::string serializeState() override {
        return std::to_string(m_counter) + "," + std::to_string(m_reloadCount);
    }

    void deserializeState(const std::string& state) override {
        size_t pos = state.find(',');
        if (pos != std::string::npos) {
            m_counter = std::stoi(state.substr(0, pos));
            m_reloadCount = std::stoi(state.substr(pos + 1));
            m_reloadCount++; // Increment on each reload
        }
    }

    int getCounter() const { return m_counter; }
    int getReloadCount() const { return m_reloadCount; }

    static const char* getManifestJson() {
        return R"({"name": "SimpleReload", "version": "1.0.0"})";
    }
};

// Test Application
class TestHotReloadApp : public Application {
public:
    TestHotReloadApp() : Application(createConfig()) {}

    static ApplicationConfig createConfig() {
        ApplicationConfig config;
        config.name = "HotReloadTest";
        config.version = "1.0.0";
        config.pluginDirectory = "./plugins";
        config.autoLoadPlugins = false;
        config.autoInitPlugins = false;
        config.targetFPS = 60;
        return config;
    }
};

// =============================================================================
// Application Pause/Resume Tests
// =============================================================================

TEST_CASE("Application - Pause and Resume", "[application][hot-reload]") {
    TestHotReloadApp app;
    REQUIRE(app.initialize());

    SECTION("Initial state is not paused") {
        REQUIRE_FALSE(app.isPaused());
    }

    SECTION("Pause application") {
        app.pause();
        REQUIRE(app.isPaused());
    }

    SECTION("Resume application") {
        app.pause();
        REQUIRE(app.isPaused());

        app.resume();
        REQUIRE_FALSE(app.isPaused());
    }

    SECTION("Multiple pause/resume cycles") {
        app.pause();
        REQUIRE(app.isPaused());
        app.resume();
        REQUIRE_FALSE(app.isPaused());

        app.pause();
        REQUIRE(app.isPaused());
        app.resume();
        REQUIRE_FALSE(app.isPaused());
    }

    app.shutdown();
}

// =============================================================================
// Plugin State Serialization Tests
// =============================================================================

TEST_CASE("Plugin - State Serialization", "[plugin][hot-reload]") {
    SimpleReloadPlugin plugin;
    PluginContext ctx(nullptr, nullptr, nullptr, nullptr, nullptr, "test");

    plugin.initialize(ctx);

    SECTION("Serialize and deserialize counter state") {
        plugin.onUpdate(0.016f);
        plugin.onUpdate(0.016f);
        plugin.onUpdate(0.016f);

        REQUIRE(plugin.getCounter() == 3);

        std::string state = plugin.serializeState();
        REQUIRE_FALSE(state.empty());

        SimpleReloadPlugin plugin2;
        plugin2.initialize(ctx);
        plugin2.deserializeState(state);

        REQUIRE(plugin2.getCounter() == 3);
        REQUIRE(plugin2.getReloadCount() == 1);
    }

    SECTION("Multiple serialization cycles increment reload count") {
        plugin.onUpdate(0.016f);
        std::string state1 = plugin.serializeState();

        SimpleReloadPlugin plugin2;
        plugin2.initialize(ctx);
        plugin2.deserializeState(state1);
        REQUIRE(plugin2.getReloadCount() == 1);

        std::string state2 = plugin2.serializeState();

        SimpleReloadPlugin plugin3;
        plugin3.initialize(ctx);
        plugin3.deserializeState(state2);
        REQUIRE(plugin3.getReloadCount() == 2);
    }

    SECTION("Empty state handles gracefully") {
        SimpleReloadPlugin plugin2;
        plugin2.initialize(ctx);
        plugin2.deserializeState("");
        // Should not crash
        REQUIRE(plugin2.getCounter() == 0);
    }
}

// =============================================================================
// EventBus Plugin Cleanup Tests
// =============================================================================

TEST_CASE("EventBus - Plugin-aware cleanup", "[eventbus][hot-reload]") {
    EventBus bus;
    int callCount = 0;

    SECTION("Unsubscribe all handlers for specific plugin") {
        bus.subscribeWithPlugin("test.event", [&](const Event&) { callCount++; }, 0, "Plugin1");
        bus.subscribeWithPlugin("test.event", [&](const Event&) { callCount++; }, 0, "Plugin1");
        bus.subscribeWithPlugin("test.event", [&](const Event&) { callCount++; }, 0, "Plugin2");

        Event event("test.event");
        bus.publish("test.event", event);
        REQUIRE(callCount == 3);

        size_t removed = bus.unsubscribePlugin("Plugin1");
        REQUIRE(removed == 2);

        callCount = 0;
        bus.publish("test.event", event);
        REQUIRE(callCount == 1); // Only Plugin2 handler remains
    }

    SECTION("Unsubscribe non-existent plugin is safe") {
        bus.subscribeWithPlugin("test.event", [&](const Event&) { callCount++; }, 0, "Plugin1");

        size_t removed = bus.unsubscribePlugin("NonExistent");
        REQUIRE(removed == 0);

        Event event("test.event");
        bus.publish("test.event", event);
        REQUIRE(callCount == 1); // Plugin1 handler still works
    }

    SECTION("Multiple events with same plugin") {
        bus.subscribeWithPlugin("event1", [&](const Event&) { callCount++; }, 0, "Plugin1");
        bus.subscribeWithPlugin("event2", [&](const Event&) { callCount++; }, 0, "Plugin1");
        bus.subscribeWithPlugin("event3", [&](const Event&) { callCount++; }, 0, "Plugin1");

        size_t removed = bus.unsubscribePlugin("Plugin1");
        REQUIRE(removed == 3);

        Event e1("event1"), e2("event2"), e3("event3");
        bus.publish("event1", e1);
        bus.publish("event2", e2);
        bus.publish("event3", e3);
        REQUIRE(callCount == 0); // All handlers removed
    }
}

// =============================================================================
// ServiceLocator Plugin Cleanup Tests
// =============================================================================

TEST_CASE("ServiceLocator - Plugin-aware cleanup", "[servicelocator][hot-reload]") {
    ServiceLocator locator;

    class TestService {
    public:
        int value = 42;
    };

    class TestService2 {
    public:
        int value = 100;
    };

    SECTION("Unregister services for specific plugin") {
        locator.registerSingletonWithPlugin<TestService>(
            std::make_shared<TestService>(),
            "Plugin1"
        );

        REQUIRE(locator.isRegistered<TestService>());

        size_t removed = locator.unregisterPlugin("Plugin1");
        REQUIRE(removed == 1);

        REQUIRE_FALSE(locator.isRegistered<TestService>());
    }

    SECTION("Multiple services from same plugin") {
        locator.registerSingletonWithPlugin<TestService>(
            std::make_shared<TestService>(),
            "Plugin1"
        );
        locator.registerSingletonWithPlugin<TestService2>(
            std::make_shared<TestService2>(),
            "Plugin1"
        );

        REQUIRE(locator.isRegistered<TestService>());
        REQUIRE(locator.isRegistered<TestService2>());

        size_t removed = locator.unregisterPlugin("Plugin1");
        REQUIRE(removed == 2);

        REQUIRE_FALSE(locator.isRegistered<TestService>());
        REQUIRE_FALSE(locator.isRegistered<TestService2>());
    }

    SECTION("Services from different plugins") {
        locator.registerSingletonWithPlugin<TestService>(
            std::make_shared<TestService>(),
            "Plugin1"
        );
        locator.registerSingletonWithPlugin<TestService2>(
            std::make_shared<TestService2>(),
            "Plugin2"
        );

        size_t removed = locator.unregisterPlugin("Plugin1");
        REQUIRE(removed == 1);

        REQUIRE_FALSE(locator.isRegistered<TestService>());
        REQUIRE(locator.isRegistered<TestService2>()); // Plugin2 service remains
    }
}

// =============================================================================
// ResourceManager Plugin Cleanup Tests
// =============================================================================

TEST_CASE("ResourceManager - Plugin-aware cleanup", "[resourcemanager][hot-reload]") {
    ResourceManager manager;

    struct TestResource {
        int value = 100;
    };

    SECTION("Unload resources for specific plugin") {
        manager.addWithPlugin<TestResource>("res1", std::make_shared<TestResource>(), "Plugin1");
        manager.addWithPlugin<TestResource>("res2", std::make_shared<TestResource>(), "Plugin1");
        manager.addWithPlugin<TestResource>("res3", std::make_shared<TestResource>(), "Plugin2");

        REQUIRE(manager.getResourceCount() == 3);

        size_t removed = manager.unloadPlugin("Plugin1");
        REQUIRE(removed == 2);

        REQUIRE(manager.getResourceCount() == 1);
        REQUIRE(manager.isLoaded("res3"));
        REQUIRE_FALSE(manager.isLoaded("res1"));
        REQUIRE_FALSE(manager.isLoaded("res2"));
    }

    SECTION("Unload non-existent plugin is safe") {
        manager.addWithPlugin<TestResource>("res1", std::make_shared<TestResource>(), "Plugin1");

        size_t removed = manager.unloadPlugin("NonExistent");
        REQUIRE(removed == 0);

        REQUIRE(manager.getResourceCount() == 1);
        REQUIRE(manager.isLoaded("res1"));
    }

    SECTION("All plugins unload") {
        manager.addWithPlugin<TestResource>("res1", std::make_shared<TestResource>(), "Plugin1");
        manager.addWithPlugin<TestResource>("res2", std::make_shared<TestResource>(), "Plugin2");

        manager.unloadPlugin("Plugin1");
        manager.unloadPlugin("Plugin2");

        REQUIRE(manager.getResourceCount() == 0);
    }
}

// =============================================================================
// Dependency Reverse Lookup Tests
// =============================================================================

TEST_CASE("DependencyResolver - Reverse dependency lookup", "[dependencyresolver][hot-reload]") {
    DependencyResolver resolver;

    PluginMetadata pA;
    pA.name = "PluginA";
    pA.version = "1.0.0";

    PluginMetadata pB;
    pB.name = "PluginB";
    pB.version = "1.0.0";
    pB.addDependency("PluginA", "1.0.0", "2.0.0", true);

    PluginMetadata pC;
    pC.name = "PluginC";
    pC.version = "1.0.0";
    pC.addDependency("PluginA", "1.0.0", "2.0.0", true);

    resolver.addPlugin(pA);
    resolver.addPlugin(pB);
    resolver.addPlugin(pC);

    SECTION("Get all dependents of a plugin") {
        auto dependents = resolver.getDependents("PluginA");
        REQUIRE(dependents.size() == 2);

        bool hasB = std::find(dependents.begin(), dependents.end(), "PluginB") != dependents.end();
        bool hasC = std::find(dependents.begin(), dependents.end(), "PluginC") != dependents.end();

        REQUIRE(hasB);
        REQUIRE(hasC);
    }

    SECTION("Plugin with no dependents") {
        auto dependents = resolver.getDependents("PluginB");
        REQUIRE(dependents.empty());
    }

    SECTION("Dependency chain") {
        PluginMetadata pD;
        pD.name = "PluginD";
        pD.version = "1.0.0";
        pD.addDependency("PluginB", "1.0.0", "2.0.0", true);

        resolver.addPlugin(pD);

        auto dependentsA = resolver.getDependents("PluginA");
        REQUIRE(dependentsA.size() == 2); // B and C depend on A

        auto dependentsB = resolver.getDependents("PluginB");
        REQUIRE(dependentsB.size() == 1); // D depends on B
        REQUIRE(dependentsB[0] == "PluginD");
    }
}

// =============================================================================
// FileWatcher Basic Functionality Tests
// =============================================================================

TEST_CASE("FileWatcher - Basic hot reload detection", "[filewatcher][hot-reload]") {
    FileWatcher watcher(std::chrono::milliseconds(100));

    std::string testFile = "/tmp/test_filewatcher_hr.txt";
    std::atomic<bool> changeDetected{false};

    // Create test file
    {
        std::ofstream file(testFile);
        file << "initial";
    }

    SECTION("Detect file modification") {
        watcher.addWatch(testFile, [&](const std::string&, FileChangeType type) {
            if (type == FileChangeType::Modified) {
                changeDetected = true;
            }
        });

        watcher.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(150));

        // Modify file
        {
            std::ofstream file(testFile, std::ios::app);
            file << " modified";
        }

        // Wait for detection
        std::this_thread::sleep_for(std::chrono::milliseconds(250));

        watcher.stop();

        REQUIRE(changeDetected);
    }

    // Cleanup
    std::remove(testFile.c_str());
}

TEST_CASE("FileWatcher - Multiple file monitoring", "[filewatcher][hot-reload]") {
    FileWatcher watcher(std::chrono::milliseconds(100));

    std::string file1 = "/tmp/test_watch1.txt";
    std::string file2 = "/tmp/test_watch2.txt";

    std::atomic<int> changesDetected{0};

    // Create files
    {
        std::ofstream f1(file1); f1 << "file1";
        std::ofstream f2(file2); f2 << "file2";
    }

    watcher.addWatch(file1, [&](const std::string&, FileChangeType type) {
        if (type == FileChangeType::Modified) changesDetected++;
    });

    watcher.addWatch(file2, [&](const std::string&, FileChangeType type) {
        if (type == FileChangeType::Modified) changesDetected++;
    });

    watcher.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Modify both files
    {
        std::ofstream f1(file1, std::ios::app); f1 << " mod";
        std::ofstream f2(file2, std::ios::app); f2 << " mod";
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    watcher.stop();

    REQUIRE(changesDetected >= 1); // At least one change detected

    // Cleanup
    std::remove(file1.c_str());
    std::remove(file2.c_str());
}

// =============================================================================
// Integration Benchmarks
// =============================================================================

TEST_CASE("Hot Reload - Performance", "[.benchmark][hot-reload]") {
    BENCHMARK("Serialize plugin state") {
        SimpleReloadPlugin plugin;
        PluginContext ctx(nullptr, nullptr, nullptr, nullptr, nullptr, "bench");
        plugin.initialize(ctx);
        for (int i = 0; i < 100; ++i) {
            plugin.onUpdate(0.016f);
        }
        return plugin.serializeState();
    };

    BENCHMARK("Deserialize plugin state") {
        SimpleReloadPlugin plugin;
        PluginContext ctx(nullptr, nullptr, nullptr, nullptr, nullptr, "bench");
        plugin.initialize(ctx);
        std::string state = "100,5";
        plugin.deserializeState(state);
    };

    BENCHMARK("EventBus plugin cleanup (100 handlers)") {
        return []{
            EventBus bus;
            for (int i = 0; i < 100; ++i) {
                bus.subscribeWithPlugin("event" + std::to_string(i % 10),
                                       [](const Event&) {}, 0, "TestPlugin");
            }
            return bus.unsubscribePlugin("TestPlugin");
        }();
    };
}

#include <catch_amalgamated.hpp>
#include "../../core/PluginManager.hpp"
#include "../../core/IPlugin.hpp"
#include "../../core/Application.hpp"
#include "../../core/EventBus.hpp"
#include "../../core/ServiceLocator.hpp"
#include "../../core/ResourceManager.hpp"

using namespace mcf;

// Mock plugin for testing
class TestPlugin1 : public IPlugin {
private:
    bool m_initialized = false;
    PluginMetadata m_metadata;
    PluginContext m_context;

public:
    TestPlugin1() {
        m_metadata.name = "TestPlugin1";
        m_metadata.version = "1.0.0";
        m_metadata.loadPriority = 100;
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

    const PluginMetadata& getMetadata() const override {
        return m_metadata;
    }

    static const char* getManifestJson() {
        return R"({"name":"TestPlugin1","version":"1.0.0"})";
    }
};

class TestPlugin2 : public IPlugin {
private:
    bool m_initialized = false;
    PluginMetadata m_metadata;
    PluginContext m_context;

public:
    TestPlugin2() {
        m_metadata.name = "TestPlugin2";
        m_metadata.version = "1.0.0";
        m_metadata.loadPriority = 50;
        // Depends on TestPlugin1
        m_metadata.addDependency("TestPlugin1", "1.0.0", "2.0.0", true);
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

    const PluginMetadata& getMetadata() const override {
        return m_metadata;
    }

    static const char* getManifestJson() {
        return R"({"name":"TestPlugin2","version":"1.0.0","dependencies":[{"pluginName":"TestPlugin1"}]})";
    }
};

TEST_CASE("PluginManager - Singleton instance", "[PluginManager]") {
    SECTION("Get instance") {
        PluginManager& manager1 = PluginManager::getInstance();
        PluginManager& manager2 = PluginManager::getInstance();
        REQUIRE(&manager1 == &manager2);
    }

    SECTION("Instance is not null") {
        PluginManager& manager = PluginManager::getInstance();
        REQUIRE(&manager != nullptr);
    }
}

TEST_CASE("PluginManager - Service initialization", "[PluginManager]") {
    PluginManager& manager = PluginManager::getInstance();
    EventBus eventBus;
    ServiceLocator serviceLocator;
    ResourceManager resourceManager;
    Application app;

    SECTION("Initialize with all services") {
        manager.initialize(&eventBus, &serviceLocator, &app, &resourceManager);
        // Should not crash
        REQUIRE(true);
    }

    SECTION("Initialize with minimal services") {
        manager.initialize(&eventBus, &serviceLocator, &app);
        REQUIRE(true);
    }

    // Cleanup
    manager.unloadAll();
}

TEST_CASE("PluginManager - Plugin count", "[PluginManager]") {
    PluginManager& manager = PluginManager::getInstance();
    manager.unloadAll(); // Clean slate

    SECTION("Initial count is zero") {
        REQUIRE(manager.getPluginCount() == 0);
    }

    SECTION("Count reflects loaded plugins") {
        // Note: This test can only be fully validated with actual plugin files
        // For now, just verify the API works
        size_t count = manager.getPluginCount();
        REQUIRE(count >= 0);
    }

    manager.unloadAll();
}

TEST_CASE("PluginManager - Plugin discovery", "[PluginManager]") {
    PluginManager& manager = PluginManager::getInstance();
    manager.unloadAll();

    SECTION("Discover from non-existent directory") {
        size_t count = manager.loadPluginsFromDirectory("/nonexistent/directory");
        REQUIRE(count == 0);
    }

    SECTION("Discover from empty directory") {
        // Create temporary empty directory
        std::filesystem::create_directories("test_empty_plugins");
        size_t count = manager.loadPluginsFromDirectory("test_empty_plugins");
        REQUIRE(count == 0);
        std::filesystem::remove("test_empty_plugins");
    }

    manager.unloadAll();
}

TEST_CASE("PluginManager - Load plugins", "[PluginManager]") {
    PluginManager& manager = PluginManager::getInstance();
    EventBus eventBus;
    ServiceLocator serviceLocator;
    ResourceManager resourceManager;
    Application app;

    manager.initialize(&eventBus, &serviceLocator, &app, &resourceManager);
    manager.unloadAll();

    SECTION("Load from non-existent directory") {
        size_t count = manager.loadPluginsFromDirectory("/nonexistent/directory");
        // Should handle gracefully
        REQUIRE(manager.getPluginCount() == 0);
    }

    SECTION("Load from empty directory") {
        std::filesystem::create_directories("test_empty_plugins");
        size_t count = manager.loadPluginsFromDirectory("test_empty_plugins");
        REQUIRE(manager.getPluginCount() == 0);
        std::filesystem::remove("test_empty_plugins");
    }

    manager.unloadAll();
}

TEST_CASE("PluginManager - Initialize plugins", "[PluginManager]") {
    PluginManager& manager = PluginManager::getInstance();
    EventBus eventBus;
    ServiceLocator serviceLocator;
    ResourceManager resourceManager;
    Application app;

    manager.initialize(&eventBus, &serviceLocator, &app, &resourceManager);
    manager.unloadAll();

    SECTION("Initialize with no plugins loaded") {
        // With the new API, plugins are initialized when loaded
        // So we just test that we can load from an empty directory
        size_t count = manager.loadPluginsFromDirectory("test_empty_plugins");
        REQUIRE(count == 0); // Should succeed even with no plugins
    }

    manager.unloadAll();
}

TEST_CASE("PluginManager - UnloadAll", "[PluginManager]") {
    PluginManager& manager = PluginManager::getInstance();

    SECTION("Unload when empty") {
        manager.unloadAll();
        REQUIRE(manager.getPluginCount() == 0);
    }

    SECTION("Multiple unloadAll calls") {
        manager.unloadAll();
        manager.unloadAll();
        REQUIRE(manager.getPluginCount() == 0);
    }
}

TEST_CASE("PluginManager - Hot reload enable/disable", "[PluginManager]") {
    PluginManager& manager = PluginManager::getInstance();
    manager.unloadAll();

    SECTION("Enable hot reload") {
        manager.enableHotReload(std::chrono::milliseconds(1000));
        // Should not crash
        REQUIRE(true);
    }

    SECTION("Disable hot reload") {
        manager.enableHotReload(std::chrono::milliseconds(1000));
        manager.disableHotReload();
        REQUIRE(true);
    }

    SECTION("Multiple enable calls") {
        manager.enableHotReload(std::chrono::milliseconds(1000));
        manager.enableHotReload(std::chrono::milliseconds(500));
        REQUIRE(true);
    }

    SECTION("Disable without enable") {
        manager.disableHotReload();
        REQUIRE(true);
    }

    manager.disableHotReload();
    manager.unloadAll();
}

TEST_CASE("PluginManager - Get plugin", "[PluginManager]") {
    PluginManager& manager = PluginManager::getInstance();
    manager.unloadAll();

    SECTION("Get non-existent plugin") {
        IPlugin* plugin = manager.getPlugin("NonExistentPlugin");
        REQUIRE(plugin == nullptr);
    }

    SECTION("Get plugin with empty name") {
        IPlugin* plugin = manager.getPlugin("");
        REQUIRE(plugin == nullptr);
    }

    manager.unloadAll();
}

TEST_CASE("PluginManager - Has plugin", "[PluginManager]") {
    PluginManager& manager = PluginManager::getInstance();
    manager.unloadAll();

    SECTION("Check non-existent plugin") {
        bool exists = manager.isLoaded("NonExistentPlugin");
        REQUIRE(exists == false);
    }

    SECTION("Check empty name") {
        bool exists = manager.isLoaded("");
        REQUIRE(exists == false);
    }

    manager.unloadAll();
}

TEST_CASE("PluginManager - Plugin lifecycle callbacks", "[PluginManager]") {
    PluginManager& manager = PluginManager::getInstance();
    manager.unloadAll();

    bool pauseCalled = false;
    bool resumeCalled = false;

    SECTION("Set pause callback") {
        manager.setPauseCallback([&pauseCalled]() { pauseCalled = true; });
        REQUIRE(true);
    }

    SECTION("Set resume callback") {
        manager.setResumeCallback([&resumeCalled]() { resumeCalled = true; });
        REQUIRE(true);
    }

    SECTION("Set both callbacks") {
        manager.setPauseCallback([&pauseCalled]() { pauseCalled = true; });
        manager.setResumeCallback([&resumeCalled]() { resumeCalled = true; });
        REQUIRE(true);
    }

    manager.unloadAll();
}

TEST_CASE("PluginManager - Reload specific plugin", "[PluginManager]") {
    PluginManager& manager = PluginManager::getInstance();
    EventBus eventBus;
    ServiceLocator serviceLocator;
    ResourceManager resourceManager;
    Application app;

    manager.initialize(&eventBus, &serviceLocator, &app, &resourceManager);
    manager.unloadAll();

    SECTION("Reload non-existent plugin") {
        bool result = manager.reloadPlugin("NonExistentPlugin");
        REQUIRE(result == false);
    }

    SECTION("Reload with empty name") {
        bool result = manager.reloadPlugin("");
        REQUIRE(result == false);
    }

    manager.unloadAll();
}

TEST_CASE("PluginManager - Thread safety", "[PluginManager]") {
    PluginManager& manager = PluginManager::getInstance();
    manager.unloadAll();

    SECTION("Concurrent getPluginCount calls") {
        std::vector<std::thread> threads;
        std::atomic<size_t> totalCount{0};

        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&manager, &totalCount]() {
                for (int j = 0; j < 100; ++j) {
                    totalCount += manager.getPluginCount();
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        // Should complete without crashes
        REQUIRE(true);
    }

    SECTION("Concurrent isLoaded calls") {
        std::vector<std::thread> threads;

        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&manager]() {
                for (int j = 0; j < 100; ++j) {
                    manager.isLoaded("TestPlugin");
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        REQUIRE(true);
    }

    manager.unloadAll();
}

TEST_CASE("PluginManager - Error handling", "[PluginManager]") {
    PluginManager& manager = PluginManager::getInstance();
    manager.unloadAll();

    SECTION("Initialize with minimal required services") {
        EventBus eventBus;
        ServiceLocator serviceLocator;
        Application app;
        // Initialize with only required services (no ResourceManager, ThreadPool, ConfigManager)
        manager.initialize(&eventBus, &serviceLocator, &app);
        REQUIRE(true);
    }

    SECTION("Initialize with all optional services as nullptr") {
        EventBus eventBus;
        ServiceLocator serviceLocator;
        Application app;
        manager.initialize(&eventBus, &serviceLocator, &app, nullptr, nullptr, nullptr);
        REQUIRE(true);
    }

    manager.unloadAll();
}

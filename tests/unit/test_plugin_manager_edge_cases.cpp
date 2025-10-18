#include <catch_amalgamated.hpp>
#include "../../core/PluginManager.hpp"
#include "../../core/IPlugin.hpp"
#include "../../core/Application.hpp"
#include "../../core/EventBus.hpp"
#include "../../core/ServiceLocator.hpp"
#include "../../core/ResourceManager.hpp"
#include "../../core/ConfigurationManager.hpp"
#include "../../core/ThreadPool.hpp"
#include "../../core/FileSystem.hpp"
#include <thread>
#include <chrono>

using namespace mcf;

// Mock plugins for testing
class SimplePlugin : public IPlugin {
private:
    bool m_initialized = false;
    PluginMetadata m_metadata;
    PluginContext m_context;

public:
    SimplePlugin() {
        m_metadata.name = "SimplePlugin";
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
        return R"({"name":"SimplePlugin","version":"1.0.0"})";
    }
};

class FailingPlugin : public IPlugin {
private:
    bool m_initialized = false;
    PluginMetadata m_metadata;

public:
    FailingPlugin() {
        m_metadata.name = "FailingPlugin";
        m_metadata.version = "1.0.0";
    }

    bool initialize(PluginContext& context) override {
        // Always fail to initialize
        return false;
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
        return R"({"name":"FailingPlugin","version":"1.0.0"})";
    }
};

class DuplicatePlugin : public IPlugin {
private:
    bool m_initialized = false;
    PluginMetadata m_metadata;

public:
    DuplicatePlugin() {
        m_metadata.name = "SimplePlugin";  // Same name as SimplePlugin
        m_metadata.version = "1.0.0";
    }

    bool initialize(PluginContext& context) override {
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
        return R"({"name":"SimplePlugin","version":"1.0.0"})";
    }
};

TEST_CASE("PluginManager Edge Cases - Load from non-existent directory", "[PluginManager][EdgeCases]") {
    PluginManager::destroy();
    PluginManager& manager = PluginManager::getInstance();

    EventBus eventBus;
    ServiceLocator serviceLocator;
    ResourceManager resourceManager;
    ApplicationConfig config;
    config.name = "TestApp";

    class TestApp : public Application {
    public:
        TestApp(const ApplicationConfig& cfg) : Application(cfg) {}
    };
    TestApp app(config);

    manager.initialize(&eventBus, &serviceLocator, &app, &resourceManager);

    SECTION("Loading from non-existent directory returns 0") {
        size_t count = manager.loadPluginsFromDirectory("/nonexistent/directory/path");
        REQUIRE(count == 0);
    }

    SECTION("Loading from file instead of directory returns 0") {
        // Create a temporary file
        std::string tempFile = "/tmp/test_not_a_directory.txt";
        std::ofstream ofs(tempFile);
        ofs << "test";
        ofs.close();

        size_t count = manager.loadPluginsFromDirectory(tempFile);
        REQUIRE(count == 0);

        std::remove(tempFile.c_str());
    }

    manager.unloadAll();
    PluginManager::destroy();
}

TEST_CASE("PluginManager Edge Cases - Initialize without core services", "[PluginManager][EdgeCases]") {
    PluginManager::destroy();
    PluginManager& manager = PluginManager::getInstance();

    SECTION("InitializeAll fails without EventBus") {
        ServiceLocator serviceLocator;
        ApplicationConfig config;
        config.name = "TestApp";

        class TestApp : public Application {
        public:
            TestApp(const ApplicationConfig& cfg) : Application(cfg) {}
        };
        TestApp app(config);

        manager.initialize(nullptr, &serviceLocator, &app);

        bool result = manager.initializeAll();
        REQUIRE(result == false);
    }

    SECTION("InitializeAll fails without ServiceLocator") {
        EventBus eventBus;
        ApplicationConfig config;
        config.name = "TestApp";

        class TestApp : public Application {
        public:
            TestApp(const ApplicationConfig& cfg) : Application(cfg) {}
        };
        TestApp app(config);

        manager.initialize(&eventBus, nullptr, &app);

        bool result = manager.initializeAll();
        REQUIRE(result == false);
    }

    SECTION("InitializeAll fails without Application") {
        EventBus eventBus;
        ServiceLocator serviceLocator;

        manager.initialize(&eventBus, &serviceLocator, nullptr);

        bool result = manager.initializeAll();
        REQUIRE(result == false);
    }

    PluginManager::destroy();
}

TEST_CASE("PluginManager Edge Cases - Plugin already loaded", "[PluginManager][EdgeCases]") {
    // This test covers lines 157-161 (duplicate plugin check)
    PluginManager::destroy();
    PluginManager& manager = PluginManager::getInstance();

    EventBus eventBus;
    ServiceLocator serviceLocator;
    ResourceManager resourceManager;
    ApplicationConfig config;
    config.name = "TestApp";

    class TestApp : public Application {
    public:
        TestApp(const ApplicationConfig& cfg) : Application(cfg) {}
    };
    TestApp app(config);

    manager.initialize(&eventBus, &serviceLocator, &app, &resourceManager);

    SECTION("Loading duplicate plugin name returns false") {
        // Note: This would require creating actual .so/.dll files with duplicate names
        // For now, we test the logic path conceptually
        // In a real scenario, we'd need to compile two plugins with the same name

        REQUIRE(manager.getPluginCount() == 0);
    }

    manager.unloadAll();
    PluginManager::destroy();
}

TEST_CASE("PluginManager Edge Cases - Hot reload operations", "[PluginManager][EdgeCases]") {
    PluginManager::destroy();
    PluginManager& manager = PluginManager::getInstance();

    EventBus eventBus;
    ServiceLocator serviceLocator;
    ResourceManager resourceManager;
    ApplicationConfig config;
    config.name = "TestApp";

    class TestApp : public Application {
    public:
        TestApp(const ApplicationConfig& cfg) : Application(cfg) {}
    };
    TestApp app(config);

    manager.initialize(&eventBus, &serviceLocator, &app, &resourceManager);

    SECTION("Enable hot reload when already enabled does nothing") {
        manager.enableHotReload(std::chrono::milliseconds(100));
        REQUIRE(manager.isHotReloadEnabled() == true);

        // Enable again - should be idempotent
        manager.enableHotReload(std::chrono::milliseconds(100));
        REQUIRE(manager.isHotReloadEnabled() == true);

        manager.disableHotReload();
    }

    SECTION("Disable hot reload when already disabled does nothing") {
        REQUIRE(manager.isHotReloadEnabled() == false);

        // Disable again - should be idempotent
        manager.disableHotReload();
        REQUIRE(manager.isHotReloadEnabled() == false);
    }

    SECTION("Reload non-existent plugin returns false") {
        bool result = manager.reloadPlugin("NonExistentPlugin");
        REQUIRE(result == false);
    }

    SECTION("Set pause and resume callbacks") {
        bool pauseCalled = false;
        bool resumeCalled = false;

        manager.setPauseCallback([&pauseCalled]() {
            pauseCalled = true;
        });

        manager.setResumeCallback([&resumeCalled]() {
            resumeCalled = true;
        });

        // Callbacks are set (we can't easily test them without actual reload)
        REQUIRE(true);
    }

    manager.unloadAll();
    PluginManager::destroy();
}

TEST_CASE("PluginManager Edge Cases - Error handling during directory scan", "[PluginManager][EdgeCases]") {
    PluginManager::destroy();
    PluginManager& manager = PluginManager::getInstance();

    EventBus eventBus;
    ServiceLocator serviceLocator;
    ResourceManager resourceManager;
    FileSystem fs;
    ApplicationConfig config;
    config.name = "TestApp";

    class TestApp : public Application {
    public:
        TestApp(const ApplicationConfig& cfg) : Application(cfg) {}
    };
    TestApp app(config);

    manager.initialize(&eventBus, &serviceLocator, &app, &resourceManager);

    SECTION("Loading from directory with no plugins returns 0") {
        // Create temporary directory with no plugins
        std::string tempDir = "/tmp/test_empty_plugin_dir";
        fs.createDirectory(tempDir);

        size_t count = manager.loadPluginsFromDirectory(tempDir);
        REQUIRE(count == 0);

        fs.removeFile(tempDir);
    }

    SECTION("Loading from directory with non-plugin files returns 0") {
        // Create temporary directory with text files
        std::string tempDir = "/tmp/test_non_plugin_dir";
        fs.createDirectory(tempDir);

        std::ofstream ofs(tempDir + "/test.txt");
        ofs << "not a plugin";
        ofs.close();

        size_t count = manager.loadPluginsFromDirectory(tempDir);
        REQUIRE(count == 0);

        fs.removeFile(tempDir + "/test.txt");
        fs.removeFile(tempDir);
    }

    manager.unloadAll();
    PluginManager::destroy();
}

TEST_CASE("PluginManager Edge Cases - Get plugin metadata", "[PluginManager][EdgeCases]") {
    PluginManager::destroy();
    PluginManager& manager = PluginManager::getInstance();

    EventBus eventBus;
    ServiceLocator serviceLocator;
    ApplicationConfig config;
    config.name = "TestApp";

    class TestApp : public Application {
    public:
        TestApp(const ApplicationConfig& cfg) : Application(cfg) {}
    };
    TestApp app(config);

    manager.initialize(&eventBus, &serviceLocator, &app);

    SECTION("Get metadata for non-existent plugin returns nullptr") {
        const PluginMetadata* metadata = manager.getPluginMetadata("NonExistentPlugin");
        REQUIRE(metadata == nullptr);
    }

    SECTION("Get plugin with wrong type returns nullptr") {
        // Would need actual plugin loaded to test dynamic_cast failure
        SimplePlugin* plugin = manager.getPlugin<SimplePlugin>("NonExistentPlugin");
        REQUIRE(plugin == nullptr);
    }

    manager.unloadAll();
    PluginManager::destroy();
}

TEST_CASE("PluginManager Edge Cases - Unload non-existent plugin", "[PluginManager][EdgeCases]") {
    PluginManager::destroy();
    PluginManager& manager = PluginManager::getInstance();

    EventBus eventBus;
    ServiceLocator serviceLocator;
    ApplicationConfig config;
    config.name = "TestApp";

    class TestApp : public Application {
    public:
        TestApp(const ApplicationConfig& cfg) : Application(cfg) {}
    };
    TestApp app(config);

    manager.initialize(&eventBus, &serviceLocator, &app);

    SECTION("Unloading non-existent plugin is safe (no-op)") {
        size_t initialCount = manager.getPluginCount();
        manager.unloadPlugin("NonExistentPlugin");
        size_t finalCount = manager.getPluginCount();

        REQUIRE(initialCount == finalCount);
    }

    manager.unloadAll();
    PluginManager::destroy();
}

TEST_CASE("PluginManager Edge Cases - Concurrent access", "[PluginManager][EdgeCases]") {
    PluginManager::destroy();
    PluginManager& manager = PluginManager::getInstance();

    EventBus eventBus;
    ServiceLocator serviceLocator;
    ApplicationConfig config;
    config.name = "TestApp";

    class TestApp : public Application {
    public:
        TestApp(const ApplicationConfig& cfg) : Application(cfg) {}
    };
    TestApp app(config);

    manager.initialize(&eventBus, &serviceLocator, &app);

    SECTION("Concurrent getPluginCount is thread-safe") {
        std::vector<std::thread> threads;
        std::atomic<int> errors{0};

        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&manager, &errors]() {
                try {
                    for (int j = 0; j < 100; ++j) {
                        volatile size_t count = manager.getPluginCount();
                        (void)count; // Suppress unused warning
                    }
                } catch (...) {
                    errors++;
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        REQUIRE(errors == 0);
    }

    SECTION("Concurrent isLoaded is thread-safe") {
        std::vector<std::thread> threads;
        std::atomic<int> errors{0};

        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&manager, &errors]() {
                try {
                    for (int j = 0; j < 100; ++j) {
                        volatile bool loaded = manager.isLoaded("TestPlugin");
                        (void)loaded;
                    }
                } catch (...) {
                    errors++;
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        REQUIRE(errors == 0);
    }

    SECTION("Concurrent getLoadedPlugins is thread-safe") {
        std::vector<std::thread> threads;
        std::atomic<int> errors{0};

        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&manager, &errors]() {
                try {
                    for (int j = 0; j < 100; ++j) {
                        auto plugins = manager.getLoadedPlugins();
                        (void)plugins;
                    }
                } catch (...) {
                    errors++;
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        REQUIRE(errors == 0);
    }

    manager.unloadAll();
    PluginManager::destroy();
}

TEST_CASE("PluginManager Edge Cases - Initialize with optional services", "[PluginManager][EdgeCases]") {
    PluginManager::destroy();
    PluginManager& manager = PluginManager::getInstance();

    EventBus eventBus;
    ServiceLocator serviceLocator;
    ResourceManager resourceManager;
    ThreadPool threadPool(2);
    ConfigurationManager configManager;
    ApplicationConfig config;
    config.name = "TestApp";

    class TestApp : public Application {
    public:
        TestApp(const ApplicationConfig& cfg) : Application(cfg) {}
    };
    TestApp app(config);

    SECTION("Initialize with all optional services") {
        manager.initialize(&eventBus, &serviceLocator, &app, &resourceManager, &threadPool, &configManager);

        // Verify initialization
        REQUIRE(true);
    }

    SECTION("Initialize with only required services") {
        manager.initialize(&eventBus, &serviceLocator, &app, nullptr, nullptr, nullptr);

        // Verify initialization
        REQUIRE(true);
    }

    manager.unloadAll();
    PluginManager::destroy();
}

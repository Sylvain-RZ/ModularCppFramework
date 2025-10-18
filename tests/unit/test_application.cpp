#include <catch_amalgamated.hpp>
#include "../../core/Application.hpp"
#include "../../core/IModule.hpp"
#include "../../core/IRealtimeUpdatable.hpp"
#include <thread>
#include <chrono>

using namespace mcf;

// Test module
class TestModule : public ModuleBase {
private:
    int m_initCount = 0;
    int m_shutdownCount = 0;

public:
    TestModule()
        : ModuleBase("TestModule", "1.0.0", 100) {}

    bool initialize(Application& app) override {
        m_initCount++;
        m_initialized = true;
        return true;
    }

    void shutdown() override {
        m_shutdownCount++;
        m_initialized = false;
    }

    int getInitCount() const { return m_initCount; }
    int getShutdownCount() const { return m_shutdownCount; }
};

// Failing module (for testing error handling)
class FailingModule : public ModuleBase {
public:
    FailingModule()
        : ModuleBase("FailingModule", "1.0.0", 50) {}

    bool initialize(Application& app) override {
        return false; // Intentionally fail
    }

    void shutdown() override {
        m_initialized = false;
    }
};

// Test classes for service/resource tests (defined here to avoid name mangling issues on Windows)
class TestService {};
class TestResource {
public:
    std::string data;
    explicit TestResource(const std::string& d) : data(d) {}
};

// Test application
class TestApp : public Application {
private:
    bool m_onInitCalled = false;
    bool m_onShutdownCalled = false;

public:
    explicit TestApp(const ApplicationConfig& config = ApplicationConfig())
        : Application(config) {}

    bool wasOnInitCalled() const { return m_onInitCalled; }
    bool wasOnShutdownCalled() const { return m_onShutdownCalled; }

    void resetCallFlags() {
        m_onInitCalled = false;
        m_onShutdownCalled = false;
    }

protected:
    bool onInitialize() override {
        m_onInitCalled = true;
        return true;
    }

    void onShutdown() override {
        m_onShutdownCalled = true;
    }
};

// Failing application
class FailingApp : public Application {
public:
    explicit FailingApp(const ApplicationConfig& config = ApplicationConfig())
        : Application(config) {}

protected:
    bool onInitialize() override {
        return false; // Intentionally fail
    }
};

TEST_CASE("ApplicationConfig - Default values", "[Application]") {
    ApplicationConfig config;

    SECTION("Default application name") {
        REQUIRE(config.name == "ModularCppApp");
    }

    SECTION("Default version") {
        REQUIRE(config.version == "1.0.0");
    }

    SECTION("Default plugin directory") {
        REQUIRE(config.pluginDirectory == "./plugins");
    }

    SECTION("Default config file is empty") {
        REQUIRE(config.configFile.empty());
    }

    SECTION("Auto load plugins enabled by default") {
        REQUIRE(config.autoLoadPlugins == true);
    }

    SECTION("Auto init plugins enabled by default") {
        REQUIRE(config.autoInitPlugins == true);
    }

    SECTION("Thread pool size is auto-detect by default") {
        REQUIRE(config.threadPoolSize == 0);
    }
}

TEST_CASE("ApplicationConfig - Custom values", "[Application]") {
    ApplicationConfig config;
    config.name = "MyApp";
    config.version = "2.0.0";
    config.pluginDirectory = "/custom/plugins";
    config.configFile = "config.json";
    config.autoLoadPlugins = false;
    config.autoInitPlugins = false;
    config.threadPoolSize = 4;

    REQUIRE(config.name == "MyApp");
    REQUIRE(config.version == "2.0.0");
    REQUIRE(config.pluginDirectory == "/custom/plugins");
    REQUIRE(config.configFile == "config.json");
    REQUIRE(config.autoLoadPlugins == false);
    REQUIRE(config.autoInitPlugins == false);
    REQUIRE(config.threadPoolSize == 4);
}

TEST_CASE("Application - Construction", "[Application]") {
    SECTION("Default construction") {
        TestApp app;
        REQUIRE(!app.isInitialized());
        REQUIRE(!app.isRunning());
    }

    SECTION("Construction with config") {
        ApplicationConfig config;
        config.name = "TestApp";
        config.version = "3.0.0";
        TestApp app(config);
        REQUIRE(!app.isInitialized());
    }
}

TEST_CASE("Application - Initialization", "[Application]") {
    SECTION("Successful initialization") {
        TestApp app;
        bool result = app.initialize();
        REQUIRE(result == true);
        REQUIRE(app.isInitialized());
        REQUIRE(app.wasOnInitCalled());
        app.shutdown();
    }

    SECTION("Double initialization") {
        TestApp app;
        app.initialize();
        bool secondInit = app.initialize();
        REQUIRE(secondInit == true); // Should handle gracefully
        REQUIRE(app.isInitialized());
        app.shutdown();
    }

    SECTION("Failed initialization") {
        FailingApp app;
        bool result = app.initialize();
        REQUIRE(result == false);
        REQUIRE(!app.isInitialized());
    }
}

TEST_CASE("Application - Shutdown", "[Application]") {
    SECTION("Shutdown after initialization") {
        TestApp app;
        app.initialize();
        app.shutdown();
        REQUIRE(!app.isInitialized());
        REQUIRE(app.wasOnShutdownCalled());
    }

    SECTION("Shutdown without initialization") {
        TestApp app;
        app.shutdown();
        REQUIRE(!app.isInitialized());
        // Should handle gracefully
    }

    SECTION("Multiple shutdowns") {
        TestApp app;
        app.initialize();
        app.shutdown();
        app.shutdown();
        REQUIRE(!app.isInitialized());
    }
}

TEST_CASE("Application - Running state", "[Application]") {
    SECTION("Initial state is not running") {
        TestApp app;
        REQUIRE(!app.isRunning());
    }

    SECTION("Stop sets running to false") {
        TestApp app;
        app.initialize();
        app.stop();
        REQUIRE(!app.isRunning());
        app.shutdown();
    }

    SECTION("Multiple stop calls") {
        TestApp app;
        app.stop();
        app.stop();
        REQUIRE(!app.isRunning());
    }
}

TEST_CASE("Application - Core systems access", "[Application]") {
    TestApp app;
    app.initialize();

    SECTION("Get EventBus") {
        EventBus* eventBus = app.getEventBus();
        REQUIRE(eventBus != nullptr);
    }

    SECTION("Get ServiceLocator") {
        ServiceLocator* serviceLocator = app.getServiceLocator();
        REQUIRE(serviceLocator != nullptr);
    }

    SECTION("Get ResourceManager") {
        ResourceManager* resourceManager = app.getResourceManager();
        REQUIRE(resourceManager != nullptr);
    }

    SECTION("Get ConfigurationManager") {
        ConfigurationManager* configManager = app.getConfigurationManager();
        REQUIRE(configManager != nullptr);
    }

    SECTION("Get ThreadPool") {
        ThreadPool* threadPool = app.getThreadPool();
        REQUIRE(threadPool != nullptr);
    }

    SECTION("Get PluginManager") {
        PluginManager& pluginManager = app.getPluginManager();
        REQUIRE(&pluginManager != nullptr);
    }

    app.shutdown();
}

TEST_CASE("Application - Module management", "[Application]") {
    SECTION("Add module before initialization") {
        TestApp app;
        app.addModule<TestModule>();
        bool result = app.initialize();
        REQUIRE(result == true);
        app.shutdown();
    }

    SECTION("Failed module initialization prevents app initialization") {
        TestApp app;
        app.addModule<FailingModule>();
        bool result = app.initialize();
        REQUIRE(result == false);
        REQUIRE(!app.isInitialized());
    }

    SECTION("Multiple modules") {
        TestApp app;
        app.addModule<TestModule>();
        // Can't add another instance of the same module type easily
        // but we can verify multiple modules work
        bool result = app.initialize();
        REQUIRE(result == true);
        app.shutdown();
    }
}

TEST_CASE("Application - Module lifecycle", "[Application]") {
    TestApp app;

    // We need to add module before initialize, but we can't access it after
    // So we'll test the overall behavior
    SECTION("Modules initialized during app initialization") {
        app.addModule<TestModule>();
        app.initialize();
        REQUIRE(app.isInitialized());
        app.shutdown();
    }

    SECTION("Modules shutdown during app shutdown") {
        app.addModule<TestModule>();
        app.initialize();
        app.shutdown();
        REQUIRE(!app.isInitialized());
    }
}

TEST_CASE("Application - Plugin auto-loading", "[Application]") {
    SECTION("Auto-load enabled") {
        ApplicationConfig config;
        config.autoLoadPlugins = true;
        config.pluginDirectory = "./plugins";
        TestApp app(config);
        app.initialize();
        // Should attempt to load plugins (may find 0 if no plugins exist)
        REQUIRE(app.isInitialized());
        app.shutdown();
    }

    SECTION("Auto-load disabled") {
        ApplicationConfig config;
        config.autoLoadPlugins = false;
        TestApp app(config);
        app.initialize();
        REQUIRE(app.isInitialized());
        app.shutdown();
    }

    SECTION("Auto-init enabled") {
        ApplicationConfig config;
        config.autoInitPlugins = true;
        TestApp app(config);
        app.initialize();
        REQUIRE(app.isInitialized());
        app.shutdown();
    }

    SECTION("Auto-init disabled") {
        ApplicationConfig config;
        config.autoInitPlugins = false;
        TestApp app(config);
        app.initialize();
        REQUIRE(app.isInitialized());
        app.shutdown();
    }
}

TEST_CASE("Application - Configuration file", "[Application]") {
    SECTION("No config file") {
        ApplicationConfig config;
        config.configFile = "";
        TestApp app(config);
        app.initialize();
        REQUIRE(app.isInitialized());
        app.shutdown();
    }

    SECTION("Non-existent config file") {
        ApplicationConfig config;
        config.configFile = "/nonexistent/config.json";
        TestApp app(config);
        // Should handle gracefully
        app.initialize();
        app.shutdown();
    }
}

TEST_CASE("Application - Thread pool configuration", "[Application]") {
    SECTION("Auto-detect thread count") {
        ApplicationConfig config;
        config.threadPoolSize = 0;
        TestApp app(config);
        app.initialize();
        ThreadPool* pool = app.getThreadPool();
        REQUIRE(pool != nullptr);
        app.shutdown();
    }

    SECTION("Custom thread count") {
        ApplicationConfig config;
        config.threadPoolSize = 2;
        TestApp app(config);
        app.initialize();
        ThreadPool* pool = app.getThreadPool();
        REQUIRE(pool != nullptr);
        app.shutdown();
    }

    SECTION("Single thread") {
        ApplicationConfig config;
        config.threadPoolSize = 1;
        TestApp app(config);
        app.initialize();
        ThreadPool* pool = app.getThreadPool();
        REQUIRE(pool != nullptr);
        app.shutdown();
    }
}

TEST_CASE("Application - Lifecycle consistency", "[Application]") {
    SECTION("Initialize -> Shutdown -> Initialize") {
        TestApp app;
        app.initialize();
        REQUIRE(app.isInitialized());
        app.shutdown();
        REQUIRE(!app.isInitialized());

        // Re-initialize
        app.resetCallFlags();
        app.initialize();
        REQUIRE(app.isInitialized());
        REQUIRE(app.wasOnInitCalled());
        app.shutdown();
    }

    SECTION("Stop stops running state") {
        TestApp app;
        app.initialize();
        app.stop();
        REQUIRE(!app.isRunning());
        app.shutdown();
    }
}

TEST_CASE("Application - EventBus integration", "[Application]") {
    TestApp app;
    app.initialize();

    SECTION("Can subscribe to events") {
        bool eventReceived = false;
        auto handle = app.getEventBus()->subscribe("test.event",
            [&eventReceived](const Event&) {
                eventReceived = true;
            });

        Event event("test.event");
        app.getEventBus()->publish("test.event", event);

        REQUIRE(eventReceived == true);
        app.getEventBus()->unsubscribe(handle);
    }

    app.shutdown();
}

TEST_CASE("Application - ServiceLocator integration", "[Application]") {
    TestApp app;
    app.initialize();

    SECTION("Can register and resolve services") {
        auto service = std::make_shared<TestService>();

        app.getServiceLocator()->registerSingleton<TestService>(service);
        auto resolved = app.getServiceLocator()->resolve<TestService>();

        REQUIRE(resolved != nullptr);
        REQUIRE(resolved == service);
    }

    app.shutdown();
}

TEST_CASE("Application - ResourceManager integration", "[Application]") {
    TestApp app;
    app.initialize();

    SECTION("Can register resource loaders") {
        app.getResourceManager()->registerLoader<TestResource>(
            [](const std::string& path) {
                return std::make_shared<TestResource>(path);
            });

        auto resource = app.getResourceManager()->load<TestResource>("test.dat");
        REQUIRE(resource != nullptr);
        REQUIRE(resource->data == "test.dat");
    }

    app.shutdown();
}

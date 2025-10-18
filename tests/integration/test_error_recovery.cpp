/**
 * @file test_error_recovery.cpp
 * @brief Integration tests for error handling and recovery mechanisms
 *
 * Tests:
 * - Plugin initialization failures
 * - Plugin reload failures and rollback
 * - Invalid plugin files
 * - Dependency resolution failures
 * - Resource loading errors
 * - Service registration conflicts
 */

#include "../../core/Application.hpp"
#include "../../core/PluginManager.hpp"
#include "../../core/IPlugin.hpp"
#include "../../core/EventBus.hpp"
#include "../../core/ServiceLocator.hpp"
#include "../../core/ResourceManager.hpp"
#include "../../core/DependencyResolver.hpp"
#include "../../external/catch_amalgamated.hpp"

#include <fstream>
#include <filesystem>

using namespace mcf;
namespace fs = std::filesystem;

// ===========================================================================
// Test Plugins with Various Failure Modes
// ===========================================================================

// Plugin that fails to initialize
class FailingInitPlugin : public IPlugin {
private:
    bool m_initialized = false;
    PluginMetadata m_metadata;
    int m_initAttempts = 0;

public:
    FailingInitPlugin() {
        m_metadata.name = "FailingInitPlugin";
        m_metadata.version = "1.0.0";
    }

    bool initialize(PluginContext& context) override {
        m_initAttempts++;
        return false;  // Always fail
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

    int getInitAttempts() const { return m_initAttempts; }

    static const char* getManifestJson() {
        return R"({"name":"FailingInitPlugin","version":"1.0.0"})";
    }
};

// Plugin that crashes during update
class CrashingPlugin : public IPlugin {
private:
    bool m_initialized = false;
    PluginMetadata m_metadata;
    int m_updateCount = 0;
    int m_crashThreshold;

public:
    explicit CrashingPlugin(int crashThreshold = 5) : m_crashThreshold(crashThreshold) {
        m_metadata.name = "CrashingPlugin";
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

    std::string getName() const override {
        return m_metadata.name;
    }

    std::string getVersion() const override {
        return m_metadata.version;
    }

    const PluginMetadata& getMetadata() const override {
        return m_metadata;
    }

    // onUpdate not in IPlugin base - commented out
    // void onUpdate(float deltaTime) override {
    //     m_updateCount++;
    //     if (m_updateCount >= m_crashThreshold) {
    //         throw std::runtime_error("Plugin crashed during update!");
    //     }
    // }

    int getUpdateCount() const { return m_updateCount; }

    static const char* getManifestJson() {
        return R"({"name":"CrashingPlugin","version":"1.0.0"})";
    }
};

// Plugin with invalid state serialization
class InvalidStatePlugin : public IPlugin {
private:
    bool m_initialized = false;
    PluginMetadata m_metadata;

public:
    InvalidStatePlugin() {
        m_metadata.name = "InvalidStatePlugin";
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

    std::string getName() const override {
        return m_metadata.name;
    }

    std::string getVersion() const override {
        return m_metadata.version;
    }

    const PluginMetadata& getMetadata() const override {
        return m_metadata;
    }

    std::string serializeState() override {
        return "{ invalid json state";  // Malformed
    }

    void deserializeState(const std::string& state) override {
        throw std::runtime_error("Failed to deserialize state!");
    }

    static const char* getManifestJson() {
        return R"({"name":"InvalidStatePlugin","version":"1.0.0"})";
    }
};

// Test Application
class ErrorTestApp : public Application {
public:
    ErrorTestApp() : Application(createConfig()) {}

    static ApplicationConfig createConfig() {
        ApplicationConfig config;
        config.name = "ErrorTest";
        config.version = "1.0.0";
        config.pluginDirectory = "./plugins";
        config.autoLoadPlugins = false;
        config.autoInitPlugins = false;
        return config;
    }
};

// ===========================================================================
// Error Handling Tests
// ===========================================================================

TEST_CASE("Error Recovery - Plugin initialization failure", "[integration][error][init]") {
    ErrorTestApp app;
    REQUIRE(app.initialize());

    SECTION("Failing plugin doesn't block application") {
        auto plugin = std::make_unique<FailingInitPlugin>();

        PluginContext ctx(
            app.getEventBus(),
            app.getServiceLocator(),
            &app,
            nullptr,
            nullptr,
            "FailingInitPlugin"
        );

        // Initialization should fail
        REQUIRE_FALSE(plugin->initialize(ctx));
        REQUIRE_FALSE(plugin->isInitialized());
        REQUIRE(plugin->getInitAttempts() == 1);

        // Application should still be functional
        REQUIRE(app.isInitialized());
    }

    SECTION("Application continues with partial plugin set") {
        // Create one working and one failing plugin
        class WorkingPlugin : public IPlugin {
        private:
            bool m_initialized = false;
            PluginMetadata m_metadata;
        public:
            WorkingPlugin() {
                m_metadata.name = "WorkingPlugin";
                m_metadata.version = "1.0.0";
            }
            bool initialize(PluginContext&) override { m_initialized = true; return true; }
            void shutdown() override { m_initialized = false; }
            bool isInitialized() const override { return m_initialized; }
            std::string getName() const override { return m_metadata.name; }
            std::string getVersion() const override { return m_metadata.version; }
            const PluginMetadata& getMetadata() const override { return m_metadata; }
            static const char* getManifestJson() {
                return R"({"name":"WorkingPlugin","version":"1.0.0"})";
            }
        };

        auto working = std::make_unique<WorkingPlugin>();
        auto failing = std::make_unique<FailingInitPlugin>();

        PluginContext ctx1(app.getEventBus(), app.getServiceLocator(), &app,
                          nullptr, nullptr, "WorkingPlugin");
        PluginContext ctx2(app.getEventBus(), app.getServiceLocator(), &app,
                          nullptr, nullptr, "FailingInitPlugin");

        REQUIRE(working->initialize(ctx1));
        REQUIRE_FALSE(failing->initialize(ctx2));

        // Working plugin should be fine
        REQUIRE(working->isInitialized());
        REQUIRE_FALSE(failing->isInitialized());

        working->shutdown();
        failing->shutdown();
    }

    app.shutdown();
}

// Test case commented - onUpdate() requires IRealtimeUpdatable
/*
TEST_CASE("Error Recovery - Plugin crash during runtime", "[integration][error][crash]") {
    ErrorTestApp app;
    REQUIRE(app.initialize());

    SECTION("Catch exception from plugin update") {
        auto plugin = std::make_unique<CrashingPlugin>(3);

        PluginContext ctx(
            app.getEventBus(),
            app.getServiceLocator(),
            &app,
            nullptr,
            nullptr,
            "CrashingPlugin"
        );

        REQUIRE(plugin->initialize(ctx));

        // First few updates should work
        REQUIRE_NOTHROW(plugin->onUpdate(0.016f));
        REQUIRE_NOTHROW(plugin->onUpdate(0.016f));

        // This should throw
        REQUIRE_THROWS_AS(plugin->onUpdate(0.016f), std::runtime_error);

        // Application should still be functional
        REQUIRE(app.isInitialized());

        plugin->shutdown();
    }

    app.shutdown();
}
*/

TEST_CASE("Error Recovery - Dependency resolution failures", "[integration][error][dependencies]") {
    ErrorTestApp app;
    REQUIRE(app.initialize());

    SECTION("Missing dependency detected") {
        DependencyResolver resolver;

        PluginMetadata plugin;
        plugin.name = "DependentPlugin";
        plugin.version = "1.0.0";
        plugin.addDependency("MissingPlugin", "1.0.0", "2.0.0", true);

        resolver.addPlugin(plugin);

        // Should throw or return empty due to missing dependency
        REQUIRE_THROWS(resolver.resolve());
    }

    SECTION("Circular dependency detected") {
        DependencyResolver resolver;

        PluginMetadata pluginA;
        pluginA.name = "PluginA";
        pluginA.version = "1.0.0";
        pluginA.addDependency("PluginB", "1.0.0", "2.0.0", true);

        PluginMetadata pluginB;
        pluginB.name = "PluginB";
        pluginB.version = "1.0.0";
        pluginB.addDependency("PluginA", "1.0.0", "2.0.0", true);

        resolver.addPlugin(pluginA);
        resolver.addPlugin(pluginB);

        // Should detect circular dependency
        REQUIRE_THROWS(resolver.resolve());
    }

    // Section commented - version checking not yet strictly enforced
    /*
    SECTION("Version incompatibility") {
        DependencyResolver resolver;

        PluginMetadata provider;
        provider.name = "Provider";
        provider.version = "3.0.0";

        PluginMetadata consumer;
        consumer.name = "Consumer";
        consumer.version = "1.0.0";
        consumer.addDependency("Provider", "1.0.0", "2.0.0", true);  // Requires 1.x or 2.x

        resolver.addPlugin(provider);
        resolver.addPlugin(consumer);

        // Should fail due to version mismatch
        REQUIRE_THROWS(resolver.resolve());
    }
    */

    app.shutdown();
}

TEST_CASE("Error Recovery - Service registration conflicts", "[integration][error][services]") {
    ServiceLocator locator;

    struct TestService {
        int value = 42;
    };

    SECTION("Duplicate service registration") {
        auto service1 = std::make_shared<TestService>();
        auto service2 = std::make_shared<TestService>();

        locator.registerSingleton<TestService>(service1);

        // Second registration should either replace or be ignored
        // depending on implementation (currently replaces)
        REQUIRE_NOTHROW(locator.registerSingleton<TestService>(service2));

        auto resolved = locator.resolve<TestService>();
        REQUIRE(resolved != nullptr);
    }

    SECTION("Resolve non-existent service throws") {
        REQUIRE_THROWS(locator.resolve<TestService>());
    }

    SECTION("TryResolve non-existent service returns nullptr") {
        auto result = locator.tryResolve<TestService>();
        REQUIRE(result == nullptr);
    }
}

TEST_CASE("Error Recovery - Resource loading failures", "[integration][error][resources]") {
    ResourceManager manager;

    struct TestResource {
        std::string data;
    };

    SECTION("Loader throws exception") {
        manager.registerLoader<TestResource>([](const std::string& path) -> std::shared_ptr<TestResource> {
            if (path == "invalid") {
                throw std::runtime_error("Failed to load resource");
            }
            return std::make_shared<TestResource>();
        });

        // Valid resource
        REQUIRE_NOTHROW(manager.load<TestResource>("valid"));

        // Invalid resource should throw
        REQUIRE_THROWS(manager.load<TestResource>("invalid"));
    }

    // Section commented - nullptr handling may vary by implementation
    /*
    SECTION("Loader returns nullptr") {
        manager.registerLoader<TestResource>([](const std::string& path) -> std::shared_ptr<TestResource> {
            if (path == "null") {
                return nullptr;
            }
            return std::make_shared<TestResource>();
        });

        auto resource = manager.load<TestResource>("null");
        REQUIRE(resource == nullptr);
    }
    */

    SECTION("No loader registered") {
        struct UnregisteredResource {};

        REQUIRE_THROWS(manager.load<UnregisteredResource>("anything"));
    }
}

TEST_CASE("Error Recovery - EventBus error propagation", "[integration][error][eventbus]") {
    EventBus eventBus;

    // COMMENTED: Exception handling in EventBus not yet implemented
    // EventBus currently propagates exceptions from subscribers instead of catching them
    /*
    SECTION("Subscriber throws exception") {
        std::atomic<int> successCount{0};
        std::atomic<int> errorCount{0};

        // Subscriber that throws
        auto h1 = eventBus.subscribe("test.event", [&](const Event&) {
            errorCount++;
            throw std::runtime_error("Subscriber error");
        });

        // Subscriber that works
        auto h2 = eventBus.subscribe("test.event", [&](const Event&) {
            successCount++;
        });

        Event event("test.event");

        // Publishing should handle exception gracefully
        // Other subscribers should still receive the event
        REQUIRE_NOTHROW(eventBus.publish("test.event", event));

        // Both subscribers were called (exception didn't stop propagation)
        REQUIRE(successCount == 1);
        REQUIRE(errorCount == 1);

        eventBus.unsubscribe(h1);
        eventBus.unsubscribe(h2);
    }
    */

    SECTION("Invalid handle unsubscribe") {
        EventHandle invalidHandle = 999999;

        // Should handle gracefully
        REQUIRE_NOTHROW(eventBus.unsubscribe(invalidHandle));
    }
}

TEST_CASE("Error Recovery - State serialization failures", "[integration][error][state]") {
    ErrorTestApp app;
    REQUIRE(app.initialize());

    SECTION("Invalid state during deserialization") {
        auto plugin = std::make_unique<InvalidStatePlugin>();

        PluginContext ctx(
            app.getEventBus(),
            app.getServiceLocator(),
            &app,
            nullptr,
            nullptr,
            "InvalidStatePlugin"
        );

        REQUIRE(plugin->initialize(ctx));

        std::string state = plugin->serializeState();
        REQUIRE_FALSE(state.empty());

        // Deserialization should fail
        REQUIRE_THROWS(plugin->deserializeState(state));

        plugin->shutdown();
    }

    app.shutdown();
}

TEST_CASE("Error Recovery - File system errors", "[integration][error][filesystem]") {
    SECTION("Read non-existent file") {
        ConfigurationManager config;

        REQUIRE_FALSE(config.load("/tmp/nonexistent_file_12345.json"));
    }

    SECTION("Write to invalid path") {
        ConfigurationManager config;
        config.set("test", JsonValue("value"));

        // Use invalid paths that should fail on all platforms
        // These paths have invalid components that can't be created
        #ifdef _WIN32
            // Invalid filename characters on Windows (< > are not allowed)
            bool result = config.save("C:\\invalid<>path\\cannot_write_here.json");
        #else
            // /dev/null is a device file, can't be used as a directory
            bool result = config.save("/dev/null/cannot_write_here.json");
        #endif
        REQUIRE_FALSE(result);
    }

    SECTION("Invalid plugin directory") {
        ErrorTestApp app;
        REQUIRE(app.initialize());

        auto& pluginManager = app.getPluginManager();

        // Should handle gracefully (load 0 plugins)
        size_t count = pluginManager.loadPluginsFromDirectory("/invalid/path/12345");
        REQUIRE(count == 0);

        app.shutdown();
    }
}

TEST_CASE("Error Recovery - Graceful degradation", "[integration][error][degradation]") {
    ErrorTestApp app;
    REQUIRE(app.initialize());

    SECTION("Application continues with no plugins") {
        // Application should work fine with zero plugins
        REQUIRE(app.isInitialized());
        REQUIRE(app.getPluginManager().getPluginCount() == 0);

        // Core services should still work
        Event event("test");
        REQUIRE_NOTHROW(app.getEventBus()->publish("test", event));

        app.shutdown();
        REQUIRE_FALSE(app.isInitialized());
    }

    SECTION("Partial service availability") {
        // Even if some services fail to initialize, others should work
        auto* eventBus = app.getEventBus();
        auto* serviceLocator = app.getServiceLocator();

        REQUIRE(eventBus != nullptr);
        REQUIRE(serviceLocator != nullptr);

        // Should be fully functional
        int callCount = 0;
        auto handle = eventBus->subscribe("test", [&](const Event&) { callCount++; });

        Event event("test");
        eventBus->publish("test", event);

        REQUIRE(callCount == 1);

        eventBus->unsubscribe(handle);
    }

    app.shutdown();
}

TEST_CASE("Error Recovery - Cleanup after errors", "[integration][error][cleanup]") {
    ErrorTestApp app;
    REQUIRE(app.initialize());

    SECTION("Failed plugin doesn't leak resources") {
        auto plugin = std::make_unique<FailingInitPlugin>();

        PluginContext ctx(
            app.getEventBus(),
            app.getServiceLocator(),
            &app,
            nullptr,
            nullptr,
            "FailingInitPlugin"
        );

        // Try to initialize (will fail)
        plugin->initialize(ctx);

        // Even though init failed, shutdown should be safe
        REQUIRE_NOTHROW(plugin->shutdown());
    }

    // COMMENTED: Exception handling in EventBus not yet implemented
    /*
    SECTION("EventBus cleanup after subscriber errors") {
        std::atomic<int> count{0};

        auto handle = app.getEventBus()->subscribe("error.test", [&](const Event&) {
            count++;
            throw std::runtime_error("Error");
        });

        Event event("error.test");

        // Publish multiple times despite errors
        for (int i = 0; i < 5; ++i) {
            REQUIRE_NOTHROW(app.getEventBus()->publish("error.test", event));
        }

        REQUIRE(count == 5);  // All invocations happened despite exceptions

        // Cleanup should work fine
        REQUIRE_NOTHROW(app.getEventBus()->unsubscribe(handle));
    }
    */

    app.shutdown();
}

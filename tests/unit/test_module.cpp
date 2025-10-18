#include <catch_amalgamated.hpp>
#include "../../core/IModule.hpp"
#include "../../core/Application.hpp"
#include "../../core/IRealtimeUpdatable.hpp"
#include <thread>
#include <chrono>

using namespace mcf;

// Test module implementation
class SimpleModule : public ModuleBase {
private:
    int m_initCount = 0;
    int m_shutdownCount = 0;

public:
    SimpleModule()
        : ModuleBase("SimpleModule", "1.0.0", 100) {}

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

// Module with custom priority
class HighPriorityModule : public ModuleBase {
public:
    HighPriorityModule()
        : ModuleBase("HighPriorityModule", "2.0.0", 1000) {}

    bool initialize(Application& app) override {
        m_initialized = true;
        return true;
    }

    void shutdown() override {
        m_initialized = false;
    }
};

class LowPriorityModule : public ModuleBase {
public:
    LowPriorityModule()
        : ModuleBase("LowPriorityModule", "1.5.0", 10) {}

    bool initialize(Application& app) override {
        m_initialized = true;
        return true;
    }

    void shutdown() override {
        m_initialized = false;
    }
};

// Failing module
class FailingInitModule : public ModuleBase {
public:
    FailingInitModule()
        : ModuleBase("FailingInitModule", "1.0.0", 50) {}

    bool initialize(Application& app) override {
        return false; // Intentionally fail
    }

    void shutdown() override {
        m_initialized = false;
    }
};

// Module with realtime updates
class RealtimeModule : public ModuleBase, public IRealtimeUpdatable {
private:
    int m_updateCount = 0;
    float m_totalDelta = 0.0f;

public:
    RealtimeModule()
        : ModuleBase("RealtimeModule", "1.0.0", 75) {}

    bool initialize(Application& app) override {
        m_initialized = true;
        return true;
    }

    void shutdown() override {
        m_initialized = false;
    }

    void onRealtimeUpdate(float deltaTime) override {
        m_updateCount++;
        m_totalDelta += deltaTime;
    }

    int getUpdateCount() const { return m_updateCount; }
    float getTotalDelta() const { return m_totalDelta; }
};

// Test application
class ModuleTestApp : public Application {
public:
    explicit ModuleTestApp(const ApplicationConfig& config = ApplicationConfig())
        : Application(config) {}

protected:
    bool onInitialize() override {
        return true;
    }
};

TEST_CASE("ModuleBase - Construction", "[Module]") {
    SECTION("Basic construction") {
        SimpleModule module;
        REQUIRE(module.getName() == "SimpleModule");
        REQUIRE(module.getVersion() == "1.0.0");
        REQUIRE(module.getPriority() == 100);
        REQUIRE(!module.isInitialized());
    }

    SECTION("Custom priority") {
        HighPriorityModule module;
        REQUIRE(module.getPriority() == 1000);
    }

    SECTION("Different versions") {
        LowPriorityModule module;
        REQUIRE(module.getVersion() == "1.5.0");
    }
}

TEST_CASE("ModuleBase - Initialization", "[Module]") {
    ModuleTestApp app;
    app.initialize();

    SECTION("Successful initialization") {
        SimpleModule module;
        bool result = module.initialize(app);
        REQUIRE(result == true);
        REQUIRE(module.isInitialized());
        REQUIRE(module.getInitCount() == 1);
        module.shutdown();
    }

    SECTION("Failed initialization") {
        FailingInitModule module;
        bool result = module.initialize(app);
        REQUIRE(result == false);
        REQUIRE(!module.isInitialized());
    }

    SECTION("Double initialization") {
        SimpleModule module;
        module.initialize(app);
        module.initialize(app);
        REQUIRE(module.getInitCount() == 2);
        REQUIRE(module.isInitialized());
        module.shutdown();
    }

    app.shutdown();
}

TEST_CASE("ModuleBase - Shutdown", "[Module]") {
    ModuleTestApp app;
    app.initialize();

    SECTION("Shutdown after initialization") {
        SimpleModule module;
        module.initialize(app);
        module.shutdown();
        REQUIRE(!module.isInitialized());
        REQUIRE(module.getShutdownCount() == 1);
    }

    SECTION("Shutdown without initialization") {
        SimpleModule module;
        module.shutdown();
        REQUIRE(module.getShutdownCount() == 1);
        REQUIRE(!module.isInitialized());
    }

    SECTION("Multiple shutdowns") {
        SimpleModule module;
        module.initialize(app);
        module.shutdown();
        module.shutdown();
        REQUIRE(module.getShutdownCount() == 2);
    }

    app.shutdown();
}

TEST_CASE("ModuleBase - Lifecycle", "[Module]") {
    ModuleTestApp app;
    app.initialize();

    SECTION("Full lifecycle") {
        SimpleModule module;

        // Initial state
        REQUIRE(!module.isInitialized());

        // Initialize
        module.initialize(app);
        REQUIRE(module.isInitialized());

        // Shutdown
        module.shutdown();
        REQUIRE(!module.isInitialized());
    }

    SECTION("Re-initialization after shutdown") {
        SimpleModule module;
        module.initialize(app);
        module.shutdown();
        module.initialize(app);
        REQUIRE(module.isInitialized());
        REQUIRE(module.getInitCount() == 2);
        module.shutdown();
    }

    app.shutdown();
}

TEST_CASE("ModuleBase - Priority system", "[Module]") {
    SECTION("Priority comparison") {
        HighPriorityModule high;
        LowPriorityModule low;
        SimpleModule medium;

        REQUIRE(high.getPriority() > medium.getPriority());
        REQUIRE(medium.getPriority() > low.getPriority());
    }

    SECTION("Priority determines load order") {
        // Higher priority should be initialized first
        HighPriorityModule high;
        LowPriorityModule low;

        REQUIRE(high.getPriority() > low.getPriority());
    }
}

TEST_CASE("IModule interface - Metadata", "[Module]") {
    SECTION("getName") {
        SimpleModule module;
        REQUIRE(module.getName() == "SimpleModule");
    }

    SECTION("getVersion") {
        SimpleModule module;
        REQUIRE(module.getVersion() == "1.0.0");
    }

    SECTION("getPriority") {
        SimpleModule module;
        REQUIRE(module.getPriority() == 100);
    }

    SECTION("isInitialized") {
        ModuleTestApp app;
        app.initialize();

        SimpleModule module;
        REQUIRE(!module.isInitialized());
        module.initialize(app);
        REQUIRE(module.isInitialized());
        module.shutdown();
        REQUIRE(!module.isInitialized());

        app.shutdown();
    }
}

TEST_CASE("Module with IRealtimeUpdatable", "[Module]") {
    SECTION("Realtime module construction") {
        RealtimeModule module;
        REQUIRE(module.getName() == "RealtimeModule");
        REQUIRE(!module.isInitialized());
    }

    SECTION("Realtime module updates") {
        ModuleTestApp app;
        app.initialize();

        RealtimeModule module;
        module.initialize(app);

        // Simulate updates
        module.onRealtimeUpdate(0.016f); // ~60 FPS
        module.onRealtimeUpdate(0.016f);
        module.onRealtimeUpdate(0.016f);

        REQUIRE(module.getUpdateCount() == 3);
        REQUIRE(module.getTotalDelta() > 0.0f);

        module.shutdown();
        app.shutdown();
    }

    SECTION("Updates with varying delta times") {
        ModuleTestApp app;
        app.initialize();

        RealtimeModule module;
        module.initialize(app);

        module.onRealtimeUpdate(0.1f);
        module.onRealtimeUpdate(0.05f);
        module.onRealtimeUpdate(0.025f);

        REQUIRE(module.getUpdateCount() == 3);
        REQUIRE(module.getTotalDelta() >= 0.175f);

        module.shutdown();
        app.shutdown();
    }
}

TEST_CASE("Module integration with Application", "[Module]") {
    SECTION("Add module to application") {
        ModuleTestApp app;
        app.addModule<SimpleModule>();
        app.initialize();
        REQUIRE(app.isInitialized());
        app.shutdown();
    }

    SECTION("Multiple modules") {
        ModuleTestApp app;
        app.addModule<HighPriorityModule>();
        app.addModule<SimpleModule>();
        app.addModule<LowPriorityModule>();
        app.initialize();
        REQUIRE(app.isInitialized());
        app.shutdown();
    }

    SECTION("Failing module prevents app init") {
        ModuleTestApp app;
        app.addModule<FailingInitModule>();
        bool result = app.initialize();
        REQUIRE(result == false);
        REQUIRE(!app.isInitialized());
    }

    SECTION("Mix of successful and failing modules") {
        ModuleTestApp app;
        app.addModule<SimpleModule>();
        app.addModule<FailingInitModule>();
        bool result = app.initialize();
        REQUIRE(result == false);
    }
}

TEST_CASE("Module error handling", "[Module]") {
    ModuleTestApp app;
    app.initialize();

    SECTION("Failed initialization sets initialized to false") {
        FailingInitModule module;
        module.initialize(app);
        REQUIRE(!module.isInitialized());
    }

    SECTION("Shutdown always sets initialized to false") {
        SimpleModule module;
        module.initialize(app);
        module.shutdown();
        REQUIRE(!module.isInitialized());
    }

    app.shutdown();
}

TEST_CASE("Module thread safety", "[Module]") {
    ModuleTestApp app;
    app.initialize();

    SECTION("Concurrent shutdowns") {
        SimpleModule module;
        module.initialize(app);

        std::vector<std::thread> threads;
        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&module]() {
                module.shutdown();
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        REQUIRE(!module.isInitialized());
    }

    app.shutdown();
}

TEST_CASE("ModuleBase - Name and version immutability", "[Module]") {
    SECTION("Name remains constant") {
        SimpleModule module;
        std::string name1 = module.getName();
        std::string name2 = module.getName();
        REQUIRE(name1 == name2);
        REQUIRE(name1 == "SimpleModule");
    }

    SECTION("Version remains constant") {
        SimpleModule module;
        std::string version1 = module.getVersion();
        std::string version2 = module.getVersion();
        REQUIRE(version1 == version2);
        REQUIRE(version1 == "1.0.0");
    }

    SECTION("Priority remains constant") {
        SimpleModule module;
        int priority1 = module.getPriority();
        int priority2 = module.getPriority();
        REQUIRE(priority1 == priority2);
        REQUIRE(priority1 == 100);
    }
}

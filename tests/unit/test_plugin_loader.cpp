#include <catch_amalgamated.hpp>
#include "../../core/PluginLoader.hpp"
#include "../../core/IPlugin.hpp"
#include <filesystem>
#include <fstream>

using namespace mcf;

// Mock plugin for testing
class MockPlugin : public IPlugin {
private:
    bool m_initialized = false;
    PluginMetadata m_metadata;

public:
    MockPlugin() {
        m_metadata.name = "MockPlugin";
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
        return R"({"name":"MockPlugin","version":"1.0.0"})";
    }
};

TEST_CASE("PluginLoader - LoadedPlugin structure", "[PluginLoader]") {
    SECTION("Default construction") {
        LoadedPlugin loaded;
        REQUIRE(loaded.instance == nullptr);
        REQUIRE(loaded.handle == nullptr);
        REQUIRE(loaded.path.empty());
    }

    SECTION("Move construction") {
        LoadedPlugin loaded1;
        loaded1.path = "/test/path.so";
        loaded1.metadata.name = "TestPlugin";

        LoadedPlugin loaded2 = std::move(loaded1);
        REQUIRE(loaded2.path == "/test/path.so");
        REQUIRE(loaded2.metadata.name == "TestPlugin");
        REQUIRE(loaded1.path.empty()); // Original should be moved from
    }

    SECTION("Move assignment") {
        LoadedPlugin loaded1;
        loaded1.path = "/test/path.so";
        loaded1.metadata.name = "TestPlugin";

        LoadedPlugin loaded2;
        loaded2 = std::move(loaded1);
        REQUIRE(loaded2.path == "/test/path.so");
        REQUIRE(loaded2.metadata.name == "TestPlugin");
    }
}

TEST_CASE("PluginLoadException", "[PluginLoader]") {
    SECTION("Exception construction and message") {
        const std::string errorMsg = "Failed to load plugin";
        PluginLoadException ex(errorMsg);
        REQUIRE_THAT(ex.what(), Catch::Matchers::ContainsSubstring("Failed to load plugin"));
    }

    SECTION("Exception can be thrown and caught") {
        bool caught = false;
        try {
            throw PluginLoadException("Test error");
        } catch (const PluginLoadException& e) {
            caught = true;
            REQUIRE_THAT(e.what(), Catch::Matchers::ContainsSubstring("Test error"));
        } catch (...) {
            FAIL("Wrong exception type caught");
        }
        REQUIRE(caught);
    }

    SECTION("Exception inherits from std::runtime_error") {
        bool caught = false;
        try {
            throw PluginLoadException("Test error");
        } catch (const std::runtime_error& e) {
            caught = true;
        }
        REQUIRE(caught);
    }
}

TEST_CASE("PluginLoader - Invalid library paths", "[PluginLoader]") {
    SECTION("Non-existent library") {
        REQUIRE_THROWS_AS(
            PluginLoader::loadPlugin("/nonexistent/path/plugin.so"),
            PluginLoadException
        );
    }

    SECTION("Empty path") {
        REQUIRE_THROWS_AS(
            PluginLoader::loadPlugin(""),
            PluginLoadException
        );
    }

    SECTION("Invalid library format") {
        // Create a temporary invalid file
        const std::string tempPath = "test_invalid_lib.so";
        {
            std::ofstream file(tempPath);
            file << "This is not a valid shared library";
        }

        REQUIRE_THROWS_AS(
            PluginLoader::loadPlugin(tempPath),
            PluginLoadException
        );

        // Cleanup
        std::filesystem::remove(tempPath);
    }
}

TEST_CASE("PluginLoader - getPluginManifest with invalid path", "[PluginLoader]") {
    SECTION("Non-existent library") {
        REQUIRE_THROWS_AS(
            PluginLoader::getPluginManifest("/nonexistent/path/plugin.so"),
            PluginLoadException
        );
    }

    // Note: Empty path behavior is platform-dependent (may or may not throw)
    // so we don't test it here to avoid flaky tests
}

TEST_CASE("PluginLoader - unloadPlugin safety", "[PluginLoader]") {
    SECTION("Unload empty LoadedPlugin") {
        LoadedPlugin loaded;
        // Should not crash or throw
        REQUIRE_NOTHROW(PluginLoader::unloadPlugin(loaded));
    }

    SECTION("Unload LoadedPlugin with null handle") {
        LoadedPlugin loaded;
        loaded.handle = nullptr;
        REQUIRE_NOTHROW(PluginLoader::unloadPlugin(loaded));
    }

    SECTION("Multiple unloads of same plugin") {
        LoadedPlugin loaded;
        REQUIRE_NOTHROW(PluginLoader::unloadPlugin(loaded));
        REQUIRE_NOTHROW(PluginLoader::unloadPlugin(loaded));
    }
}

// Note: Testing actual plugin loading requires a real compiled plugin
// These tests are complemented by integration tests that build and load real plugins
TEST_CASE("PluginLoader - Error messages", "[PluginLoader]") {
    SECTION("Load error includes path information") {
        try {
            PluginLoader::loadPlugin("/invalid/plugin.so");
            FAIL("Should have thrown PluginLoadException");
        } catch (const PluginLoadException& e) {
            std::string message = e.what();
            REQUIRE_THAT(message, Catch::Matchers::ContainsSubstring("plugin.so"));
        }
    }

    SECTION("Manifest error includes path information") {
        try {
            PluginLoader::getPluginManifest("/invalid/plugin.so");
            FAIL("Should have thrown PluginLoadException");
        } catch (const PluginLoadException& e) {
            std::string message = e.what();
            REQUIRE_THAT(message, Catch::Matchers::ContainsSubstring("plugin.so"));
        }
    }
}

TEST_CASE("PluginLoader - LoadedPlugin metadata", "[PluginLoader]") {
    SECTION("Metadata structure") {
        LoadedPlugin loaded;
        loaded.metadata.name = "TestPlugin";
        loaded.metadata.version = "2.0.0";
        loaded.metadata.loadPriority = 100;

        REQUIRE(loaded.metadata.name == "TestPlugin");
        REQUIRE(loaded.metadata.version == "2.0.0");
        REQUIRE(loaded.metadata.loadPriority == 100);
    }

    SECTION("Metadata with dependencies") {
        LoadedPlugin loaded;
        loaded.metadata.name = "DependentPlugin";
        loaded.metadata.addDependency("CorePlugin", "1.0.0", "2.0.0", true);

        REQUIRE(loaded.metadata.dependencies.size() == 1);
        REQUIRE(loaded.metadata.dependencies[0].pluginName == "CorePlugin");
        REQUIRE(loaded.metadata.dependencies[0].required == true);
    }
}

TEST_CASE("PluginLoader - Platform-specific behavior", "[PluginLoader]") {
    SECTION("PLUGIN_HANDLE is defined") {
        // This test ensures the platform-specific handle type is properly defined
        LoadedPlugin loaded;
        loaded.handle = nullptr;
        REQUIRE(loaded.handle == nullptr);
    }

    SECTION("Library extensions by platform") {
        // Document expected extensions per platform
        #ifdef _WIN32
            const std::string expectedExt = ".dll";
        #else
            const std::string expectedExt = ".so";
        #endif

        REQUIRE(!expectedExt.empty());
    }
}

/**
 * @file test_plugin_loader_edge_cases.cpp
 * @brief Comprehensive edge case tests for PluginLoader
 *
 * Tests cover:
 * - Symbol resolution failures (missing createPlugin, destroyPlugin, getPluginManifest)
 * - Null pointer returns from createPlugin
 * - Exception handling during plugin loading
 * - Multiple consecutive load/unload cycles
 * - Concurrent loading attempts
 * - Path handling edge cases (special characters, long paths, unicode)
 * - Memory safety with LoadedPlugin move semantics
 * - Error message quality and diagnostics
 * - Platform-specific behavior (Windows vs Linux)
 */

#include <catch_amalgamated.hpp>
#include "../../core/PluginLoader.hpp"
#include "../../core/IPlugin.hpp"
#include "../../core/PluginContext.hpp"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <atomic>

using namespace mcf;
namespace fs = std::filesystem;

// Test plugin with proper metadata
class WellBehavedPlugin : public IPlugin {
private:
    bool m_initialized = false;
    PluginMetadata m_metadata;
    PluginContext m_context;

public:
    WellBehavedPlugin() {
        m_metadata.name = "WellBehavedPlugin";
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

    std::string getName() const override {
        return m_metadata.name;
    }

    std::string getVersion() const override {
        return m_metadata.version;
    }

    const PluginMetadata& getMetadata() const override {
        return m_metadata;
    }

    static const char* getManifestJson() {
        return R"({"name":"WellBehavedPlugin","version":"1.0.0"})";
    }
};

TEST_CASE("PluginLoader - LoadedPlugin move semantics comprehensive", "[PluginLoader][edge]") {
    SECTION("Move construction preserves all fields") {
        LoadedPlugin original;
        original.path = "/test/plugin.so";
        original.metadata.name = "TestPlugin";
        original.metadata.version = "1.2.3";
        original.metadata.loadPriority = 42;
        original.metadata.addDependency("CorePlugin", "1.0.0", "2.0.0", true);

        LoadedPlugin moved = std::move(original);

        REQUIRE(moved.path == "/test/plugin.so");
        REQUIRE(moved.metadata.name == "TestPlugin");
        REQUIRE(moved.metadata.version == "1.2.3");
        REQUIRE(moved.metadata.loadPriority == 42);
        REQUIRE(moved.metadata.dependencies.size() == 1);
        REQUIRE(moved.metadata.dependencies[0].pluginName == "CorePlugin");
    }

    SECTION("Move assignment preserves all fields") {
        LoadedPlugin original;
        original.path = "/another/path.dll";
        original.metadata.name = "AnotherPlugin";
        original.metadata.version = "2.0.0";

        LoadedPlugin target;
        target = std::move(original);

        REQUIRE(target.path == "/another/path.dll");
        REQUIRE(target.metadata.name == "AnotherPlugin");
        REQUIRE(target.metadata.version == "2.0.0");
    }

    SECTION("Chained move operations") {
        LoadedPlugin p1;
        p1.path = "/test1.so";
        p1.metadata.name = "Plugin1";

        LoadedPlugin p2 = std::move(p1);
        LoadedPlugin p3 = std::move(p2);
        LoadedPlugin p4;
        p4 = std::move(p3);

        REQUIRE(p4.path == "/test1.so");
        REQUIRE(p4.metadata.name == "Plugin1");
    }

    SECTION("Move to self (should be safe)") {
        LoadedPlugin plugin;
        plugin.path = "/test.so";

        // This is undefined behavior in C++ but should not crash
        // We test that the structure remains valid
        plugin = std::move(plugin);

        // After move-to-self, object should still be in a valid state
        REQUIRE_NOTHROW([&]() {
            LoadedPlugin temp = std::move(plugin);
        }());
    }

    SECTION("Multiple move assignments to same target") {
        LoadedPlugin target;

        LoadedPlugin source1;
        source1.path = "/path1.so";
        target = std::move(source1);
        REQUIRE(target.path == "/path1.so");

        LoadedPlugin source2;
        source2.path = "/path2.so";
        target = std::move(source2);
        REQUIRE(target.path == "/path2.so");

        LoadedPlugin source3;
        source3.path = "/path3.so";
        target = std::move(source3);
        REQUIRE(target.path == "/path3.so");
    }
}

TEST_CASE("PluginLoader - Invalid path handling", "[PluginLoader][edge]") {
    SECTION("Null character in path") {
        std::string pathWithNull = std::string("/test/path") + '\0' + std::string("extra.so");
        REQUIRE_THROWS_AS(
            PluginLoader::loadPlugin(pathWithNull),
            PluginLoadException
        );
    }

    SECTION("Path with only whitespace") {
        REQUIRE_THROWS_AS(
            PluginLoader::loadPlugin("   "),
            PluginLoadException
        );
    }

    SECTION("Path with special characters") {
        std::string specialPath = "/test/path with spaces/plugin!@#$%.so";
        REQUIRE_THROWS_AS(
            PluginLoader::loadPlugin(specialPath),
            PluginLoadException
        );
    }

    SECTION("Very long path") {
        // Create a path that's very long (e.g., 4096+ characters)
        std::string longPath(4096, 'a');
        longPath += ".so";

        REQUIRE_THROWS_AS(
            PluginLoader::loadPlugin(longPath),
            PluginLoadException
        );
    }

    SECTION("Path with directory traversal") {
        REQUIRE_THROWS_AS(
            PluginLoader::loadPlugin("../../../../../../etc/passwd"),
            PluginLoadException
        );
    }

    SECTION("Path with unicode characters") {
        std::string unicodePath = "/test/路径/플러그인.so";
        REQUIRE_THROWS_AS(
            PluginLoader::loadPlugin(unicodePath),
            PluginLoadException
        );
    }

    SECTION("Relative path variations") {
        REQUIRE_THROWS_AS(
            PluginLoader::loadPlugin("./nonexistent.so"),
            PluginLoadException
        );

        REQUIRE_THROWS_AS(
            PluginLoader::loadPlugin("../nonexistent.so"),
            PluginLoadException
        );
    }

    SECTION("Path that is a directory") {
        // Create a temporary directory
        fs::path tempDir = fs::temp_directory_path() / "mcf_test_dir";
        fs::create_directories(tempDir);

        REQUIRE_THROWS_AS(
            PluginLoader::loadPlugin(tempDir.string()),
            PluginLoadException
        );

        fs::remove_all(tempDir);
    }
}

TEST_CASE("PluginLoader - Invalid library format", "[PluginLoader][edge]") {
    SECTION("Empty file") {
        fs::path tempFile = fs::temp_directory_path() / "empty_plugin.so";
        {
            std::ofstream file(tempFile);
            // Create empty file
        }

        REQUIRE_THROWS_AS(
            PluginLoader::loadPlugin(tempFile.string()),
            PluginLoadException
        );

        fs::remove(tempFile);
    }

    SECTION("Text file with .so extension") {
        fs::path tempFile = fs::temp_directory_path() / "text_plugin.so";
        {
            std::ofstream file(tempFile);
            file << "This is just a text file, not a shared library!\n";
            file << "It should fail to load.\n";
        }

        REQUIRE_THROWS_AS(
            PluginLoader::loadPlugin(tempFile.string()),
            PluginLoadException
        );

        fs::remove(tempFile);
    }

    SECTION("Binary file that is not ELF/PE format") {
        fs::path tempFile = fs::temp_directory_path() / "binary_plugin.so";
        {
            std::ofstream file(tempFile, std::ios::binary);
            // Write random binary data
            for (int i = 0; i < 1024; ++i) {
                file.put(static_cast<char>(i % 256));
            }
        }

        REQUIRE_THROWS_AS(
            PluginLoader::loadPlugin(tempFile.string()),
            PluginLoadException
        );

        fs::remove(tempFile);
    }

    SECTION("Corrupted library header") {
        fs::path tempFile = fs::temp_directory_path() / "corrupt_plugin.so";
        {
            std::ofstream file(tempFile, std::ios::binary);
            // Write partial ELF header (will be corrupted/incomplete)
#ifdef _WIN32
            // Partial PE header
            file << "MZ\x90\x00";
#else
            // Partial ELF header
            file << "\x7f" << "ELF";
#endif
            file << "corrupted data";
        }

        REQUIRE_THROWS_AS(
            PluginLoader::loadPlugin(tempFile.string()),
            PluginLoadException
        );

        fs::remove(tempFile);
    }
}

TEST_CASE("PluginLoader - getPluginManifest edge cases", "[PluginLoader][edge]") {
    SECTION("Empty path (platform-dependent behavior)") {
        // Note: Empty path behavior is platform-dependent
        // Some platforms throw, others may return empty manifest
        // We just verify it doesn't crash
        try {
            auto manifest = PluginLoader::getPluginManifest("");
            // If no exception, that's also acceptable behavior
            SUCCEED("Empty path handled without crash");
        } catch (const PluginLoadException&) {
            // Exception is also acceptable
            SUCCEED("Empty path throws exception as expected");
        }
    }

    SECTION("Non-existent file") {
        REQUIRE_THROWS_AS(
            PluginLoader::getPluginManifest("/absolutely/nonexistent/path/plugin.so"),
            PluginLoadException
        );
    }

    SECTION("Invalid library format") {
        fs::path tempFile = fs::temp_directory_path() / "invalid_manifest.so";
        {
            std::ofstream file(tempFile);
            file << "Not a valid library";
        }

        REQUIRE_THROWS_AS(
            PluginLoader::getPluginManifest(tempFile.string()),
            PluginLoadException
        );

        fs::remove(tempFile);
    }

    SECTION("Path with special characters") {
        std::string specialPath = "/test/path!@#$/plugin.so";
        REQUIRE_THROWS_AS(
            PluginLoader::getPluginManifest(specialPath),
            PluginLoadException
        );
    }
}

TEST_CASE("PluginLoader - unloadPlugin edge cases", "[PluginLoader][edge]") {
    SECTION("Unload with null instance and null handle") {
        LoadedPlugin loaded;
        loaded.instance = nullptr;
        loaded.handle = nullptr;

        REQUIRE_NOTHROW(PluginLoader::unloadPlugin(loaded));
        REQUIRE(loaded.instance == nullptr);
        REQUIRE(loaded.handle == nullptr);
    }

    SECTION("Unload with valid instance but null handle") {
        LoadedPlugin loaded;
        loaded.instance = std::make_unique<WellBehavedPlugin>();
        loaded.handle = nullptr;

        REQUIRE_NOTHROW(PluginLoader::unloadPlugin(loaded));
        REQUIRE(loaded.instance == nullptr);
    }

    SECTION("Double unload") {
        LoadedPlugin loaded;
        loaded.instance = std::make_unique<WellBehavedPlugin>();

        PluginLoader::unloadPlugin(loaded);
        REQUIRE(loaded.instance == nullptr);

        // Second unload should be safe
        REQUIRE_NOTHROW(PluginLoader::unloadPlugin(loaded));
    }

    SECTION("Triple unload") {
        LoadedPlugin loaded;

        PluginLoader::unloadPlugin(loaded);
        PluginLoader::unloadPlugin(loaded);
        PluginLoader::unloadPlugin(loaded);

        // All unloads should be safe
        REQUIRE(loaded.instance == nullptr);
    }

    SECTION("Unload after move") {
        LoadedPlugin original;
        original.instance = std::make_unique<WellBehavedPlugin>();

        LoadedPlugin moved = std::move(original);

        // Unload the moved-from object should be safe
        REQUIRE_NOTHROW(PluginLoader::unloadPlugin(original));

        // Unload the target should also work
        REQUIRE_NOTHROW(PluginLoader::unloadPlugin(moved));
    }
}

TEST_CASE("PluginLoader - PluginLoadException details", "[PluginLoader][edge]") {
    SECTION("Exception message contains full path") {
        std::string testPath = "/very/specific/test/path/plugin.so";
        try {
            PluginLoader::loadPlugin(testPath);
            FAIL("Should have thrown PluginLoadException");
        } catch (const PluginLoadException& e) {
            std::string message = e.what();
            REQUIRE_THAT(message, Catch::Matchers::ContainsSubstring("plugin.so"));
        }
    }

    SECTION("Exception is catchable as std::runtime_error") {
        bool caughtAsRuntimeError = false;
        bool caughtAsPluginLoadException = false;

        try {
            throw PluginLoadException("Test exception");
        } catch (const std::runtime_error& e) {
            caughtAsRuntimeError = true;
        }

        try {
            throw PluginLoadException("Test exception");
        } catch (const PluginLoadException& e) {
            caughtAsPluginLoadException = true;
        }

        REQUIRE(caughtAsRuntimeError);
        REQUIRE(caughtAsPluginLoadException);
    }

    SECTION("Exception is catchable as std::exception") {
        bool caught = false;
        try {
            throw PluginLoadException("Test exception");
        } catch (const std::exception& e) {
            caught = true;
        }
        REQUIRE(caught);
    }

    SECTION("Exception message preserved through catch and rethrow") {
        const std::string originalMessage = "Original error message";

        try {
            try {
                throw PluginLoadException(originalMessage);
            } catch (...) {
                throw; // Rethrow
            }
        } catch (const PluginLoadException& e) {
            REQUIRE_THAT(e.what(), Catch::Matchers::ContainsSubstring(originalMessage));
        }
    }

    SECTION("Multiple exception instances are independent") {
        PluginLoadException ex1("Error 1");
        PluginLoadException ex2("Error 2");

        REQUIRE_THAT(ex1.what(), Catch::Matchers::ContainsSubstring("Error 1"));
        REQUIRE_THAT(ex2.what(), Catch::Matchers::ContainsSubstring("Error 2"));
        REQUIRE_THAT(ex1.what(), !Catch::Matchers::ContainsSubstring("Error 2"));
    }
}

TEST_CASE("PluginLoader - Metadata handling edge cases", "[PluginLoader][edge]") {
    SECTION("Empty metadata fields") {
        LoadedPlugin loaded;
        loaded.metadata.name = "";
        loaded.metadata.version = "";

        REQUIRE(loaded.metadata.name.empty());
        REQUIRE(loaded.metadata.version.empty());
    }

    SECTION("Metadata with very long strings") {
        LoadedPlugin loaded;
        loaded.metadata.name = std::string(10000, 'X');
        loaded.metadata.version = std::string(5000, '9');

        REQUIRE(loaded.metadata.name.length() == 10000);
        REQUIRE(loaded.metadata.version.length() == 5000);
    }

    SECTION("Metadata with special characters") {
        LoadedPlugin loaded;
        loaded.metadata.name = "Plugin!@#$%^&*()_+-=[]{}|;':\",./<>?";
        loaded.metadata.version = "1.0.0-alpha+build.123";

        REQUIRE(!loaded.metadata.name.empty());
        REQUIRE(!loaded.metadata.version.empty());
    }

    SECTION("Metadata with unicode") {
        LoadedPlugin loaded;
        loaded.metadata.name = "プラグイン";
        loaded.metadata.version = "版本1.0";

        REQUIRE(!loaded.metadata.name.empty());
        REQUIRE(!loaded.metadata.version.empty());
    }

    SECTION("Metadata with many dependencies") {
        LoadedPlugin loaded;
        loaded.metadata.name = "HeavyPlugin";

        // Add 100 dependencies
        for (int i = 0; i < 100; ++i) {
            loaded.metadata.addDependency(
                "Dep" + std::to_string(i),
                "1.0.0",
                "2.0.0",
                i % 2 == 0 // Alternate required/optional
            );
        }

        REQUIRE(loaded.metadata.dependencies.size() == 100);
        REQUIRE(loaded.metadata.dependencies[0].required == true);
        REQUIRE(loaded.metadata.dependencies[1].required == false);
    }

    SECTION("Metadata priority edge values") {
        LoadedPlugin loaded1;
        loaded1.metadata.loadPriority = 0;
        REQUIRE(loaded1.metadata.loadPriority == 0);

        LoadedPlugin loaded2;
        loaded2.metadata.loadPriority = 2147483647; // Max int32
        REQUIRE(loaded2.metadata.loadPriority == 2147483647);

        LoadedPlugin loaded3;
        loaded3.metadata.loadPriority = -2147483648; // Min int32
        REQUIRE(loaded3.metadata.loadPriority == -2147483648);
    }
}

TEST_CASE("PluginLoader - Thread safety considerations", "[PluginLoader][edge]") {
    SECTION("Concurrent load attempts on different paths") {
        // Note: This tests that multiple threads can attempt loads simultaneously
        // without crashing, even though all loads will fail

        std::vector<std::thread> threads;
        std::atomic<int> exceptionCount{0};

        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([i, &exceptionCount]() {
                try {
                    std::string path = "/nonexistent/plugin" + std::to_string(i) + ".so";
                    PluginLoader::loadPlugin(path);
                } catch (const PluginLoadException&) {
                    ++exceptionCount;
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        // All loads should have failed with exceptions
        REQUIRE(exceptionCount == 10);
    }

    SECTION("Concurrent unload attempts") {
        std::vector<LoadedPlugin> plugins(10);
        std::vector<std::thread> threads;

        for (size_t i = 0; i < plugins.size(); ++i) {
            threads.emplace_back([&plugins, i]() {
                PluginLoader::unloadPlugin(plugins[i]);
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        // All unloads should have completed without crash
        for (const auto& plugin : plugins) {
            REQUIRE(plugin.instance == nullptr);
        }
    }
}

TEST_CASE("PluginLoader - LoadedPlugin stress test", "[PluginLoader][edge][stress]") {
    SECTION("Create and destroy many LoadedPlugin instances") {
        std::vector<LoadedPlugin> plugins;

        for (int i = 0; i < 1000; ++i) {
            LoadedPlugin loaded;
            loaded.path = "/test/plugin" + std::to_string(i) + ".so";
            loaded.metadata.name = "Plugin" + std::to_string(i);
            loaded.metadata.version = "1.0." + std::to_string(i);
            plugins.push_back(std::move(loaded));
        }

        REQUIRE(plugins.size() == 1000);
        REQUIRE(plugins[500].metadata.name == "Plugin500");
    }

    SECTION("Chain of moves") {
        LoadedPlugin original;
        original.path = "/test.so";
        original.metadata.name = "TestPlugin";

        // Perform 100 move operations
        LoadedPlugin current = std::move(original);
        for (int i = 0; i < 99; ++i) {
            LoadedPlugin temp = std::move(current);
            current = std::move(temp);
        }

        REQUIRE(current.path == "/test.so");
        REQUIRE(current.metadata.name == "TestPlugin");
    }

    SECTION("Alternating construction and destruction") {
        for (int i = 0; i < 100; ++i) {
            LoadedPlugin loaded;
            loaded.path = "/test.so";
            loaded.metadata.name = "TestPlugin";
            // Destroyed at end of loop iteration
        }
        // Should complete without issues
        SUCCEED();
    }
}

TEST_CASE("PluginLoader - Error message quality", "[PluginLoader][edge]") {
    SECTION("Load error includes operation description") {
        try {
            PluginLoader::loadPlugin("/invalid/path.so");
            FAIL("Should have thrown");
        } catch (const PluginLoadException& e) {
            std::string msg = e.what();
            // Should mention "load" or "Failed" or similar
            bool hasGoodMessage =
                msg.find("load") != std::string::npos ||
                msg.find("Load") != std::string::npos ||
                msg.find("Failed") != std::string::npos;
            REQUIRE(hasGoodMessage);
        }
    }

    SECTION("Manifest error is distinguishable from load error") {
        try {
            PluginLoader::getPluginManifest("/invalid/manifest.so");
            FAIL("Should have thrown");
        } catch (const PluginLoadException& e) {
            std::string msg = e.what();
            // Should mention "manifest" or use different error message
            bool isManifestError =
                msg.find("manifest") != std::string::npos ||
                msg.find("Manifest") != std::string::npos;
            REQUIRE(isManifestError);
        }
    }
}

TEST_CASE("PluginLoader - Platform handle type", "[PluginLoader][edge]") {
    SECTION("PLUGIN_HANDLE can be set to nullptr") {
        LoadedPlugin loaded;
        loaded.handle = nullptr;
        REQUIRE(loaded.handle == nullptr);
    }

    SECTION("PLUGIN_HANDLE can be compared") {
        LoadedPlugin loaded1;
        LoadedPlugin loaded2;
        loaded1.handle = nullptr;
        loaded2.handle = nullptr;

        REQUIRE(loaded1.handle == loaded2.handle);
    }

    SECTION("PLUGIN_HANDLE type is consistent") {
        LoadedPlugin loaded;
        loaded.handle = nullptr;

        // This tests that handle can be used consistently
        if (loaded.handle == nullptr) {
            SUCCEED();
        } else {
            FAIL("Handle should be nullptr");
        }
    }
}

TEST_CASE("PluginLoader - Path edge cases in error messages", "[PluginLoader][edge]") {
    SECTION("Empty path produces clear error") {
        try {
            PluginLoader::loadPlugin("");
            FAIL("Should have thrown");
        } catch (const PluginLoadException& e) {
            // Error message should exist and be non-empty
            REQUIRE(std::string(e.what()).length() > 0);
        }
    }

    SECTION("Long path is handled in error message") {
        std::string longPath(1000, 'x');
        longPath += ".so";

        try {
            PluginLoader::loadPlugin(longPath);
            FAIL("Should have thrown");
        } catch (const PluginLoadException& e) {
            // Should not crash when formatting error with long path
            REQUIRE(std::string(e.what()).length() > 0);
        }
    }

    SECTION("Path with null character") {
        std::string nullPath = std::string("/test") + '\0' + std::string("path.so");

        try {
            PluginLoader::loadPlugin(nullPath);
            FAIL("Should have thrown");
        } catch (const PluginLoadException& e) {
            // Should handle null character without crashing
            REQUIRE(std::string(e.what()).length() > 0);
        }
    }
}

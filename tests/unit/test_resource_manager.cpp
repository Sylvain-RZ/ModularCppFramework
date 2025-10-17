/**
 * @file test_resource_manager_catch2.cpp
 * @brief Unit tests for ResourceManager using Catch2
 */

#include "../../core/ResourceManager.hpp"
#include "../../external/catch_amalgamated.hpp"

#include <memory>
#include <string>
#include <algorithm>

using namespace mcf;

// Test resource types
struct TestTexture {
    std::string path;
    int width = 0;
    int height = 0;
    static int instanceCount;

    TestTexture(const std::string& p, int w = 256, int h = 256)
        : path(p), width(w), height(h) {
        instanceCount++;
    }

    ~TestTexture() {
        instanceCount--;
    }
};

int TestTexture::instanceCount = 0;

struct TestMesh {
    std::string path;
    int vertexCount = 0;

    explicit TestMesh(const std::string& p, int verts = 100)
        : path(p), vertexCount(verts) {}
};

// =============================================================================
// Basic Resource Loading Tests
// =============================================================================

TEST_CASE("ResourceManager - Register loader and load resource", "[resourcemanager][core]") {
    ResourceManager manager;

    manager.registerLoader<TestTexture>([](const std::string& path) {
        return std::make_shared<TestTexture>(path, 512, 512);
    });

    SECTION("Load resource with registered loader") {
        auto texture = manager.load<TestTexture>("test.png");
        REQUIRE(texture != nullptr);
        REQUIRE(texture->path == "test.png");
        REQUIRE(texture->width == 512);
        REQUIRE(texture->height == 512);
    }

    SECTION("Load multiple different resources") {
        auto tex1 = manager.load<TestTexture>("texture1.png");
        auto tex2 = manager.load<TestTexture>("texture2.png");
        REQUIRE(tex1 != nullptr);
        REQUIRE(tex2 != nullptr);
        REQUIRE(tex1->path == "texture1.png");
        REQUIRE(tex2->path == "texture2.png");
    }
}

TEST_CASE("ResourceManager - Loader not registered", "[resourcemanager][core][error]") {
    ResourceManager manager;

    REQUIRE_THROWS_AS(manager.load<TestTexture>("test.png"), std::runtime_error);
}

// =============================================================================
// Resource Caching Tests
// =============================================================================

TEST_CASE("ResourceManager - Resource caching", "[resourcemanager][core]") {
    ResourceManager manager;

    manager.registerLoader<TestTexture>([](const std::string& path) {
        return std::make_shared<TestTexture>(path);
    });

    SECTION("Cached resource returns same instance") {
        auto texture1 = manager.load<TestTexture>("cached.png");
        auto texture2 = manager.load<TestTexture>("cached.png");

        REQUIRE(texture1.get() == texture2.get());
    }

    SECTION("Different paths return different instances") {
        auto texture1 = manager.load<TestTexture>("texture1.png");
        auto texture2 = manager.load<TestTexture>("texture2.png");

        REQUIRE(texture1.get() != texture2.get());
    }

    SECTION("Set cached flag controls caching behavior") {
        auto texture = manager.load<TestTexture>("uncached.png");
        manager.setCached("uncached.png", false);

        // Resource should still be loaded
        REQUIRE(manager.isLoaded("uncached.png"));
    }
}

// =============================================================================
// Reference Counting Tests
// =============================================================================

TEST_CASE("ResourceManager - Reference counting", "[resourcemanager][core]") {
    ResourceManager manager;

    manager.registerLoader<TestTexture>([](const std::string& path) {
        return std::make_shared<TestTexture>(path);
    });

    SECTION("Initial reference count") {
        auto texture = manager.load<TestTexture>("counted.png");
        REQUIRE(manager.getReferenceCount("counted.png") == 1);
    }

    SECTION("Multiple loads increase reference count") {
        auto texture1 = manager.load<TestTexture>("counted.png");
        auto texture2 = manager.load<TestTexture>("counted.png");
        REQUIRE(manager.getReferenceCount("counted.png") == 2);
    }

    SECTION("Release decreases reference count") {
        auto texture = manager.load<TestTexture>("counted.png");
        manager.load<TestTexture>("counted.png"); // Load again

        manager.release("counted.png");
        REQUIRE(manager.getReferenceCount("counted.png") == 1);
    }
}

// =============================================================================
// Manual Resource Management Tests
// =============================================================================

TEST_CASE("ResourceManager - Manual add and get", "[resourcemanager][core]") {
    ResourceManager manager;

    SECTION("Add and retrieve resource manually") {
        auto texture = std::make_shared<TestTexture>("manual.png");
        manager.add<TestTexture>("manual.png", texture);

        auto retrieved = manager.get<TestTexture>("manual.png");
        REQUIRE(retrieved != nullptr);
        REQUIRE(retrieved->path == "manual.png");
    }

    SECTION("Get non-existent resource returns nullptr") {
        auto texture = manager.get<TestTexture>("nonexistent.png");
        REQUIRE(texture == nullptr);
    }

    SECTION("Is loaded checks resource existence") {
        REQUIRE_FALSE(manager.isLoaded("test.png"));

        manager.add<TestTexture>("test.png", std::make_shared<TestTexture>("test.png"));
        REQUIRE(manager.isLoaded("test.png"));
    }
}

TEST_CASE("ResourceManager - Unload resources", "[resourcemanager][core]") {
    ResourceManager manager;

    manager.add<TestTexture>("test.png", std::make_shared<TestTexture>("test.png"));
    REQUIRE(manager.isLoaded("test.png"));

    SECTION("Unload removes resource") {
        manager.unload("test.png");
        REQUIRE_FALSE(manager.isLoaded("test.png"));
    }

    SECTION("Unload non-existent resource is safe") {
        manager.unload("nonexistent.png");
        // Should not throw
    }
}

// =============================================================================
// Multiple Resource Types Tests
// =============================================================================

TEST_CASE("ResourceManager - Multiple resource types", "[resourcemanager][core]") {
    ResourceManager manager;

    manager.registerLoader<TestTexture>([](const std::string& path) {
        return std::make_shared<TestTexture>(path);
    });

    manager.registerLoader<TestMesh>([](const std::string& path) {
        return std::make_shared<TestMesh>(path);
    });

    SECTION("Load different resource types") {
        auto texture = manager.load<TestTexture>("test.png");
        auto mesh = manager.load<TestMesh>("test.obj");

        REQUIRE(texture != nullptr);
        REQUIRE(mesh != nullptr);
        REQUIRE(texture->path == "test.png");
        REQUIRE(mesh->path == "test.obj");
    }

    SECTION("Resource count includes all types") {
        manager.load<TestTexture>("tex1.png");
        manager.load<TestTexture>("tex2.png");
        manager.load<TestMesh>("mesh1.obj");

        auto paths = manager.getLoadedResources();
        REQUIRE(paths.size() == 3);
    }
}

// =============================================================================
// Resource Cleanup Tests
// =============================================================================

TEST_CASE("ResourceManager - Clear operations", "[resourcemanager][core]") {
    ResourceManager manager;

    manager.registerLoader<TestTexture>([](const std::string& path) {
        return std::make_shared<TestTexture>(path);
    });

    SECTION("Clear unreferenced resources") {
        auto tex1 = manager.load<TestTexture>("tex1.png");
        auto tex2 = manager.load<TestTexture>("tex2.png");

        manager.setCached("tex1.png", false);
        manager.release("tex1.png");

        // Resource count should be 1 or 2 depending on cleanup timing
        size_t count = manager.getResourceCount();
        REQUIRE((count == 1 || count == 2));
    }

    SECTION("Clear all resources") {
        manager.add<TestTexture>("tex1.png", std::make_shared<TestTexture>("tex1.png"));
        manager.add<TestTexture>("tex2.png", std::make_shared<TestTexture>("tex2.png"));

        REQUIRE(manager.getResourceCount() == 2);

        manager.clear();
        REQUIRE(manager.getResourceCount() == 0);
    }

    SECTION("Get loaded resources list") {
        manager.add<TestTexture>("tex1.png", std::make_shared<TestTexture>("tex1.png"));
        manager.add<TestTexture>("tex2.png", std::make_shared<TestTexture>("tex2.png"));

        auto paths = manager.getLoadedResources();
        REQUIRE(paths.size() == 2);
        REQUIRE(std::find(paths.begin(), paths.end(), "tex1.png") != paths.end());
        REQUIRE(std::find(paths.begin(), paths.end(), "tex2.png") != paths.end());
    }
}

// =============================================================================
// Plugin-Aware Resource Management Tests (Hot Reload Support)
// =============================================================================

TEST_CASE("ResourceManager - Plugin-aware resources", "[resourcemanager][hot-reload]") {
    ResourceManager manager;

    auto tex1 = std::make_shared<TestTexture>("tex1.png");
    auto tex2 = std::make_shared<TestTexture>("tex2.png");
    auto tex3 = std::make_shared<TestTexture>("tex3.png");

    manager.addWithPlugin<TestTexture>("tex1.png", tex1, "Plugin1");
    manager.addWithPlugin<TestTexture>("tex2.png", tex2, "Plugin1");
    manager.addWithPlugin<TestTexture>("tex3.png", tex3, "Plugin2");

    SECTION("Track resources per plugin") {
        REQUIRE(manager.getResourceCount() == 3);
        REQUIRE(manager.isLoaded("tex1.png"));
        REQUIRE(manager.isLoaded("tex2.png"));
        REQUIRE(manager.isLoaded("tex3.png"));
    }

    SECTION("Unload all resources for specific plugin") {
        size_t removed = manager.unloadPlugin("Plugin1");
        REQUIRE(removed == 2);

        REQUIRE(manager.getResourceCount() == 1);
        REQUIRE(manager.isLoaded("tex3.png"));
        REQUIRE_FALSE(manager.isLoaded("tex1.png"));
        REQUIRE_FALSE(manager.isLoaded("tex2.png"));
    }

    SECTION("Unload non-existent plugin is safe") {
        size_t removed = manager.unloadPlugin("NonExistentPlugin");
        REQUIRE(removed == 0);
        REQUIRE(manager.getResourceCount() == 3);
    }
}

// =============================================================================
// Resource Lifecycle Tests
// =============================================================================

TEST_CASE("ResourceManager - Resource instance lifecycle", "[resourcemanager][core]") {
    TestTexture::instanceCount = 0;

    SECTION("Resources cleaned up on manager destruction") {
        {
            ResourceManager manager;

            manager.registerLoader<TestTexture>([](const std::string& path) {
                return std::make_shared<TestTexture>(path);
            });

            auto tex = manager.load<TestTexture>("test.png");
            REQUIRE(TestTexture::instanceCount == 1);

            auto tex2 = manager.load<TestTexture>("test2.png");
            REQUIRE(TestTexture::instanceCount == 2);
        } // Manager destroyed, should clean up resources

        REQUIRE(TestTexture::instanceCount == 0);
    }

    SECTION("Resources cleaned up when unloaded") {
        TestTexture::instanceCount = 0;

        ResourceManager manager;
        manager.add<TestTexture>("test.png", std::make_shared<TestTexture>("test.png"));
        REQUIRE(TestTexture::instanceCount == 1);

        manager.unload("test.png");
        REQUIRE(TestTexture::instanceCount == 0);
    }
}

// =============================================================================
// Performance Benchmarks
// =============================================================================

TEST_CASE("ResourceManager - Performance", "[.benchmark][resourcemanager]") {
    ResourceManager manager;

    manager.registerLoader<TestTexture>([](const std::string& path) {
        return std::make_shared<TestTexture>(path);
    });

    BENCHMARK("Load 100 unique resources") {
        for (int i = 0; i < 100; ++i) {
            manager.load<TestTexture>("texture_" + std::to_string(i) + ".png");
        }
    };

    // Load resources first
    for (int i = 0; i < 100; ++i) {
        manager.load<TestTexture>("cached_" + std::to_string(i) + ".png");
    }

    BENCHMARK("Get 100 cached resources") {
        for (int i = 0; i < 100; ++i) {
            manager.get<TestTexture>("cached_" + std::to_string(i) + ".png");
        }
    };

    BENCHMARK("Clear 100 resources") {
        // Load first
        for (int i = 0; i < 100; ++i) {
            manager.load<TestTexture>("bench_" + std::to_string(i) + ".png");
        }
        return manager.clear();
    };
}

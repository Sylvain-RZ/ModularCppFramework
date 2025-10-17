/**
 * @file test_service_locator_catch2.cpp
 * @brief Unit tests for ServiceLocator using Catch2 v3
 */

#define CATCH_CONFIG_MAIN
#include <catch_amalgamated.hpp>

#include "../../core/ServiceLocator.hpp"
#include <memory>

using namespace mcf;

// Test interfaces and implementations
class ITestService {
public:
    virtual ~ITestService() = default;
    virtual int getValue() const = 0;
};

class TestServiceImpl : public ITestService {
private:
    int m_value;
public:
    TestServiceImpl(int value = 42) : m_value(value) {}
    int getValue() const override { return m_value; }
};

TEST_CASE("ServiceLocator - Singleton registration and resolution", "[servicelocator][core]") {
    ServiceLocator locator;

    auto service = std::make_shared<TestServiceImpl>(100);
    locator.registerSingleton<ITestService>(service);

    auto resolved = locator.resolve<ITestService>();
    REQUIRE(resolved != nullptr);
    REQUIRE(resolved->getValue() == 100);
}

TEST_CASE("ServiceLocator - Singleton returns same instance", "[servicelocator][core]") {
    ServiceLocator locator;

    auto service = std::make_shared<TestServiceImpl>(200);
    locator.registerSingleton<ITestService>(service);

    auto resolved1 = locator.resolve<ITestService>();
    auto resolved2 = locator.resolve<ITestService>();

    REQUIRE(resolved1.get() == resolved2.get());
}

TEST_CASE("ServiceLocator - Factory lifetime", "[servicelocator][core]") {
    ServiceLocator locator;
    int counter = 0;

    SECTION("Transient creates new instances") {
        locator.registerFactory<ITestService>(
            [&counter]() { return std::make_shared<TestServiceImpl>(++counter); },
            ServiceLifetime::Transient
        );

        auto resolved1 = locator.resolve<ITestService>();
        auto resolved2 = locator.resolve<ITestService>();

        REQUIRE(resolved1->getValue() == 1);
        REQUIRE(resolved2->getValue() == 2);
        REQUIRE(resolved1.get() != resolved2.get());
    }

    SECTION("Singleton factory creates once") {
        locator.registerFactory<ITestService>(
            [&counter]() { return std::make_shared<TestServiceImpl>(++counter); },
            ServiceLifetime::Singleton
        );

        auto resolved1 = locator.resolve<ITestService>();
        auto resolved2 = locator.resolve<ITestService>();

        REQUIRE(resolved1->getValue() == 1);
        REQUIRE(resolved2->getValue() == 1);
        REQUIRE(resolved1.get() == resolved2.get());
    }
}

TEST_CASE("ServiceLocator - Type registration", "[servicelocator][core]") {
    ServiceLocator locator;

    locator.registerType<ITestService, TestServiceImpl>(ServiceLifetime::Singleton);

    auto resolved = locator.resolve<ITestService>();
    REQUIRE(resolved != nullptr);
    REQUIRE(resolved->getValue() == 42); // Default constructor
}

TEST_CASE("ServiceLocator - Named services", "[servicelocator][core]") {
    ServiceLocator locator;

    auto service1 = std::make_shared<TestServiceImpl>(111);
    auto service2 = std::make_shared<TestServiceImpl>(222);

    locator.registerNamed<ITestService>("service1", service1);
    locator.registerNamed<ITestService>("service2", service2);

    auto resolved1 = locator.resolveNamed<ITestService>("service1");
    auto resolved2 = locator.resolveNamed<ITestService>("service2");

    REQUIRE(resolved1->getValue() == 111);
    REQUIRE(resolved2->getValue() == 222);
}

TEST_CASE("ServiceLocator - Registration checks", "[servicelocator][core]") {
    ServiceLocator locator;

    SECTION("isRegistered") {
        REQUIRE_FALSE(locator.isRegistered<ITestService>());

        auto service = std::make_shared<TestServiceImpl>();
        locator.registerSingleton<ITestService>(service);

        REQUIRE(locator.isRegistered<ITestService>());
    }

    SECTION("isRegisteredNamed") {
        REQUIRE_FALSE(locator.isRegisteredNamed("test"));

        auto service = std::make_shared<TestServiceImpl>();
        locator.registerNamed<ITestService>("test", service);

        REQUIRE(locator.isRegisteredNamed("test"));
    }
}

TEST_CASE("ServiceLocator - Try resolve (no exceptions)", "[servicelocator][core]") {
    ServiceLocator locator;

    SECTION("Returns nullptr for unregistered service") {
        auto resolved = locator.tryResolve<ITestService>();
        REQUIRE(resolved == nullptr);
    }

    SECTION("Returns service after registration") {
        auto service = std::make_shared<TestServiceImpl>();
        locator.registerSingleton<ITestService>(service);

        auto resolved = locator.tryResolve<ITestService>();
        REQUIRE(resolved != nullptr);
    }
}

TEST_CASE("ServiceLocator - Unregister", "[servicelocator][core]") {
    ServiceLocator locator;

    auto service = std::make_shared<TestServiceImpl>();
    locator.registerSingleton<ITestService>(service);
    REQUIRE(locator.isRegistered<ITestService>());

    locator.unregister<ITestService>();
    REQUIRE_FALSE(locator.isRegistered<ITestService>());
}

TEST_CASE("ServiceLocator - Plugin-aware cleanup", "[servicelocator][core][hot-reload]") {
    ServiceLocator locator;

    auto service1 = std::make_shared<TestServiceImpl>(111);
    locator.registerSingletonWithPlugin<ITestService>(service1, "Plugin1");
    REQUIRE(locator.isRegistered<ITestService>());

    size_t removed = locator.unregisterPlugin("Plugin1");
    REQUIRE(removed == 1);
    REQUIRE_FALSE(locator.isRegistered<ITestService>());
}

TEST_CASE("ServiceLocator - Clear all", "[servicelocator][core]") {
    ServiceLocator locator;

    locator.registerSingleton<ITestService>(std::make_shared<TestServiceImpl>());
    locator.registerNamed<ITestService>("test", std::make_shared<TestServiceImpl>());

    REQUIRE(locator.serviceCount() == 2);

    locator.clear();

    REQUIRE(locator.serviceCount() == 0);
    REQUIRE_FALSE(locator.isRegistered<ITestService>());
    REQUIRE_FALSE(locator.isRegisteredNamed("test"));
}

TEST_CASE("ServiceLocator - Exceptions", "[servicelocator][core]") {
    ServiceLocator locator;

    SECTION("Resolve throws on missing service") {
        REQUIRE_THROWS_AS(locator.resolve<ITestService>(), std::runtime_error);
    }

    SECTION("ResolveNamed throws on missing service") {
        REQUIRE_THROWS_AS(locator.resolveNamed<ITestService>("nonexistent"), std::runtime_error);
    }
}

TEST_CASE("ServiceLocator - Performance", "[.benchmark][servicelocator]") {
    BENCHMARK("Register 100 singletons") {
        ServiceLocator locator;
        for (int i = 0; i < 100; ++i) {
            locator.registerNamed<ITestService>(
                "service" + std::to_string(i),
                std::make_shared<TestServiceImpl>(i)
            );
        }
        return locator.serviceCount();
    };

    BENCHMARK("Resolve singleton 1000 times") {
        ServiceLocator locator;
        locator.registerSingleton<ITestService>(std::make_shared<TestServiceImpl>());
        return [&]() {
            for (int i = 0; i < 1000; ++i) {
                auto s = locator.resolve<ITestService>();
            }
        };
    };
}

// Test service with unique ID for scoped lifetime testing
class CountedService : public ITestService {
private:
    static int s_instanceCount;
    static int s_nextId;
    int m_id;

public:
    CountedService() : m_id(++s_nextId) {
        ++s_instanceCount;
    }

    ~CountedService() override {
        --s_instanceCount;
    }

    int getValue() const override { return m_id; }

    static int getInstanceCount() { return s_instanceCount; }
    static void resetCount() { s_instanceCount = 0; s_nextId = 0; }
};

int CountedService::s_instanceCount = 0;
int CountedService::s_nextId = 0;

TEST_CASE("ServiceLocator - Scoped lifetime basics", "[servicelocator][scoped][core]") {
    ServiceLocator locator;
    CountedService::resetCount();

    locator.registerFactory<ITestService>(
        []() { return std::make_shared<CountedService>(); },
        ServiceLifetime::Scoped
    );

    SECTION("Cannot resolve scoped service outside of scope") {
        REQUIRE_THROWS_WITH(
            locator.resolve<ITestService>(),
            Catch::Matchers::ContainsSubstring("outside of a scope")
        );
    }

    SECTION("Same instance within a scope") {
        ServiceScope scope(locator);
        auto service1 = locator.resolve<ITestService>();
        auto service2 = locator.resolve<ITestService>();

        REQUIRE(service1.get() == service2.get());
        REQUIRE(service1->getValue() == service2->getValue());
        REQUIRE(CountedService::getInstanceCount() == 1);
    }

    SECTION("Different instance in different scopes") {
        int id1, id2;

        {
            ServiceScope scope1(locator);
            auto service = locator.resolve<ITestService>();
            id1 = service->getValue();
        }

        {
            ServiceScope scope2(locator);
            auto service = locator.resolve<ITestService>();
            id2 = service->getValue();
        }

        REQUIRE(id1 != id2);
    }

    SECTION("Instance is destroyed when scope exits") {
        {
            ServiceScope scope(locator);
            auto service = locator.resolve<ITestService>();
            REQUIRE(CountedService::getInstanceCount() == 1);
        }
        REQUIRE(CountedService::getInstanceCount() == 0);
    }
}

TEST_CASE("ServiceLocator - Nested scopes", "[servicelocator][scoped][core]") {
    ServiceLocator locator;
    CountedService::resetCount();

    locator.registerFactory<ITestService>(
        []() { return std::make_shared<CountedService>(); },
        ServiceLifetime::Scoped
    );

    SECTION("Different instances in nested scopes") {
        ServiceScope outerScope(locator);
        auto outerService = locator.resolve<ITestService>();
        int outerId = outerService->getValue();

        {
            ServiceScope innerScope(locator);
            auto innerService = locator.resolve<ITestService>();
            int innerId = innerService->getValue();

            REQUIRE(outerId != innerId);
            REQUIRE(CountedService::getInstanceCount() == 2);
        }

        // Inner scope exited, outer scope still valid
        REQUIRE(CountedService::getInstanceCount() == 1);
        REQUIRE(outerService->getValue() == outerId);
    }

    REQUIRE(CountedService::getInstanceCount() == 0);
}

TEST_CASE("ServiceLocator - Scope tracking", "[servicelocator][scoped][core]") {
    ServiceLocator locator;

    SECTION("Scope depth tracking") {
        REQUIRE_FALSE(locator.isInScope());
        REQUIRE(locator.scopeDepth() == 0);

        {
            ServiceScope scope1(locator);
            REQUIRE(locator.isInScope());
            REQUIRE(locator.scopeDepth() == 1);

            {
                ServiceScope scope2(locator);
                REQUIRE(locator.scopeDepth() == 2);

                {
                    ServiceScope scope3(locator);
                    REQUIRE(locator.scopeDepth() == 3);
                }
                REQUIRE(locator.scopeDepth() == 2);
            }
            REQUIRE(locator.scopeDepth() == 1);
        }

        REQUIRE_FALSE(locator.isInScope());
        REQUIRE(locator.scopeDepth() == 0);
    }

    SECTION("Exit scope throws when no scope active") {
        REQUIRE_THROWS_WITH(
            locator.exitScope(),
            Catch::Matchers::ContainsSubstring("no scope is active")
        );
    }
}

TEST_CASE("ServiceLocator - Scoped vs Singleton vs Transient", "[servicelocator][scoped][core]") {
    ServiceLocator locator;
    CountedService::resetCount();

    // Register three services with different lifetimes
    locator.registerFactory<ITestService>(
        []() { return std::make_shared<CountedService>(); },
        ServiceLifetime::Singleton
    );

    SECTION("Singleton returns same instance always") {
        auto s1 = locator.resolve<ITestService>();
        auto s2 = locator.resolve<ITestService>();
        REQUIRE(s1.get() == s2.get());
    }
}

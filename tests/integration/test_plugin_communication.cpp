/**
 * @file test_plugin_communication.cpp
 * @brief Integration test for multi-plugin communication and interaction
 *
 * Tests complex scenarios where multiple plugins interact via EventBus,
 * ServiceLocator, and shared resources.
 */

#include "../../core/Application.hpp"
#include "../../core/IPlugin.hpp"
#include "../../core/EventBus.hpp"
#include "../../core/ServiceLocator.hpp"
#include "../../core/ResourceManager.hpp"
#include "../../core/DependencyResolver.hpp"
#include "../../external/catch_amalgamated.hpp"

#include <atomic>
#include <memory>
#include <vector>

using namespace mcf;

// ===========================================================================
// Shared Service Interfaces
// ===========================================================================

class IDataService {
public:
    virtual ~IDataService() = default;
    virtual void setData(const std::string& key, int value) = 0;
    virtual int getData(const std::string& key) const = 0;
};

class DataService : public IDataService {
private:
    std::map<std::string, int> m_data;

public:
    void setData(const std::string& key, int value) override {
        m_data[key] = value;
    }

    int getData(const std::string& key) const override {
        auto it = m_data.find(key);
        return it != m_data.end() ? it->second : 0;
    }
};

// ===========================================================================
// Test Plugins
// ===========================================================================

// Producer Plugin - generates data and publishes events
class ProducerPlugin : public IPlugin {
private:
    bool m_initialized = false;
    PluginMetadata m_metadata;
    PluginContext m_context;
    int m_producedCount = 0;

public:
    ProducerPlugin() {
        m_metadata.name = "ProducerPlugin";
        m_metadata.version = "1.0.0";
        m_metadata.loadPriority = 100;
    }

    bool initialize(PluginContext& context) override {
        m_context = context;
        m_initialized = true;

        // Register a service
        if (auto* locator = context.getServiceLocator()) {
            locator->registerSingletonWithPlugin<IDataService>(
                std::make_shared<DataService>(),
                m_metadata.name
            );
        }

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

    void produceData(int value) {
        m_producedCount++;

        // Store in service
        if (auto* locator = m_context.getServiceLocator()) {
            if (auto service = locator->tryResolve<IDataService>()) {
                service->setData("latest", value);
            }
        }

        // Publish event
        if (auto* eventBus = m_context.getEventBus()) {
            Event event("data.produced", value);
            eventBus->publish("data.produced", event);
        }
    }

    int getProducedCount() const { return m_producedCount; }

    static const char* getManifestJson() {
        return R"({"name":"ProducerPlugin","version":"1.0.0"})";
    }
};

// Consumer Plugin - consumes data from events
class ConsumerPlugin : public IPlugin {
private:
    bool m_initialized = false;
    PluginMetadata m_metadata;
    PluginContext m_context;
    EventHandle m_eventHandle = 0;
    std::atomic<int> m_consumedCount{0};
    std::atomic<int> m_lastValue{0};

public:
    ConsumerPlugin() {
        m_metadata.name = "ConsumerPlugin";
        m_metadata.version = "1.0.0";
        m_metadata.loadPriority = 50;
        m_metadata.addDependency("ProducerPlugin", "1.0.0", "2.0.0", true);
    }

    bool initialize(PluginContext& context) override {
        m_context = context;
        m_initialized = true;

        // Subscribe to producer events
        if (auto* eventBus = context.getEventBus()) {
            m_eventHandle = eventBus->subscribeWithPlugin(
                "data.produced",
                [this](const Event& e) {
                    m_consumedCount++;
                    if (e.data.has_value()) {
                        try {
                            m_lastValue = std::any_cast<int>(e.data);
                        } catch (...) {
                            // Ignore cast errors
                        }
                    }
                },
                100,
                m_metadata.name
            );
        }

        return true;
    }

    void shutdown() override {
        if (auto* eventBus = m_context.getEventBus()) {
            if (m_eventHandle) {
                eventBus->unsubscribe(m_eventHandle);
            }
        }
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

    int getConsumedCount() const { return m_consumedCount.load(); }
    int getLastValue() const { return m_lastValue.load(); }

    static const char* getManifestJson() {
        return R"({"name":"ConsumerPlugin","version":"1.0.0","dependencies":[{"pluginName":"ProducerPlugin"}]})";
    }
};

// Processor Plugin - transforms data
class ProcessorPlugin : public IPlugin {
private:
    bool m_initialized = false;
    PluginMetadata m_metadata;
    PluginContext m_context;
    EventHandle m_inputHandle = 0;
    std::atomic<int> m_processedCount{0};

public:
    ProcessorPlugin() {
        m_metadata.name = "ProcessorPlugin";
        m_metadata.version = "1.0.0";
        m_metadata.loadPriority = 75;
        m_metadata.addDependency("ProducerPlugin", "1.0.0", "2.0.0", false);
    }

    bool initialize(PluginContext& context) override {
        m_context = context;
        m_initialized = true;

        // Subscribe to input and republish transformed
        if (auto* eventBus = context.getEventBus()) {
            m_inputHandle = eventBus->subscribeWithPlugin(
                "data.produced",
                [this](const Event& e) {
                    m_processedCount++;

                    if (e.data.has_value()) {
                        try {
                            int value = std::any_cast<int>(e.data);
                            int processed = value * 2;  // Simple transformation

                            // Publish processed data
                            if (auto* eb = m_context.getEventBus()) {
                                Event processedEvent("data.processed", processed);
                                eb->publish("data.processed", processedEvent);
                            }
                        } catch (...) {
                            // Ignore cast errors
                        }
                    }
                },
                75,  // Lower priority than consumer
                m_metadata.name
            );
        }

        return true;
    }

    void shutdown() override {
        if (auto* eventBus = m_context.getEventBus()) {
            if (m_inputHandle) {
                eventBus->unsubscribe(m_inputHandle);
            }
        }
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

    int getProcessedCount() const { return m_processedCount.load(); }

    static const char* getManifestJson() {
        return R"({"name":"ProcessorPlugin","version":"1.0.0","dependencies":[{"pluginName":"ProducerPlugin","required":false}]})";
    }
};

// ===========================================================================
// Test Application
// ===========================================================================

class MultiPluginTestApp : public Application {
public:
    MultiPluginTestApp() : Application(createConfig()) {}

    static ApplicationConfig createConfig() {
        ApplicationConfig config;
        config.name = "MultiPluginTest";
        config.version = "1.0.0";
        config.pluginDirectory = "./plugins";
        config.autoLoadPlugins = false;
        config.autoInitPlugins = false;
        return config;
    }
};

// ===========================================================================
// Tests
// ===========================================================================

TEST_CASE("Multi-Plugin - Basic communication via EventBus", "[integration][multi-plugin][eventbus]") {
    MultiPluginTestApp app;
    REQUIRE(app.initialize());

    // Create and manually register plugins
    auto producer = std::make_unique<ProducerPlugin>();
    auto consumer = std::make_unique<ConsumerPlugin>();

    PluginContext producerCtx(
        app.getEventBus(),
        app.getServiceLocator(),
        &app,
        nullptr,
        nullptr,
        "ProducerPlugin"
    );

    PluginContext consumerCtx(
        app.getEventBus(),
        app.getServiceLocator(),
        &app,
        nullptr,
        nullptr,
        "ConsumerPlugin"
    );

    SECTION("Producer sends, consumer receives") {
        REQUIRE(producer->initialize(producerCtx));
        REQUIRE(consumer->initialize(consumerCtx));

        // Producer generates data
        producer->produceData(42);
        producer->produceData(100);
        producer->produceData(200);

        // Small delay for async processing
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Consumer should have received all events
        REQUIRE(producer->getProducedCount() == 3);
        REQUIRE(consumer->getConsumedCount() == 3);
        REQUIRE(consumer->getLastValue() == 200);

        consumer->shutdown();
        producer->shutdown();
    }

    SECTION("Multiple consumers receive same events") {
        auto consumer2 = std::make_unique<ConsumerPlugin>();
        PluginContext consumer2Ctx(
            app.getEventBus(),
            app.getServiceLocator(),
            &app,
            nullptr,
            nullptr,
            "ConsumerPlugin2"
        );

        REQUIRE(producer->initialize(producerCtx));
        REQUIRE(consumer->initialize(consumerCtx));
        REQUIRE(consumer2->initialize(consumer2Ctx));

        producer->produceData(123);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        REQUIRE(consumer->getConsumedCount() == 1);
        REQUIRE(consumer2->getConsumedCount() == 1);
        REQUIRE(consumer->getLastValue() == 123);
        REQUIRE(consumer2->getLastValue() == 123);

        consumer2->shutdown();
        consumer->shutdown();
        producer->shutdown();
    }

    app.shutdown();
}

TEST_CASE("Multi-Plugin - Service sharing", "[integration][multi-plugin][services]") {
    MultiPluginTestApp app;
    REQUIRE(app.initialize());

    auto producer = std::make_unique<ProducerPlugin>();
    auto consumer = std::make_unique<ConsumerPlugin>();

    PluginContext producerCtx(
        app.getEventBus(),
        app.getServiceLocator(),
        &app,
        nullptr,
        nullptr,
        "ProducerPlugin"
    );

    PluginContext consumerCtx(
        app.getEventBus(),
        app.getServiceLocator(),
        &app,
        nullptr,
        nullptr,
        "ConsumerPlugin"
    );

    SECTION("Consumer accesses producer's service") {
        REQUIRE(producer->initialize(producerCtx));
        REQUIRE(consumer->initialize(consumerCtx));

        // Producer stores data in service
        producer->produceData(999);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Consumer reads from shared service
        auto* locator = app.getServiceLocator();
        REQUIRE(locator->isRegistered<IDataService>());

        auto service = locator->resolve<IDataService>();
        REQUIRE(service != nullptr);
        REQUIRE(service->getData("latest") == 999);

        consumer->shutdown();
        producer->shutdown();
    }

    app.shutdown();
}

TEST_CASE("Multi-Plugin - Pipeline processing", "[integration][multi-plugin][pipeline]") {
    MultiPluginTestApp app;
    REQUIRE(app.initialize());

    auto producer = std::make_unique<ProducerPlugin>();
    auto processor = std::make_unique<ProcessorPlugin>();

    PluginContext producerCtx(
        app.getEventBus(),
        app.getServiceLocator(),
        &app,
        nullptr,
        nullptr,
        "ProducerPlugin"
    );

    PluginContext processorCtx(
        app.getEventBus(),
        app.getServiceLocator(),
        &app,
        nullptr,
        nullptr,
        "ProcessorPlugin"
    );

    SECTION("Producer -> Processor pipeline") {
        std::atomic<int> finalValue{0};
        EventHandle handle = 0;

        REQUIRE(producer->initialize(producerCtx));
        REQUIRE(processor->initialize(processorCtx));

        // Subscribe to processed data
        handle = app.getEventBus()->subscribe("data.processed",
            [&finalValue](const Event& e) {
                if (e.data.has_value()) {
                    try {
                        finalValue = std::any_cast<int>(e.data);
                    } catch (...) {}
                }
            }
        );

        // Produce data
        producer->produceData(10);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Processor should have transformed it (10 * 2 = 20)
        REQUIRE(processor->getProcessedCount() == 1);
        REQUIRE(finalValue == 20);

        app.getEventBus()->unsubscribe(handle);
        processor->shutdown();
        producer->shutdown();
    }

    SECTION("Three-stage pipeline") {
        std::atomic<int> rawCount{0};
        std::atomic<int> processedCount{0};

        REQUIRE(producer->initialize(producerCtx));
        REQUIRE(processor->initialize(processorCtx));

        // Count both raw and processed events
        auto h1 = app.getEventBus()->subscribe("data.produced",
            [&rawCount](const Event&) { rawCount++; }
        );

        auto h2 = app.getEventBus()->subscribe("data.processed",
            [&processedCount](const Event&) { processedCount++; }
        );

        // Produce multiple values
        for (int i = 1; i <= 5; ++i) {
            producer->produceData(i);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        REQUIRE(rawCount == 5);
        REQUIRE(processedCount == 5);
        REQUIRE(processor->getProcessedCount() == 5);

        app.getEventBus()->unsubscribe(h1);
        app.getEventBus()->unsubscribe(h2);
        processor->shutdown();
        producer->shutdown();
    }

    app.shutdown();
}

TEST_CASE("Multi-Plugin - Dependency ordering", "[integration][multi-plugin][dependencies]") {
    MultiPluginTestApp app;
    REQUIRE(app.initialize());

    SECTION("Consumer depends on producer") {
        // Create separate resolver for testing
        DependencyResolver resolver;

        ProducerPlugin prod;
        ConsumerPlugin cons;

        resolver.addPlugin(prod.getMetadata());
        resolver.addPlugin(cons.getMetadata());

        auto loadOrder = resolver.resolve();
        REQUIRE(loadOrder.size() == 2);

        // Producer should load before consumer
        bool producerFirst = false;
        if (loadOrder[0] == "ProducerPlugin" && loadOrder[1] == "ConsumerPlugin") {
            producerFirst = true;
        }

        REQUIRE(producerFirst);
    }

    app.shutdown();
}

TEST_CASE("Multi-Plugin - Cleanup on shutdown", "[integration][multi-plugin][cleanup]") {
    MultiPluginTestApp app;
    REQUIRE(app.initialize());

    auto producer = std::make_unique<ProducerPlugin>();
    auto consumer = std::make_unique<ConsumerPlugin>();

    PluginContext producerCtx(
        app.getEventBus(),
        app.getServiceLocator(),
        &app,
        nullptr,
        nullptr,
        "ProducerPlugin"
    );

    PluginContext consumerCtx(
        app.getEventBus(),
        app.getServiceLocator(),
        &app,
        nullptr,
        nullptr,
        "ConsumerPlugin"
    );

    SECTION("Plugin-aware cleanup removes all traces") {
        REQUIRE(producer->initialize(producerCtx));
        REQUIRE(consumer->initialize(consumerCtx));

        producer->produceData(42);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Verify service registered
        REQUIRE(app.getServiceLocator()->isRegistered<IDataService>());

        // Shutdown producer (should clean up service)
        producer->shutdown();
        app.getServiceLocator()->unregisterPlugin("ProducerPlugin");

        REQUIRE_FALSE(app.getServiceLocator()->isRegistered<IDataService>());

        // Shutdown consumer (should clean up event subscriptions)
        consumer->shutdown();
        app.getEventBus()->unsubscribePlugin("ConsumerPlugin");

        // Verify no events delivered after cleanup
        std::atomic<int> count{0};
        app.getEventBus()->subscribe("data.produced", [&count](const Event&) { count++; });

        Event testEvent("data.produced", 999);
        app.getEventBus()->publish("data.produced", testEvent);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Only our new subscription should receive it
        REQUIRE(count == 1);
    }

    app.shutdown();
}

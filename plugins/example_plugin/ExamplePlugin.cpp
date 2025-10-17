#include "../../core/IPlugin.hpp"
#include "../../core/IRealtimeUpdatable.hpp"
#include "../../core/PluginContext.hpp"
#include "../../core/PluginMetadata.hpp"
#include "../../core/EventBus.hpp"

#include <iostream>

namespace mcf {

/**
 * @brief Example plugin demonstrating the plugin system
 *
 * This plugin implements IRealtimeUpdatable to receive frame-based updates.
 * It demonstrates event subscription, time tracking, and periodic logging.
 */
class ExamplePlugin : public IPlugin, public IRealtimeUpdatable {
private:
    bool m_initialized = false;
    PluginMetadata m_metadata;
    PluginContext m_context;
    float m_elapsedTime = 0.0f;

public:
    ExamplePlugin() {
        // Setup metadata
        m_metadata.name = "ExamplePlugin";
        m_metadata.version = "1.0.0";
        m_metadata.author = "MVK Framework";
        m_metadata.description = "A simple example plugin demonstrating the plugin API";
        m_metadata.loadPriority = 100;
    }

    ~ExamplePlugin() override {
        if (m_initialized) {
            shutdown();
        }
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

    bool initialize(PluginContext& context) override {
        if (m_initialized) {
            return true;
        }

        m_context = context;

        std::cout << "[ExamplePlugin] Initializing plugin..." << std::endl;
        std::cout << "[ExamplePlugin] Plugin name: " << m_metadata.name << std::endl;
        std::cout << "[ExamplePlugin] Version: " << m_metadata.version << std::endl;

        // Subscribe to events
        if (m_context.getEventBus()) {
            m_context.getEventBus()->subscribe("app.started",
                [](const Event& event) {
                    std::cout << "[ExamplePlugin] Received app.started event" << std::endl;
                }
            );

            m_context.getEventBus()->subscribe("app.shutdown",
                [](const Event& event) {
                    std::cout << "[ExamplePlugin] Received app.shutdown event" << std::endl;
                }
            );
        }

        // Publish initialization event
        if (m_context.getEventBus()) {
            Event initEvent("plugin.initialized");
            m_context.getEventBus()->publish("plugin.initialized", initEvent);
        }

        m_initialized = true;
        std::cout << "[ExamplePlugin] Initialization complete!" << std::endl;

        return true;
    }

    void shutdown() override {
        if (!m_initialized) {
            return;
        }

        std::cout << "[ExamplePlugin] Shutting down..." << std::endl;

        // Publish shutdown event
        if (m_context.getEventBus()) {
            Event shutdownEvent("plugin.shutdown");
            m_context.getEventBus()->publish("plugin.shutdown", shutdownEvent);
        }

        m_initialized = false;
        std::cout << "[ExamplePlugin] Shutdown complete!" << std::endl;
    }

    // IRealtimeUpdatable implementation
    void onRealtimeUpdate(float deltaTime) override {
        m_elapsedTime += deltaTime;

        // Print a message every 5 seconds
        static float messageTimer = 0.0f;
        messageTimer += deltaTime;

        if (messageTimer >= 5.0f) {
            std::cout << "[ExamplePlugin] Update - Elapsed time: "
                     << m_elapsedTime << "s" << std::endl;
            messageTimer = 0.0f;
        }
    }

    bool isInitialized() const override {
        return m_initialized;
    }

    /**
     * @brief Get plugin manifest JSON (for dynamic loading)
     */
    static const char* getManifestJson() {
        return R"({
            "name": "ExamplePlugin",
            "version": "1.0.0",
            "author": "MVK Framework",
            "description": "A simple example plugin",
            "dependencies": [],
            "load_priority": 100
        })";
    }
};

} // namespace mcf

// Export plugin symbols
MCF_PLUGIN_EXPORT(mcf::ExamplePlugin)

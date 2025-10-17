#include "../../core/IPlugin.hpp"
#include "../../core/IRealtimeUpdatable.hpp"
#include "../../core/PluginContext.hpp"
#include "../../core/PluginMetadata.hpp"
#include "../../core/EventBus.hpp"

#include <iostream>
#include <sstream>

/**
 * @brief Example plugin demonstrating hot reload functionality
 *
 * This plugin maintains a counter that persists across reloads
 * through state serialization. Modify the message below and rebuild
 * to see hot reload in action!
 *
 * Implements IRealtimeUpdatable to receive frame-based updates.
 */
class HotReloadExamplePlugin : public mcf::IPlugin, public mcf::IRealtimeUpdatable {
private:
    bool m_initialized = false;
    mcf::PluginMetadata m_metadata;
    mcf::PluginContext m_context;

    // State that will persist across reloads
    int m_updateCounter = 0;
    int m_reloadCount = 0;

    // Message that can be changed to test hot reload
    const char* m_message = "Hello from Hot Reload Example! Version 1.0";

public:
    HotReloadExamplePlugin() {
        m_metadata.name = "HotReloadExample";
        m_metadata.version = "1.0.0";
        m_metadata.author = "ModularCppFramework";
        m_metadata.description = "Example plugin demonstrating hot reload";
        m_metadata.loadPriority = 100;
    }

    std::string getName() const override {
        return m_metadata.name;
    }

    std::string getVersion() const override {
        return m_metadata.version;
    }

    const mcf::PluginMetadata& getMetadata() const override {
        return m_metadata;
    }

    bool initialize(mcf::PluginContext& context) override {
        if (m_initialized) {
            return true;
        }

        m_context = context;

        std::cout << "[HotReloadExample] Initializing plugin..." << std::endl;
        std::cout << "[HotReloadExample] " << m_message << std::endl;
        std::cout << "[HotReloadExample] Reload count: " << m_reloadCount << std::endl;

        // Subscribe to a test event
        if (m_context.getEventBus()) {
            m_context.getEventBus()->subscribe("test.event",
                [this](const mcf::Event& e) {
                    std::cout << "[HotReloadExample] Received test event!" << std::endl;
                },
                100
            );
        }

        m_initialized = true;
        return true;
    }

    void shutdown() override {
        if (!m_initialized) {
            return;
        }

        std::cout << "[HotReloadExample] Shutting down plugin..." << std::endl;
        std::cout << "[HotReloadExample] Total updates: " << m_updateCounter << std::endl;

        m_initialized = false;
    }

    // IRealtimeUpdatable implementation
    void onRealtimeUpdate(float deltaTime) override {
        if (!m_initialized) {
            return;
        }

        m_updateCounter++;

        // Print message every 60 updates (approximately once per second at 60 FPS)
        if (m_updateCounter % 60 == 0) {
            std::cout << "[HotReloadExample] Update #" << m_updateCounter
                      << " - " << m_message << std::endl;
        }
    }

    bool isInitialized() const override {
        return m_initialized;
    }

    // Hot reload support methods

    std::string serializeState() override {
        std::ostringstream oss;
        oss << m_updateCounter << "," << m_reloadCount;

        std::string state = oss.str();
        std::cout << "[HotReloadExample] Serializing state: " << state << std::endl;

        return state;
    }

    void deserializeState(const std::string& state) override {
        if (state.empty()) {
            return;
        }

        std::istringstream iss(state);
        std::string token;

        // Parse counter
        if (std::getline(iss, token, ',')) {
            m_updateCounter = std::stoi(token);
        }

        // Parse reload count
        if (std::getline(iss, token, ',')) {
            m_reloadCount = std::stoi(token);
        }

        // Increment reload count
        m_reloadCount++;

        std::cout << "[HotReloadExample] Deserialized state - Counter: "
                  << m_updateCounter << ", Reloads: " << m_reloadCount << std::endl;
    }

    void onBeforeReload() override {
        std::cout << "[HotReloadExample] Preparing for hot reload..." << std::endl;
        std::cout << "[HotReloadExample] Current update count: " << m_updateCounter << std::endl;
    }

    void onAfterReload() override {
        std::cout << "[HotReloadExample] Hot reload completed!" << std::endl;
        std::cout << "[HotReloadExample] Resumed at update count: " << m_updateCounter << std::endl;
        std::cout << "[HotReloadExample] New message: " << m_message << std::endl;
    }

    // Required for plugin manifest
    static const char* getManifestJson() {
        return R"({
            "name": "HotReloadExample",
            "version": "1.0.0",
            "author": "ModularCppFramework",
            "description": "Example plugin demonstrating hot reload"
        })";
    }
};

// Export plugin symbols
MCF_PLUGIN_EXPORT(HotReloadExamplePlugin)

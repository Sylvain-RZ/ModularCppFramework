/**
 * @file server_example.cpp
 * @brief Example TCP server application using NetworkingModule
 */

#include "core/Application.hpp"
#include "modules/networking/NetworkingModule.hpp"
#include "modules/realtime/RealtimeModule.hpp"
#include <iostream>
#include <memory>

class NetworkServerApp : public mcf::Application {
private:
    mcf::NetworkingModule* m_networkModule = nullptr;
    mcf::RealtimeModule* m_realtimeModule = nullptr;
    int m_messageCount = 0;

public:
    NetworkServerApp() : Application() {
        std::cout << "=== TCP Server Example ===" << std::endl;
        std::cout << "Starting server on port 8080..." << std::endl;

        // Configure networking for server mode
        mcf::NetworkConfig networkConfig = mcf::NetworkConfig::createServerConfig(8080, 10);
        networkConfig.enableNetworkLogging = true;
        networkConfig.logRawData = false;

        // Add networking module
        m_networkModule = addModule<mcf::NetworkingModule>(networkConfig);

        // Add realtime module for frame updates
        mcf::RealtimeConfig realtimeConfig;
        realtimeConfig.targetFPS = 30;  // 30 FPS is enough for network processing
        m_realtimeModule = addModule<mcf::RealtimeModule>(realtimeConfig);
    }

    bool onInitialize() override {
        // Get module references
        m_networkModule = getModule<mcf::NetworkingModule>();
        m_realtimeModule = getModule<mcf::RealtimeModule>();

        if (!m_networkModule) {
            std::cerr << "Failed to get NetworkingModule" << std::endl;
            return false;
        }

        // Subscribe to network events
        auto* eventBus = getEventBus();

        // Client connected event
        eventBus->subscribe("network.server.client_connected",
            [](const mcf::Event& event) {
                auto message = std::any_cast<std::string>(event.data);
                std::cout << "[EVENT] " << message << std::endl;
            });

        // Client disconnected event
        eventBus->subscribe("network.server.client_disconnected",
            [](const mcf::Event& event) {
                auto message = std::any_cast<std::string>(event.data);
                std::cout << "[EVENT] " << message << std::endl;
            });

        // Data received event
        eventBus->subscribe("network.server.data_received",
            [this](const mcf::Event& event) {
                m_messageCount++;
                std::cout << "[EVENT] Data received (message #" << m_messageCount << ")" << std::endl;
            });

        // Setup custom server callbacks for direct handling
        auto* server = m_networkModule->getServer();
        if (server) {
            server->setOnClientDataReceived(
                [server](std::shared_ptr<mcf::INetworkConnection> client, const mcf::NetworkBuffer& data) {
                    // Echo the received data back to the client
                    std::string message(data.begin(), data.end());
                    std::cout << "[SERVER] Received: " << message << std::endl;

                    // Echo back with prefix
                    std::string response = "[Echo] " + message;
                    client->send(response);

                    // Broadcast to all clients
                    std::string broadcast = "[Broadcast] Client says: " + message;
                    server->broadcast(broadcast);
                });
        }

        std::cout << "\nServer is ready and listening on port 8080" << std::endl;
        std::cout << "Connect with: telnet localhost 8080" << std::endl;
        std::cout << "Press Ctrl+C to stop the server\n" << std::endl;

        return true;
    }

    void run() override {
        if (m_realtimeModule) {
            // Run the realtime loop (handles network updates automatically)
            m_realtimeModule->run();
        }
    }

    void onShutdown() override {
        std::cout << "\nShutting down server..." << std::endl;
        std::cout << "Total messages processed: " << m_messageCount << std::endl;

        // Print server statistics
        if (m_networkModule && m_networkModule->getServer()) {
            auto stats = m_networkModule->getServer()->getStats();
            std::cout << "\nServer Statistics:" << std::endl;
            std::cout << "  Bytes sent: " << stats.bytesSent << std::endl;
            std::cout << "  Bytes received: " << stats.bytesReceived << std::endl;
            std::cout << "  Packets sent: " << stats.packetsSent << std::endl;
            std::cout << "  Packets received: " << stats.packetsReceived << std::endl;
            std::cout << "  Errors: " << stats.errors << std::endl;
            std::cout << "  Uptime: " << stats.getUptimeSeconds() << " seconds" << std::endl;
        }
    }
};

int main() {
    try {
        NetworkServerApp app;
        app.initialize();
        app.run();
        app.shutdown();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}

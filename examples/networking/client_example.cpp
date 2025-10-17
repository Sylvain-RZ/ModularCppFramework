/**
 * @file client_example.cpp
 * @brief Example TCP client application using NetworkingModule
 */

#include "core/Application.hpp"
#include "core/IRealtimeUpdatable.hpp"
#include "modules/networking/NetworkingModule.hpp"
#include "modules/realtime/RealtimeModule.hpp"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

class NetworkClientApp : public mcf::Application, public mcf::IRealtimeUpdatable {
private:
    mcf::NetworkingModule* m_networkModule = nullptr;
    mcf::RealtimeModule* m_realtimeModule = nullptr;
    int m_messagesSent = 0;
    int m_messagesReceived = 0;
    std::chrono::steady_clock::time_point m_lastSendTime;

public:
    NetworkClientApp() : Application() {
        std::cout << "=== TCP Client Example ===" << std::endl;
        std::cout << "Connecting to server at localhost:8080..." << std::endl;

        // Configure networking for client mode
        mcf::NetworkConfig networkConfig = mcf::NetworkConfig::createClientConfig("127.0.0.1", 8080);
        networkConfig.enableNetworkLogging = true;
        networkConfig.logRawData = false;
        networkConfig.autoReconnect = true;
        networkConfig.reconnectInterval = std::chrono::milliseconds(3000);

        // Add networking module
        m_networkModule = addModule<mcf::NetworkingModule>(networkConfig);

        // Add realtime module for frame updates
        mcf::RealtimeConfig realtimeConfig;
        realtimeConfig.targetFPS = 30;
        m_realtimeModule = addModule<mcf::RealtimeModule>(realtimeConfig);

        m_lastSendTime = std::chrono::steady_clock::now();
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

        // Connected event
        eventBus->subscribe("network.client.connected",
            [this](const mcf::Event& event) {
                auto message = std::any_cast<std::string>(event.data);
                std::cout << "[EVENT] " << message << std::endl;

                // Send initial greeting
                auto client = m_networkModule->getClient();
                if (client && client->isConnected()) {
                    std::string greeting = "Hello from C++ client!";
                    client->send(greeting);
                    m_messagesSent++;
                    std::cout << "[CLIENT] Sent: " << greeting << std::endl;
                }
            });

        // Disconnected event
        eventBus->subscribe("network.client.disconnected",
            [](const mcf::Event& event) {
                auto message = std::any_cast<std::string>(event.data);
                std::cout << "[EVENT] " << message << std::endl;
            });

        // Data received event
        eventBus->subscribe("network.client.data_received",
            [this](const mcf::Event& event) {
                auto data = std::any_cast<mcf::NetworkBuffer>(event.data);
                std::string message(data.begin(), data.end());
                m_messagesReceived++;
                std::cout << "[CLIENT] Received: " << message << std::endl;
            });

        // Error event
        eventBus->subscribe("network.error",
            [](const mcf::Event& event) {
                auto errorMsg = std::any_cast<std::string>(event.data);
                std::cerr << "[ERROR] " << errorMsg << std::endl;
            });

        std::cout << "\nClient is ready!" << std::endl;
        std::cout << "Will send periodic messages to the server" << std::endl;
        std::cout << "Press Ctrl+C to stop the client\n" << std::endl;

        return true;
    }

    // IRealtimeUpdatable interface
    void onRealtimeUpdate(float deltaTime) override {
        // Send periodic messages to server
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastSendTime);

        if (elapsed.count() >= 5) {  // Send message every 5 seconds
            auto client = m_networkModule->getClient();
            if (client && client->isConnected()) {
                m_messagesSent++;
                std::string message = "Message #" + std::to_string(m_messagesSent) + " from client";
                client->send(message);
                std::cout << "[CLIENT] Sent: " << message << std::endl;
                m_lastSendTime = now;
            }
        }
    }

    void run() override {
        if (m_realtimeModule) {
            // Run the realtime loop (handles network updates automatically)
            m_realtimeModule->run();
        }
    }

    void onShutdown() override {
        std::cout << "\nShutting down client..." << std::endl;
        std::cout << "Messages sent: " << m_messagesSent << std::endl;
        std::cout << "Messages received: " << m_messagesReceived << std::endl;

        // Print client statistics
        if (m_networkModule && m_networkModule->getClient()) {
            auto stats = m_networkModule->getClient()->getStats();
            std::cout << "\nClient Statistics:" << std::endl;
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
        NetworkClientApp app;
        app.initialize();
        app.run();
        app.shutdown();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}

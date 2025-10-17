#include "modules/networking/NetworkingModule.hpp"
#include <iostream>

namespace mcf {

NetworkingModule::NetworkingModule(const NetworkConfig& config)
    : ModuleBase("NetworkingModule", "1.0.0", 800)  // High priority - early init
    , m_config(config) {
}

bool NetworkingModule::initialize(Application& app) {
    if (m_initialized) {
        return true;
    }

    // Store framework component pointers
    m_app = &app;
    m_serviceLocator = app.getServiceLocator();
    m_eventBus = app.getEventBus();

    if (m_config.enableNetworkLogging) {
        std::cout << m_config.logPrefix << " Initializing NetworkingModule..." << std::endl;
    }

    // Initialize TCP Server if enabled
    if (m_config.enableTcpServer) {
        m_server = std::make_unique<TcpServer>(m_config);
        setupServerCallbacks();

        // Start server
        if (m_server->start()) {
            if (m_config.enableNetworkLogging) {
                std::cout << m_config.logPrefix << " TCP Server started on "
                         << m_config.serverBindAddress << ":" << m_config.serverPort << std::endl;
            }

            publishServerStarted();
        } else {
            std::cerr << m_config.logPrefix << " Failed to start TCP Server" << std::endl;
            publishError("Failed to start TCP Server");
            return false;
        }

        // Note: TcpServer is not registered with ServiceLocator because it's owned by unique_ptr
        // Access it via NetworkingModule::getServer() instead
    }

    // Initialize TCP Client if enabled
    if (m_config.enableTcpClient) {
        m_client = std::make_shared<TcpClient>(m_config);
        setupClientCallbacks();

        // Register client as service
        if (m_serviceLocator) {
            m_serviceLocator->registerSingleton<TcpClient>(m_client);
        }

        // Auto-connect if configured
        if (!m_config.clientServerAddress.empty()) {
            if (m_client->connect(m_config.clientServerAddress, m_config.clientServerPort)) {
                if (m_config.enableNetworkLogging) {
                    std::cout << m_config.logPrefix << " TCP Client connected to "
                             << m_config.clientServerAddress << ":" << m_config.clientServerPort << std::endl;
                }
            } else {
                std::cerr << m_config.logPrefix << " Failed to connect TCP Client" << std::endl;
                publishError("Failed to connect TCP Client");
                // Don't fail initialization - client can retry
            }
        }
    }

    m_initialized = true;

    if (m_config.enableNetworkLogging) {
        std::cout << m_config.logPrefix << " NetworkingModule initialized successfully" << std::endl;
    }

    return true;
}

void NetworkingModule::shutdown() {
    if (!m_initialized) {
        return;
    }

    if (m_config.enableNetworkLogging) {
        std::cout << m_config.logPrefix << " Shutting down NetworkingModule..." << std::endl;
    }

    // Shutdown client
    if (m_client) {
        if (m_client->isConnected()) {
            m_client->disconnect();
        }
        m_client.reset();
    }

    // Shutdown server
    if (m_server) {
        if (m_server->isRunning()) {
            m_server->stop();
        }
        publishServerStopped();
        m_server.reset();
    }

    // Clear framework pointers
    m_app = nullptr;
    m_serviceLocator = nullptr;
    m_eventBus = nullptr;

    m_initialized = false;

    if (m_config.enableNetworkLogging) {
        std::cout << m_config.logPrefix << " NetworkingModule shutdown complete" << std::endl;
    }
}

void NetworkingModule::onRealtimeUpdate(float deltaTime) {
    if (!m_initialized) {
        return;
    }

    // Update server (process client connections)
    if (m_server && m_server->isRunning()) {
        m_server->update();
    }

    // Update client (process received data)
    if (m_client && m_client->isConnected()) {
        m_client->update();
    }
}

// ============================================================================
// Event Publishing
// ============================================================================

void NetworkingModule::publishServerStarted() {
    if (!m_eventBus) return;

    Event event("network.server.started", std::string("Server started"));
    m_eventBus->publish("network.server.started", event);
}

void NetworkingModule::publishServerStopped() {
    if (!m_eventBus) return;

    Event event("network.server.stopped", std::string("Server stopped"));
    m_eventBus->publish("network.server.stopped", event);
}

void NetworkingModule::publishClientConnected(std::shared_ptr<INetworkConnection> client) {
    if (!m_eventBus) return;

    auto info = client->getConnectionInfo();
    std::string data = "Client connected: " + info.remoteAddress + ":" + std::to_string(info.remotePort);
    Event event("network.server.client_connected", data);
    m_eventBus->publish("network.server.client_connected", event);
}

void NetworkingModule::publishClientDisconnected(std::shared_ptr<INetworkConnection> client) {
    if (!m_eventBus) return;

    auto info = client->getConnectionInfo();
    std::string data = "Client disconnected: " + info.remoteAddress + ":" + std::to_string(info.remotePort);
    Event event("network.server.client_disconnected", data);
    m_eventBus->publish("network.server.client_disconnected", event);
}

void NetworkingModule::publishServerDataReceived(std::shared_ptr<INetworkConnection> client,
                                                  const NetworkBuffer& data) {
    if (!m_eventBus) return;

    // Create event with connection info and data
    struct DataReceivedInfo {
        ConnectionInfo connectionInfo;
        NetworkBuffer data;
    };

    DataReceivedInfo info{client->getConnectionInfo(), data};
    Event event("network.server.data_received", info);
    m_eventBus->publish("network.server.data_received", event);
}

void NetworkingModule::publishClientDataReceived(const NetworkBuffer& data) {
    if (!m_eventBus) return;

    Event event("network.client.data_received", data);
    m_eventBus->publish("network.client.data_received", event);
}

void NetworkingModule::publishError(const std::string& error) {
    if (!m_eventBus) return;

    Event event("network.error", error);
    m_eventBus->publish("network.error", event);
}

// ============================================================================
// Callback Setup
// ============================================================================

void NetworkingModule::setupServerCallbacks() {
    if (!m_server) return;

    // Client connected callback
    m_server->setOnClientConnected([this](std::shared_ptr<INetworkConnection> client) {
        publishClientConnected(client);
    });

    // Client disconnected callback
    m_server->setOnClientDisconnected([this](std::shared_ptr<INetworkConnection> client) {
        publishClientDisconnected(client);
    });

    // Client data received callback
    m_server->setOnClientDataReceived([this](std::shared_ptr<INetworkConnection> client,
                                              const NetworkBuffer& data) {
        publishServerDataReceived(client, data);
    });

    // Error callback
    m_server->setOnError([this](std::shared_ptr<INetworkConnection> client,
                                NetworkError error,
                                const std::string& message) {
        publishError("Server error: " + message);
    });
}

void NetworkingModule::setupClientCallbacks() {
    if (!m_client) return;

    // Connected callback
    m_client->setOnConnected([this](std::shared_ptr<INetworkConnection> conn) {
        if (!m_eventBus) return;

        auto info = conn->getConnectionInfo();
        std::string data = "Connected to server: " + info.remoteAddress + ":" + std::to_string(info.remotePort);
        Event event("network.client.connected", data);
        m_eventBus->publish("network.client.connected", event);
    });

    // Disconnected callback
    m_client->setOnDisconnected([this](std::shared_ptr<INetworkConnection> conn) {
        if (!m_eventBus) return;

        Event event("network.client.disconnected", std::string("Disconnected from server"));
        m_eventBus->publish("network.client.disconnected", event);
    });

    // Data received callback
    m_client->setOnDataReceived([this](std::shared_ptr<INetworkConnection> conn,
                                        const NetworkBuffer& data) {
        publishClientDataReceived(data);
    });

    // Error callback
    m_client->setOnError([this](std::shared_ptr<INetworkConnection> conn,
                                NetworkError error,
                                const std::string& message) {
        publishError("Client error: " + message);
    });
}

} // namespace mcf

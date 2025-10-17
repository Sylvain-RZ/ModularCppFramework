#ifndef MCF_NETWORKING_MODULE_HPP
#define MCF_NETWORKING_MODULE_HPP

#include "core/IModule.hpp"
#include "core/IRealtimeUpdatable.hpp"
#include "core/Application.hpp"
#include "core/ServiceLocator.hpp"
#include "core/EventBus.hpp"
#include "modules/networking/NetworkConfig.hpp"
#include "modules/networking/NetworkingTypes.hpp"
#include "modules/networking/TcpServer.hpp"
#include "modules/networking/TcpClient.hpp"
#include <memory>
#include <string>

namespace mcf {

/**
 * @brief NetworkingModule provides TCP/IP networking capabilities
 *
 * Features:
 * - TCP Server with multi-client support
 * - TCP Client with auto-reconnect
 * - Thread-safe network operations
 * - Event-driven architecture via EventBus
 * - Service registration via ServiceLocator
 * - Real-time frame updates for network processing
 *
 * Priority: 800 (high - early initialization before application services)
 *
 * Events published:
 * - "network.server.started" - Server started successfully
 * - "network.server.stopped" - Server stopped
 * - "network.server.client_connected" - New client connected to server
 * - "network.server.client_disconnected" - Client disconnected from server
 * - "network.server.data_received" - Server received data from client
 * - "network.client.connected" - Client connected to remote server
 * - "network.client.disconnected" - Client disconnected from remote server
 * - "network.client.data_received" - Client received data from server
 * - "network.error" - Network error occurred
 *
 * Services registered:
 * - TcpServer - Access to TCP server instance (if enabled)
 * - TcpClient - Access to TCP client instance (if enabled)
 */
class NetworkingModule : public ModuleBase, public IRealtimeUpdatable {
public:
    /**
     * @brief Construct networking module with configuration
     * @param config Network configuration (server/client settings)
     */
    explicit NetworkingModule(const NetworkConfig& config = NetworkConfig());

    /**
     * @brief Destructor
     */
    ~NetworkingModule() override = default;

    // IModule interface
    bool initialize(Application& app) override;
    void shutdown() override;

    // IRealtimeUpdatable interface
    void onRealtimeUpdate(float deltaTime) override;

    // Server access
    /**
     * @brief Get TCP server instance
     * @return Pointer to TcpServer or nullptr if not enabled
     */
    TcpServer* getServer() const { return m_server.get(); }

    /**
     * @brief Check if server is running
     */
    bool isServerRunning() const { return m_server && m_server->isRunning(); }

    // Client access
    /**
     * @brief Get TCP client instance
     * @return Pointer to TcpClient or nullptr if not enabled
     */
    std::shared_ptr<TcpClient> getClient() const { return m_client; }

    /**
     * @brief Check if client is connected
     */
    bool isClientConnected() const { return m_client && m_client->isConnected(); }

    // Configuration
    /**
     * @brief Get current network configuration
     */
    const NetworkConfig& getConfig() const { return m_config; }

    /**
     * @brief Update configuration (requires reinitialization)
     */
    void setConfig(const NetworkConfig& config) { m_config = config; }

private:
    // Event publishing helpers
    void publishServerStarted();
    void publishServerStopped();
    void publishClientConnected(std::shared_ptr<INetworkConnection> client);
    void publishClientDisconnected(std::shared_ptr<INetworkConnection> client);
    void publishServerDataReceived(std::shared_ptr<INetworkConnection> client, const NetworkBuffer& data);
    void publishClientDataReceived(const NetworkBuffer& data);
    void publishError(const std::string& error);

    // Callback setup
    void setupServerCallbacks();
    void setupClientCallbacks();

    // Configuration
    NetworkConfig m_config;

    // Framework component pointers
    Application* m_app = nullptr;
    ServiceLocator* m_serviceLocator = nullptr;
    EventBus* m_eventBus = nullptr;

    // Networking components
    std::unique_ptr<TcpServer> m_server;
    std::shared_ptr<TcpClient> m_client;
};

} // namespace mcf

#endif // MCF_NETWORKING_MODULE_HPP

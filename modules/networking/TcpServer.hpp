#ifndef MCF_TCP_SERVER_HPP
#define MCF_TCP_SERVER_HPP

// Include winsock2 BEFORE any other headers on Windows to avoid conflicts
#ifdef _WIN32
    #ifndef _WINSOCKAPI_
        #define _WINSOCKAPI_  // Prevent windows.h from including winsock.h
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
#endif

#include "modules/networking/NetworkingTypes.hpp"
#include "modules/networking/NetworkConfig.hpp"
#include "modules/networking/TcpClient.hpp"
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <memory>
#include <map>

#ifdef _WIN32
    using socket_t = SOCKET;
    #define INVALID_SOCKET_VALUE INVALID_SOCKET
    #define SOCKET_ERROR_VALUE SOCKET_ERROR
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    using socket_t = int;
    #define INVALID_SOCKET_VALUE -1
    #define SOCKET_ERROR_VALUE -1
#endif

namespace mcf {

// Server client connection wrapper
class ServerClientConnection : public INetworkConnection, public std::enable_shared_from_this<ServerClientConnection> {
public:
    explicit ServerClientConnection(socket_t clientSocket, const NetworkConfig& config);
    ~ServerClientConnection() override;

    // Disable copy and move (contains std::atomic which is not moveable)
    ServerClientConnection(const ServerClientConnection&) = delete;
    ServerClientConnection& operator=(const ServerClientConnection&) = delete;
    ServerClientConnection(ServerClientConnection&&) noexcept = delete;
    ServerClientConnection& operator=(ServerClientConnection&&) noexcept = delete;

    // INetworkConnection interface
    bool connect(const std::string& address, uint16_t port) override { return false; } // Not used for server clients
    void disconnect() override;
    bool isConnected() const override;
    ConnectionState getState() const override;

    bool send(const NetworkBuffer& data) override;
    bool send(const void* data, size_t size) override;
    bool send(const std::string& message) override;

    ConnectionInfo getConnectionInfo() const override;
    NetworkStats getStats() const override;

    void setOnConnected(OnConnectedCallback callback) override;
    void setOnDisconnected(OnDisconnectedCallback callback) override;
    void setOnDataReceived(OnDataReceivedCallback callback) override;
    void setOnError(OnErrorCallback callback) override;

    // Server client specific
    void update();
    bool sendMessage(const NetworkMessage& message);
    uint64_t getClientId() const { return m_clientId; }

private:
    void receiveThread();
    void handleError(NetworkError error, const std::string& message);
    void closeSocket();
    std::string getLastErrorString() const;

    // Configuration
    NetworkConfig m_config;

    // Client identification
    uint64_t m_clientId;
    static std::atomic<uint64_t> s_nextClientId;

    // Socket
    socket_t m_socket;
    std::atomic<ConnectionState> m_state;

    // Connection info
    mutable std::mutex m_connectionMutex;
    ConnectionInfo m_connectionInfo;

    // Statistics
    mutable std::mutex m_statsMutex;
    NetworkStats m_stats;

    // Receive thread
    std::unique_ptr<std::thread> m_receiveThread;
    std::atomic<bool> m_running;

    // Received data queue
    mutable std::mutex m_receiveMutex;
    std::queue<NetworkBuffer> m_receiveQueue;

    // Callbacks
    mutable std::mutex m_callbackMutex;
    OnConnectedCallback m_onConnected;
    OnDisconnectedCallback m_onDisconnected;
    OnDataReceivedCallback m_onDataReceived;
    OnErrorCallback m_onError;
};

// TCP Server
class TcpServer {
public:
    explicit TcpServer(const NetworkConfig& config = NetworkConfig());
    ~TcpServer();

    // Disable copy, allow move
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;
    TcpServer(TcpServer&&) noexcept = default;
    TcpServer& operator=(TcpServer&&) noexcept = default;

    // Server lifecycle
    bool start();
    bool start(const std::string& address, uint16_t port);
    void stop();
    bool isRunning() const;

    // Client management
    void update(); // Process all client connections
    size_t getClientCount() const;
    std::vector<std::shared_ptr<ServerClientConnection>> getClients() const;
    std::shared_ptr<ServerClientConnection> getClient(uint64_t clientId) const;
    void disconnectClient(uint64_t clientId);
    void disconnectAllClients();

    // Broadcast
    void broadcast(const NetworkBuffer& data);
    void broadcast(const std::string& message);
    void broadcastMessage(const NetworkMessage& message);

    // Callbacks
    void setOnClientConnected(OnClientConnectedCallback callback);
    void setOnClientDisconnected(OnClientDisconnectedCallback callback);
    void setOnClientDataReceived(OnDataReceivedCallback callback);
    void setOnError(OnErrorCallback callback);

    // Server information
    std::string getBindAddress() const;
    uint16_t getPort() const;
    NetworkStats getStats() const;

private:
    void acceptThread();
    void handleError(NetworkError error, const std::string& message);
    bool setSocketOptions();
    void closeSocket();
    std::string getLastErrorString() const;

    // Configuration
    NetworkConfig m_config;

    // Server socket
    socket_t m_serverSocket;
    std::atomic<bool> m_running;

    // Accept thread
    std::unique_ptr<std::thread> m_acceptThread;

    // Client connections
    mutable std::mutex m_clientsMutex;
    std::map<uint64_t, std::shared_ptr<ServerClientConnection>> m_clients;

    // Server information
    std::string m_bindAddress;
    uint16_t m_port;

    // Statistics
    mutable std::mutex m_statsMutex;
    NetworkStats m_stats;

    // Callbacks
    mutable std::mutex m_callbackMutex;
    OnClientConnectedCallback m_onClientConnected;
    OnClientDisconnectedCallback m_onClientDisconnected;
    OnDataReceivedCallback m_onClientDataReceived;
    OnErrorCallback m_onError;
};

} // namespace mcf

#endif // MCF_TCP_SERVER_HPP

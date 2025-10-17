#ifndef MCF_TCP_CLIENT_HPP
#define MCF_TCP_CLIENT_HPP

#include "modules/networking/NetworkingTypes.hpp"
#include "modules/networking/NetworkConfig.hpp"
#include <atomic>
#include <mutex>
#include <thread>
#include <queue>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
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

class TcpClient : public INetworkConnection, public std::enable_shared_from_this<TcpClient> {
public:
    explicit TcpClient(const NetworkConfig& config = NetworkConfig());
    ~TcpClient() override;

    // Disable copy, allow move
    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;
    TcpClient(TcpClient&&) noexcept = default;
    TcpClient& operator=(TcpClient&&) noexcept = default;

    // INetworkConnection interface
    bool connect(const std::string& address, uint16_t port) override;
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

    // Additional client-specific methods
    void update(); // Call periodically to process received data
    bool sendMessage(const NetworkMessage& message);

private:
    // Internal methods
    void receiveThread();
    void handleError(NetworkError error, const std::string& message);
    bool setSocketOptions();
    void closeSocket();
    std::string getLastErrorString() const;

    // Configuration
    NetworkConfig m_config;

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

} // namespace mcf

#endif // MCF_TCP_CLIENT_HPP

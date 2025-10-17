#ifndef MCF_NETWORKING_TYPES_HPP
#define MCF_NETWORKING_TYPES_HPP

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>

namespace mcf {

// Network protocol types
enum class NetworkProtocol {
    TCP,
    UDP
};

// Connection states
enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Disconnecting,
    Error
};

// Network error codes
enum class NetworkError {
    None = 0,
    ConnectionFailed,
    ConnectionClosed,
    ConnectionTimeout,
    BindFailed,
    ListenFailed,
    AcceptFailed,
    SendFailed,
    ReceiveFailed,
    InvalidSocket,
    AddressResolutionFailed,
    Unknown
};

// Network event types
enum class NetworkEventType {
    Connected,
    Disconnected,
    DataReceived,
    DataSent,
    Error,
    ClientConnected,
    ClientDisconnected
};

// Forward declarations
class INetworkConnection;
class TcpServer;
class TcpClient;

// Network buffer type
using NetworkBuffer = std::vector<uint8_t>;

// Network callbacks
using OnConnectedCallback = std::function<void(std::shared_ptr<INetworkConnection>)>;
using OnDisconnectedCallback = std::function<void(std::shared_ptr<INetworkConnection>)>;
using OnDataReceivedCallback = std::function<void(std::shared_ptr<INetworkConnection>, const NetworkBuffer&)>;
using OnErrorCallback = std::function<void(std::shared_ptr<INetworkConnection>, NetworkError, const std::string&)>;

// Server-specific callbacks
using OnClientConnectedCallback = std::function<void(std::shared_ptr<INetworkConnection>)>;
using OnClientDisconnectedCallback = std::function<void(std::shared_ptr<INetworkConnection>)>;

// Network statistics
struct NetworkStats {
    uint64_t bytesSent = 0;
    uint64_t bytesReceived = 0;
    uint64_t packetsSent = 0;
    uint64_t packetsReceived = 0;
    uint64_t errors = 0;
    std::chrono::steady_clock::time_point startTime;

    NetworkStats() : startTime(std::chrono::steady_clock::now()) {}

    void reset() {
        bytesSent = 0;
        bytesReceived = 0;
        packetsSent = 0;
        packetsReceived = 0;
        errors = 0;
        startTime = std::chrono::steady_clock::now();
    }

    double getUptimeSeconds() const {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - startTime);
        return static_cast<double>(duration.count());
    }
};

// Connection information
struct ConnectionInfo {
    std::string localAddress;
    uint16_t localPort = 0;
    std::string remoteAddress;
    uint16_t remotePort = 0;
    NetworkProtocol protocol = NetworkProtocol::TCP;
    ConnectionState state = ConnectionState::Disconnected;
    NetworkStats stats;
};

// Network connection interface
class INetworkConnection {
public:
    virtual ~INetworkConnection() = default;

    // Connection management
    virtual bool connect(const std::string& address, uint16_t port) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual ConnectionState getState() const = 0;

    // Data transfer
    virtual bool send(const NetworkBuffer& data) = 0;
    virtual bool send(const void* data, size_t size) = 0;
    virtual bool send(const std::string& message) = 0;

    // Connection information
    virtual ConnectionInfo getConnectionInfo() const = 0;
    virtual NetworkStats getStats() const = 0;

    // Callbacks
    virtual void setOnConnected(OnConnectedCallback callback) = 0;
    virtual void setOnDisconnected(OnDisconnectedCallback callback) = 0;
    virtual void setOnDataReceived(OnDataReceivedCallback callback) = 0;
    virtual void setOnError(OnErrorCallback callback) = 0;
};

// Network message structure for serialization
struct NetworkMessage {
    uint32_t messageId = 0;
    uint32_t dataSize = 0;
    NetworkBuffer data;

    NetworkMessage() = default;

    explicit NetworkMessage(uint32_t id) : messageId(id) {}

    NetworkMessage(uint32_t id, const NetworkBuffer& buffer)
        : messageId(id), dataSize(static_cast<uint32_t>(buffer.size())), data(buffer) {}

    NetworkMessage(uint32_t id, const std::string& str)
        : messageId(id), dataSize(static_cast<uint32_t>(str.size())) {
        data.assign(str.begin(), str.end());
    }

    // Serialize message to buffer (header + data)
    NetworkBuffer serialize() const {
        NetworkBuffer buffer;
        buffer.reserve(sizeof(messageId) + sizeof(dataSize) + data.size());

        // Write message ID (4 bytes)
        const uint8_t* idPtr = reinterpret_cast<const uint8_t*>(&messageId);
        buffer.insert(buffer.end(), idPtr, idPtr + sizeof(messageId));

        // Write data size (4 bytes)
        const uint8_t* sizePtr = reinterpret_cast<const uint8_t*>(&dataSize);
        buffer.insert(buffer.end(), sizePtr, sizePtr + sizeof(dataSize));

        // Write data
        buffer.insert(buffer.end(), data.begin(), data.end());

        return buffer;
    }

    // Deserialize message from buffer
    static bool deserialize(const NetworkBuffer& buffer, NetworkMessage& message) {
        if (buffer.size() < sizeof(uint32_t) * 2) {
            return false;
        }

        // Read message ID
        memcpy(&message.messageId, buffer.data(), sizeof(uint32_t));

        // Read data size
        memcpy(&message.dataSize, buffer.data() + sizeof(uint32_t), sizeof(uint32_t));

        // Validate size
        if (buffer.size() < sizeof(uint32_t) * 2 + message.dataSize) {
            return false;
        }

        // Read data
        message.data.assign(
            buffer.begin() + sizeof(uint32_t) * 2,
            buffer.begin() + sizeof(uint32_t) * 2 + message.dataSize
        );

        return true;
    }

    std::string toString() const {
        return std::string(data.begin(), data.end());
    }
};

} // namespace mcf

#endif // MCF_NETWORKING_TYPES_HPP

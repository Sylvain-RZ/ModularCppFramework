#ifndef MCF_NETWORK_CONFIG_HPP
#define MCF_NETWORK_CONFIG_HPP

#include <cstdint>
#include <string>
#include <chrono>

namespace mcf {

struct NetworkConfig {
    // TCP Server settings
    bool enableTcpServer = false;
    std::string serverBindAddress = "0.0.0.0";
    uint16_t serverPort = 8080;
    int serverBacklog = 10;
    size_t maxConnections = 100;

    // TCP Client settings
    bool enableTcpClient = false;
    std::string clientServerAddress = "127.0.0.1";
    uint16_t clientServerPort = 8080;
    bool autoReconnect = true;
    std::chrono::milliseconds reconnectInterval{5000};

    // Buffer settings
    size_t receiveBufferSize = 8192;
    size_t sendBufferSize = 8192;

    // Timeout settings
    std::chrono::milliseconds connectTimeout{10000};
    std::chrono::milliseconds receiveTimeout{30000};
    std::chrono::milliseconds sendTimeout{5000};

    // Performance settings
    bool enableNagle = false;  // Disable Nagle's algorithm by default for low latency
    bool enableKeepalive = true;
    std::chrono::seconds keepaliveInterval{60};
    int keepaliveProbes = 3;

    // Threading settings
    bool useAsyncIO = true;
    int workerThreads = 2;

    // Logging and debugging
    bool enableNetworkLogging = true;
    bool logRawData = false;
    std::string logPrefix = "[Network]";

    // Factory methods for common configurations
    static NetworkConfig createServerConfig(uint16_t port = 8080, size_t maxConn = 100) {
        NetworkConfig config;
        config.enableTcpServer = true;
        config.serverPort = port;
        config.maxConnections = maxConn;
        config.enableTcpClient = false;
        return config;
    }

    static NetworkConfig createClientConfig(const std::string& address, uint16_t port = 8080) {
        NetworkConfig config;
        config.enableTcpClient = true;
        config.clientServerAddress = address;
        config.clientServerPort = port;
        config.enableTcpServer = false;
        return config;
    }

    static NetworkConfig createHybridConfig(uint16_t serverPort = 8080) {
        NetworkConfig config;
        config.enableTcpServer = true;
        config.enableTcpClient = true;
        config.serverPort = serverPort;
        return config;
    }

    static NetworkConfig createLowLatencyConfig() {
        NetworkConfig config;
        config.enableNagle = false;
        config.receiveBufferSize = 4096;
        config.sendBufferSize = 4096;
        config.sendTimeout = std::chrono::milliseconds(1000);
        config.useAsyncIO = true;
        config.workerThreads = 4;
        return config;
    }

    static NetworkConfig createHighThroughputConfig() {
        NetworkConfig config;
        config.receiveBufferSize = 65536;
        config.sendBufferSize = 65536;
        config.enableNagle = true;
        config.workerThreads = 8;
        return config;
    }
};

} // namespace mcf

#endif // MCF_NETWORK_CONFIG_HPP

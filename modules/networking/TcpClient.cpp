#include "modules/networking/TcpClient.hpp"
#include <cstring>
#include <iostream>

namespace mcf {

TcpClient::TcpClient(const NetworkConfig& config)
    : m_config(config)
    , m_socket(INVALID_SOCKET_VALUE)
    , m_state(ConnectionState::Disconnected)
    , m_running(false) {

    m_connectionInfo.protocol = NetworkProtocol::TCP;

#ifdef _WIN32
    // Initialize Winsock on Windows
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        handleError(NetworkError::Unknown, "Failed to initialize Winsock");
    }
#endif
}

TcpClient::~TcpClient() {
    disconnect();

#ifdef _WIN32
    WSACleanup();
#endif
}

bool TcpClient::connect(const std::string& address, uint16_t port) {
    if (m_state != ConnectionState::Disconnected) {
        return false;
    }

    m_state = ConnectionState::Connecting;

    // Create socket
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET_VALUE) {
        handleError(NetworkError::InvalidSocket, "Failed to create socket: " + getLastErrorString());
        m_state = ConnectionState::Error;
        return false;
    }

    // Set socket options
    if (!setSocketOptions()) {
        closeSocket();
        m_state = ConnectionState::Error;
        return false;
    }

    // Resolve address
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, address.c_str(), &serverAddr.sin_addr) <= 0) {
        handleError(NetworkError::AddressResolutionFailed, "Invalid address: " + address);
        closeSocket();
        m_state = ConnectionState::Error;
        return false;
    }

    // Connect to server
    if (::connect(m_socket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR_VALUE) {
        handleError(NetworkError::ConnectionFailed, "Failed to connect: " + getLastErrorString());
        closeSocket();
        m_state = ConnectionState::Error;
        return false;
    }

    // Get local address info
    struct sockaddr_in localAddr;
    socklen_t addrLen = sizeof(localAddr);
    if (getsockname(m_socket, reinterpret_cast<struct sockaddr*>(&localAddr), &addrLen) == 0) {
        char addrStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &localAddr.sin_addr, addrStr, INET_ADDRSTRLEN);

        std::lock_guard<std::mutex> lock(m_connectionMutex);
        m_connectionInfo.localAddress = addrStr;
        m_connectionInfo.localPort = ntohs(localAddr.sin_port);
        m_connectionInfo.remoteAddress = address;
        m_connectionInfo.remotePort = port;
    }

    // Start receive thread
    m_running = true;
    m_receiveThread = std::make_unique<std::thread>(&TcpClient::receiveThread, this);

    m_state = ConnectionState::Connected;

    // Trigger connected callback
    if (m_onConnected) {
        m_onConnected(shared_from_this());
    }

    if (m_config.enableNetworkLogging) {
        std::cout << m_config.logPrefix << " Connected to " << address << ":" << port << std::endl;
    }

    return true;
}

void TcpClient::disconnect() {
    if (m_state == ConnectionState::Disconnected) {
        return;
    }

    m_state = ConnectionState::Disconnecting;
    m_running = false;

    // Wait for receive thread to finish
    if (m_receiveThread && m_receiveThread->joinable()) {
        m_receiveThread->join();
    }

    closeSocket();

    // Trigger disconnected callback
    if (m_onDisconnected) {
        m_onDisconnected(shared_from_this());
    }

    m_state = ConnectionState::Disconnected;

    if (m_config.enableNetworkLogging) {
        std::cout << m_config.logPrefix << " Disconnected" << std::endl;
    }
}

bool TcpClient::isConnected() const {
    return m_state == ConnectionState::Connected;
}

ConnectionState TcpClient::getState() const {
    return m_state;
}

bool TcpClient::send(const NetworkBuffer& data) {
    return send(data.data(), data.size());
}

bool TcpClient::send(const void* data, size_t size) {
    if (!isConnected() || size == 0) {
        return false;
    }

    ssize_t totalSent = 0;
    const uint8_t* ptr = static_cast<const uint8_t*>(data);

    while (totalSent < static_cast<ssize_t>(size)) {
        ssize_t sent = ::send(m_socket, reinterpret_cast<const char*>(ptr + totalSent),
                             size - totalSent, 0);

        if (sent == SOCKET_ERROR_VALUE) {
            handleError(NetworkError::SendFailed, "Send failed: " + getLastErrorString());
            return false;
        }

        totalSent += sent;
    }

    // Update statistics
    {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        m_stats.bytesSent += size;
        m_stats.packetsSent++;
    }

    if (m_config.enableNetworkLogging && m_config.logRawData) {
        std::cout << m_config.logPrefix << " Sent " << size << " bytes" << std::endl;
    }

    return true;
}

bool TcpClient::send(const std::string& message) {
    return send(message.data(), message.size());
}

bool TcpClient::sendMessage(const NetworkMessage& message) {
    NetworkBuffer buffer = message.serialize();
    return send(buffer);
}

ConnectionInfo TcpClient::getConnectionInfo() const {
    std::lock_guard<std::mutex> lock(m_connectionMutex);
    auto info = m_connectionInfo;
    info.state = m_state;
    return info;
}

NetworkStats TcpClient::getStats() const {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    return m_stats;
}

void TcpClient::setOnConnected(OnConnectedCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_onConnected = callback;
}

void TcpClient::setOnDisconnected(OnDisconnectedCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_onDisconnected = callback;
}

void TcpClient::setOnDataReceived(OnDataReceivedCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_onDataReceived = callback;
}

void TcpClient::setOnError(OnErrorCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_onError = callback;
}

void TcpClient::update() {
    // Process received data from queue
    std::queue<NetworkBuffer> tempQueue;
    {
        std::lock_guard<std::mutex> lock(m_receiveMutex);
        std::swap(tempQueue, m_receiveQueue);
    }

    while (!tempQueue.empty()) {
        NetworkBuffer& data = tempQueue.front();

        if (m_onDataReceived) {
            m_onDataReceived(shared_from_this(), data);
        }

        tempQueue.pop();
    }
}

void TcpClient::receiveThread() {
    NetworkBuffer buffer(m_config.receiveBufferSize);

    while (m_running && isConnected()) {
        ssize_t received = recv(m_socket, reinterpret_cast<char*>(buffer.data()),
                               buffer.size(), 0);

        if (received > 0) {
            // Update statistics
            {
                std::lock_guard<std::mutex> lock(m_statsMutex);
                m_stats.bytesReceived += received;
                m_stats.packetsReceived++;
            }

            // Add to receive queue
            NetworkBuffer receivedData(buffer.begin(), buffer.begin() + received);
            {
                std::lock_guard<std::mutex> lock(m_receiveMutex);
                m_receiveQueue.push(std::move(receivedData));
            }

            if (m_config.enableNetworkLogging && m_config.logRawData) {
                std::cout << m_config.logPrefix << " Received " << received << " bytes" << std::endl;
            }
        } else if (received == 0) {
            // Connection closed
            if (m_config.enableNetworkLogging) {
                std::cout << m_config.logPrefix << " Connection closed by server" << std::endl;
            }
            break;
        } else {
            // Error occurred
            handleError(NetworkError::ReceiveFailed, "Receive failed: " + getLastErrorString());
            break;
        }
    }

    // Connection ended
    if (m_running) {
        m_state = ConnectionState::Disconnected;
    }
}

void TcpClient::handleError(NetworkError error, const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        m_stats.errors++;
    }

    if (m_config.enableNetworkLogging) {
        std::cerr << m_config.logPrefix << " ERROR: " << message << std::endl;
    }

    if (m_onError) {
        m_onError(shared_from_this(), error, message);
    }
}

bool TcpClient::setSocketOptions() {
    // Set receive buffer size
    int recvBufSize = static_cast<int>(m_config.receiveBufferSize);
    if (setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF,
                   reinterpret_cast<const char*>(&recvBufSize), sizeof(recvBufSize)) == SOCKET_ERROR_VALUE) {
        handleError(NetworkError::Unknown, "Failed to set receive buffer size");
        return false;
    }

    // Set send buffer size
    int sendBufSize = static_cast<int>(m_config.sendBufferSize);
    if (setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF,
                   reinterpret_cast<const char*>(&sendBufSize), sizeof(sendBufSize)) == SOCKET_ERROR_VALUE) {
        handleError(NetworkError::Unknown, "Failed to set send buffer size");
        return false;
    }

#ifndef _WIN32
    // Disable Nagle's algorithm if configured (Linux/Unix)
    if (!m_config.enableNagle) {
        int flag = 1;
        if (setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) == SOCKET_ERROR_VALUE) {
            handleError(NetworkError::Unknown, "Failed to disable Nagle's algorithm");
        }
    }

    // Enable keep-alive if configured
    if (m_config.enableKeepalive) {
        int flag = 1;
        if (setsockopt(m_socket, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag)) == SOCKET_ERROR_VALUE) {
            handleError(NetworkError::Unknown, "Failed to enable keep-alive");
        }
    }
#endif

    return true;
}

void TcpClient::closeSocket() {
    if (m_socket != INVALID_SOCKET_VALUE) {
#ifdef _WIN32
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET_VALUE;
    }
}

std::string TcpClient::getLastErrorString() const {
#ifdef _WIN32
    int error = WSAGetLastError();
    char* msgBuf = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  reinterpret_cast<LPSTR>(&msgBuf), 0, nullptr);
    std::string message = msgBuf ? msgBuf : "Unknown error";
    LocalFree(msgBuf);
    return message;
#else
    return std::strerror(errno);
#endif
}

} // namespace mcf

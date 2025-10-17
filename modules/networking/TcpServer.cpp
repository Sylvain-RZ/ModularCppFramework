#include "modules/networking/TcpServer.hpp"
#include <cstring>
#include <iostream>
#include <algorithm>

namespace mcf {

// ============================================================================
// ServerClientConnection Implementation
// ============================================================================

std::atomic<uint64_t> ServerClientConnection::s_nextClientId{1};

ServerClientConnection::ServerClientConnection(socket_t clientSocket, const NetworkConfig& config)
    : m_config(config)
    , m_clientId(s_nextClientId++)
    , m_socket(clientSocket)
    , m_state(ConnectionState::Connected)
    , m_running(false) {

    m_connectionInfo.protocol = NetworkProtocol::TCP;

    // Get client address info
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    if (getpeername(m_socket, reinterpret_cast<struct sockaddr*>(&clientAddr), &addrLen) == 0) {
        char addrStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, addrStr, INET_ADDRSTRLEN);
        m_connectionInfo.remoteAddress = addrStr;
        m_connectionInfo.remotePort = ntohs(clientAddr.sin_port);
    }

    // Get local address info
    struct sockaddr_in localAddr;
    addrLen = sizeof(localAddr);
    if (getsockname(m_socket, reinterpret_cast<struct sockaddr*>(&localAddr), &addrLen) == 0) {
        char addrStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &localAddr.sin_addr, addrStr, INET_ADDRSTRLEN);
        m_connectionInfo.localAddress = addrStr;
        m_connectionInfo.localPort = ntohs(localAddr.sin_port);
    }

    // Start receive thread
    m_running = true;
    m_receiveThread = std::make_unique<std::thread>(&ServerClientConnection::receiveThread, this);
}

ServerClientConnection::~ServerClientConnection() {
    disconnect();
}

void ServerClientConnection::disconnect() {
    if (m_state == ConnectionState::Disconnected) {
        return;
    }

    m_state = ConnectionState::Disconnecting;
    m_running = false;

    // Wait for receive thread
    if (m_receiveThread && m_receiveThread->joinable()) {
        m_receiveThread->join();
    }

    closeSocket();

    if (m_onDisconnected) {
        m_onDisconnected(shared_from_this());
    }

    m_state = ConnectionState::Disconnected;
}

bool ServerClientConnection::isConnected() const {
    return m_state == ConnectionState::Connected;
}

ConnectionState ServerClientConnection::getState() const {
    return m_state;
}

bool ServerClientConnection::send(const NetworkBuffer& data) {
    return send(data.data(), data.size());
}

bool ServerClientConnection::send(const void* data, size_t size) {
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

    {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        m_stats.bytesSent += size;
        m_stats.packetsSent++;
    }

    return true;
}

bool ServerClientConnection::send(const std::string& message) {
    return send(message.data(), message.size());
}

bool ServerClientConnection::sendMessage(const NetworkMessage& message) {
    NetworkBuffer buffer = message.serialize();
    return send(buffer);
}

ConnectionInfo ServerClientConnection::getConnectionInfo() const {
    std::lock_guard<std::mutex> lock(m_connectionMutex);
    auto info = m_connectionInfo;
    info.state = m_state;
    return info;
}

NetworkStats ServerClientConnection::getStats() const {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    return m_stats;
}

void ServerClientConnection::setOnConnected(OnConnectedCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_onConnected = callback;
}

void ServerClientConnection::setOnDisconnected(OnDisconnectedCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_onDisconnected = callback;
}

void ServerClientConnection::setOnDataReceived(OnDataReceivedCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_onDataReceived = callback;
}

void ServerClientConnection::setOnError(OnErrorCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_onError = callback;
}

void ServerClientConnection::update() {
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

void ServerClientConnection::receiveThread() {
    NetworkBuffer buffer(m_config.receiveBufferSize);

    while (m_running && isConnected()) {
        ssize_t received = recv(m_socket, reinterpret_cast<char*>(buffer.data()),
                               buffer.size(), 0);

        if (received > 0) {
            {
                std::lock_guard<std::mutex> lock(m_statsMutex);
                m_stats.bytesReceived += received;
                m_stats.packetsReceived++;
            }

            NetworkBuffer receivedData(buffer.begin(), buffer.begin() + received);
            {
                std::lock_guard<std::mutex> lock(m_receiveMutex);
                m_receiveQueue.push(std::move(receivedData));
            }
        } else if (received == 0) {
            break;
        } else {
            handleError(NetworkError::ReceiveFailed, "Receive failed: " + getLastErrorString());
            break;
        }
    }

    if (m_running) {
        m_state = ConnectionState::Disconnected;
    }
}

void ServerClientConnection::handleError(NetworkError error, const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        m_stats.errors++;
    }

    if (m_config.enableNetworkLogging) {
        std::cerr << m_config.logPrefix << " Client " << m_clientId << " ERROR: " << message << std::endl;
    }

    if (m_onError) {
        m_onError(shared_from_this(), error, message);
    }
}

void ServerClientConnection::closeSocket() {
    if (m_socket != INVALID_SOCKET_VALUE) {
#ifdef _WIN32
        closesocket(m_socket);
#else
        close(m_socket);
#endif
        m_socket = INVALID_SOCKET_VALUE;
    }
}

std::string ServerClientConnection::getLastErrorString() const {
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

// ============================================================================
// TcpServer Implementation
// ============================================================================

TcpServer::TcpServer(const NetworkConfig& config)
    : m_config(config)
    , m_serverSocket(INVALID_SOCKET_VALUE)
    , m_running(false)
    , m_bindAddress(config.serverBindAddress)
    , m_port(config.serverPort) {

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        handleError(NetworkError::Unknown, "Failed to initialize Winsock");
    }
#endif
}

TcpServer::~TcpServer() {
    stop();

#ifdef _WIN32
    WSACleanup();
#endif
}

bool TcpServer::start() {
    return start(m_config.serverBindAddress, m_config.serverPort);
}

bool TcpServer::start(const std::string& address, uint16_t port) {
    if (m_running) {
        return false;
    }

    m_bindAddress = address;
    m_port = port;

    // Create server socket
    m_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_serverSocket == INVALID_SOCKET_VALUE) {
        handleError(NetworkError::InvalidSocket, "Failed to create server socket: " + getLastErrorString());
        return false;
    }

    if (!setSocketOptions()) {
        closeSocket();
        return false;
    }

    // Bind socket
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (address == "0.0.0.0" || address.empty()) {
        serverAddr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, address.c_str(), &serverAddr.sin_addr) <= 0) {
            handleError(NetworkError::AddressResolutionFailed, "Invalid bind address: " + address);
            closeSocket();
            return false;
        }
    }

    if (bind(m_serverSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR_VALUE) {
        handleError(NetworkError::BindFailed, "Failed to bind socket: " + getLastErrorString());
        closeSocket();
        return false;
    }

    // Listen for connections
    if (listen(m_serverSocket, m_config.serverBacklog) == SOCKET_ERROR_VALUE) {
        handleError(NetworkError::ListenFailed, "Failed to listen: " + getLastErrorString());
        closeSocket();
        return false;
    }

    // Start accept thread
    m_running = true;
    m_acceptThread = std::make_unique<std::thread>(&TcpServer::acceptThread, this);

    if (m_config.enableNetworkLogging) {
        std::cout << m_config.logPrefix << " Server started on " << address << ":" << port << std::endl;
    }

    return true;
}

void TcpServer::stop() {
    if (!m_running) {
        return;
    }

    m_running = false;

    // Close server socket to unblock accept()
    closeSocket();

    // Wait for accept thread
    if (m_acceptThread && m_acceptThread->joinable()) {
        m_acceptThread->join();
    }

    // Disconnect all clients
    disconnectAllClients();

    if (m_config.enableNetworkLogging) {
        std::cout << m_config.logPrefix << " Server stopped" << std::endl;
    }
}

bool TcpServer::isRunning() const {
    return m_running;
}

void TcpServer::update() {
    // Update all client connections
    std::vector<std::shared_ptr<ServerClientConnection>> clients;
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        for (const auto& pair : m_clients) {
            clients.push_back(pair.second);
        }
    }

    // Remove disconnected clients
    std::vector<uint64_t> disconnectedClients;
    for (auto& client : clients) {
        if (!client->isConnected()) {
            disconnectedClients.push_back(client->getClientId());
        } else {
            client->update();
        }
    }

    // Remove disconnected clients from map
    if (!disconnectedClients.empty()) {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        for (uint64_t clientId : disconnectedClients) {
            m_clients.erase(clientId);
        }
    }
}

size_t TcpServer::getClientCount() const {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    return m_clients.size();
}

std::vector<std::shared_ptr<ServerClientConnection>> TcpServer::getClients() const {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    std::vector<std::shared_ptr<ServerClientConnection>> clients;
    for (const auto& pair : m_clients) {
        clients.push_back(pair.second);
    }
    return clients;
}

std::shared_ptr<ServerClientConnection> TcpServer::getClient(uint64_t clientId) const {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    auto it = m_clients.find(clientId);
    return (it != m_clients.end()) ? it->second : nullptr;
}

void TcpServer::disconnectClient(uint64_t clientId) {
    std::shared_ptr<ServerClientConnection> client;
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        auto it = m_clients.find(clientId);
        if (it != m_clients.end()) {
            client = it->second;
        }
    }

    if (client) {
        client->disconnect();
    }
}

void TcpServer::disconnectAllClients() {
    std::vector<std::shared_ptr<ServerClientConnection>> clients;
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        for (const auto& pair : m_clients) {
            clients.push_back(pair.second);
        }
    }

    for (auto& client : clients) {
        client->disconnect();
    }

    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        m_clients.clear();
    }
}

void TcpServer::broadcast(const NetworkBuffer& data) {
    auto clients = getClients();
    for (auto& client : clients) {
        if (client->isConnected()) {
            client->send(data);
        }
    }
}

void TcpServer::broadcast(const std::string& message) {
    NetworkBuffer buffer(message.begin(), message.end());
    broadcast(buffer);
}

void TcpServer::broadcastMessage(const NetworkMessage& message) {
    NetworkBuffer buffer = message.serialize();
    broadcast(buffer);
}

void TcpServer::setOnClientConnected(OnClientConnectedCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_onClientConnected = callback;
}

void TcpServer::setOnClientDisconnected(OnClientDisconnectedCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_onClientDisconnected = callback;
}

void TcpServer::setOnClientDataReceived(OnDataReceivedCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_onClientDataReceived = callback;
}

void TcpServer::setOnError(OnErrorCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_onError = callback;
}

std::string TcpServer::getBindAddress() const {
    return m_bindAddress;
}

uint16_t TcpServer::getPort() const {
    return m_port;
}

NetworkStats TcpServer::getStats() const {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    return m_stats;
}

void TcpServer::acceptThread() {
    while (m_running) {
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);

        socket_t clientSocket = accept(m_serverSocket, reinterpret_cast<struct sockaddr*>(&clientAddr), &addrLen);

        if (clientSocket == INVALID_SOCKET_VALUE) {
            if (m_running) {
                handleError(NetworkError::AcceptFailed, "Accept failed: " + getLastErrorString());
            }
            continue;
        }

        // Check max connections
        if (getClientCount() >= m_config.maxConnections) {
#ifdef _WIN32
            closesocket(clientSocket);
#else
            close(clientSocket);
#endif
            if (m_config.enableNetworkLogging) {
                std::cerr << m_config.logPrefix << " Max connections reached, rejected client" << std::endl;
            }
            continue;
        }

        // Create client connection
        auto client = std::make_shared<ServerClientConnection>(clientSocket, m_config);

        // Set client callbacks
        client->setOnDisconnected([this](std::shared_ptr<INetworkConnection> conn) {
            if (m_onClientDisconnected) {
                m_onClientDisconnected(conn);
            }
        });

        client->setOnDataReceived([this](std::shared_ptr<INetworkConnection> conn, const NetworkBuffer& data) {
            if (m_onClientDataReceived) {
                m_onClientDataReceived(conn, data);
            }
        });

        client->setOnError([this](std::shared_ptr<INetworkConnection> conn, NetworkError error, const std::string& message) {
            if (m_onError) {
                m_onError(conn, error, message);
            }
        });

        // Add to clients map
        {
            std::lock_guard<std::mutex> lock(m_clientsMutex);
            m_clients[client->getClientId()] = client;
        }

        // Trigger connected callback
        if (m_onClientConnected) {
            m_onClientConnected(client);
        }

        if (m_config.enableNetworkLogging) {
            auto info = client->getConnectionInfo();
            std::cout << m_config.logPrefix << " Client connected: " << info.remoteAddress
                     << ":" << info.remotePort << " [ID: " << client->getClientId() << "]" << std::endl;
        }
    }
}

void TcpServer::handleError(NetworkError error, const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        m_stats.errors++;
    }

    if (m_config.enableNetworkLogging) {
        std::cerr << m_config.logPrefix << " SERVER ERROR: " << message << std::endl;
    }

    if (m_onError) {
        m_onError(nullptr, error, message);
    }
}

bool TcpServer::setSocketOptions() {
    // Set socket to reuse address
    int reuse = 1;
    if (setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<const char*>(&reuse), sizeof(reuse)) == SOCKET_ERROR_VALUE) {
        handleError(NetworkError::Unknown, "Failed to set SO_REUSEADDR");
        return false;
    }

#ifndef _WIN32
    // Set SO_REUSEPORT on Linux for load balancing
    if (setsockopt(m_serverSocket, SOL_SOCKET, SO_REUSEPORT,
                   &reuse, sizeof(reuse)) == SOCKET_ERROR_VALUE) {
        // Non-critical, just log
        if (m_config.enableNetworkLogging) {
            std::cerr << m_config.logPrefix << " Warning: Failed to set SO_REUSEPORT" << std::endl;
        }
    }
#endif

    return true;
}

void TcpServer::closeSocket() {
    if (m_serverSocket != INVALID_SOCKET_VALUE) {
#ifdef _WIN32
        closesocket(m_serverSocket);
#else
        close(m_serverSocket);
#endif
        m_serverSocket = INVALID_SOCKET_VALUE;
    }
}

std::string TcpServer::getLastErrorString() const {
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

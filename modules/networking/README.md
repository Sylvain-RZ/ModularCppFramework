# Networking Module

A comprehensive TCP/IP networking module for the ModularCppFramework, providing thread-safe client-server communication with event-driven architecture.

## Features

- **TCP Server**: Multi-client server with connection management
- **TCP Client**: Reliable client with auto-reconnect support
- **Thread-Safe**: All operations are thread-safe using mutexes
- **Event-Driven**: Publishes events via EventBus for network activities
- **Service Integration**: Registers services via ServiceLocator
- **Real-time Updates**: Implements IRealtimeUpdatable for frame-based processing
- **Cross-Platform**: Works on Linux and Windows
- **Statistics Tracking**: Built-in network statistics (bytes, packets, errors)
- **Configurable**: Extensive configuration options for buffers, timeouts, etc.

## Architecture

### Components

1. **NetworkingModule**: Main module class that integrates with the framework
2. **TcpServer**: Multi-client TCP server implementation
3. **TcpClient**: TCP client with connection management
4. **ServerClientConnection**: Represents a client connection on the server side
5. **NetworkConfig**: Configuration structure for all networking settings
6. **NetworkingTypes**: Core types, enums, and interfaces

### Thread Model

- **Accept Thread** (Server): Accepts new client connections
- **Receive Threads**: One per connection (client and server clients)
- **Main Thread**: Processes received data via `update()` calls

## Usage

### Server Example

```cpp
#include "core/Application.hpp"
#include "modules/networking/NetworkingModule.hpp"
#include "modules/realtime/RealtimeModule.hpp"

class MyServerApp : public mcf::Application {
    void setup() override {
        // Configure server
        mcf::NetworkConfig config = mcf::NetworkConfig::createServerConfig(8080, 100);
        config.enableNetworkLogging = true;

        // Add modules
        addModule<mcf::NetworkingModule>(config);
        addModule<mcf::RealtimeModule>();
    }

    void onInitialize() override {
        // Get networking module
        auto* netModule = getModule<mcf::NetworkingModule>();
        auto* server = netModule->getServer();

        // Set up callbacks
        server->setOnClientDataReceived(
            [server](auto client, const auto& data) {
                // Echo back to client
                client->send(data);

                // Or broadcast to all clients
                server->broadcast(data);
            });

        // Subscribe to events
        getEventBus()->subscribe("network.server.client_connected",
            [](const mcf::Event& event) {
                std::cout << "New client connected!" << std::endl;
            });
    }

    void run() override {
        getModule<mcf::RealtimeModule>()->run();
    }
};
```

### Client Example

```cpp
#include "core/Application.hpp"
#include "modules/networking/NetworkingModule.hpp"
#include "modules/realtime/RealtimeModule.hpp"

class MyClientApp : public mcf::Application {
    void setup() override {
        // Configure client
        mcf::NetworkConfig config = mcf::NetworkConfig::createClientConfig("127.0.0.1", 8080);
        config.autoReconnect = true;
        config.enableNetworkLogging = true;

        // Add modules
        addModule<mcf::NetworkingModule>(config);
        addModule<mcf::RealtimeModule>();
    }

    void onInitialize() override {
        // Subscribe to events
        getEventBus()->subscribe("network.client.connected",
            [this](const mcf::Event& event) {
                // Send greeting when connected
                auto client = getModule<mcf::NetworkingModule>()->getClient();
                client->send("Hello Server!");
            });

        getEventBus()->subscribe("network.client.data_received",
            [](const mcf::Event& event) {
                auto data = std::any_cast<mcf::NetworkBuffer>(event.data);
                std::string message(data.begin(), data.end());
                std::cout << "Received: " << message << std::endl;
            });
    }

    void run() override {
        getModule<mcf::RealtimeModule>()->run();
    }
};
```

## Configuration

### NetworkConfig Options

```cpp
mcf::NetworkConfig config;

// Server settings
config.enableTcpServer = true;
config.serverBindAddress = "0.0.0.0";
config.serverPort = 8080;
config.serverBacklog = 10;
config.maxConnections = 100;

// Client settings
config.enableTcpClient = true;
config.clientServerAddress = "127.0.0.1";
config.clientServerPort = 8080;
config.autoReconnect = true;
config.reconnectInterval = std::chrono::milliseconds(5000);

// Buffer settings
config.receiveBufferSize = 8192;
config.sendBufferSize = 8192;

// Timeout settings
config.connectTimeout = std::chrono::milliseconds(10000);
config.receiveTimeout = std::chrono::milliseconds(30000);
config.sendTimeout = std::chrono::milliseconds(5000);

// Performance settings
config.enableNagle = false;  // Disable for low latency
config.enableKeepalive = true;
config.keepaliveInterval = std::chrono::seconds(60);

// Logging
config.enableNetworkLogging = true;
config.logRawData = false;  // Set true to log raw bytes
```

### Factory Methods

```cpp
// Server configuration (port 8080, max 100 connections)
auto config = mcf::NetworkConfig::createServerConfig(8080, 100);

// Client configuration
auto config = mcf::NetworkConfig::createClientConfig("127.0.0.1", 8080);

// Hybrid (both server and client)
auto config = mcf::NetworkConfig::createHybridConfig(8080);

// Low latency configuration
auto config = mcf::NetworkConfig::createLowLatencyConfig();

// High throughput configuration
auto config = mcf::NetworkConfig::createHighThroughputConfig();
```

## Events

The networking module publishes the following events via EventBus:

### Server Events

- `network.server.started` - Server started successfully
- `network.server.stopped` - Server stopped
- `network.server.client_connected` - New client connected
- `network.server.client_disconnected` - Client disconnected
- `network.server.data_received` - Data received from client

### Client Events

- `network.client.connected` - Connected to server
- `network.client.disconnected` - Disconnected from server
- `network.client.data_received` - Data received from server

### Error Events

- `network.error` - Network error occurred

## Network Messages

For structured messaging, use the `NetworkMessage` class:

```cpp
// Create message
mcf::NetworkMessage msg(1001, "Hello World");

// Serialize and send
auto client = getModule<mcf::NetworkingModule>()->getClient();
client->sendMessage(msg);

// Receive and deserialize
getEventBus()->subscribe("network.client.data_received",
    [](const mcf::Event& event) {
        auto buffer = std::any_cast<mcf::NetworkBuffer>(event.data);
        mcf::NetworkMessage msg;
        if (mcf::NetworkMessage::deserialize(buffer, msg)) {
            std::cout << "Message ID: " << msg.messageId << std::endl;
            std::cout << "Data: " << msg.toString() << std::endl;
        }
    });
```

## Statistics

Both server and client provide network statistics:

```cpp
auto* server = netModule->getServer();
auto stats = server->getStats();

std::cout << "Bytes sent: " << stats.bytesSent << std::endl;
std::cout << "Bytes received: " << stats.bytesReceived << std::endl;
std::cout << "Packets sent: " << stats.packetsSent << std::endl;
std::cout << "Packets received: " << stats.packetsReceived << std::endl;
std::cout << "Errors: " << stats.errors << std::endl;
std::cout << "Uptime: " << stats.getUptimeSeconds() << " seconds" << std::endl;
```

## Integration with Framework

### Module Priority

The networking module has priority **800** (high), ensuring it initializes early before most application services.

### Service Registration

When enabled, the module registers services:
- `TcpServer*` - Access to server instance
- `TcpClient*` - Access to client instance (as shared_ptr)

```cpp
// Retrieve from ServiceLocator
auto* serviceLocator = app.getServiceLocator();
auto server = serviceLocator->resolve<TcpServer>();
auto client = serviceLocator->resolve<TcpClient>();
```

### Real-time Updates

The module implements `IRealtimeUpdatable` and is automatically updated by `RealtimeModule`:

```cpp
void NetworkingModule::onRealtimeUpdate(float deltaTime) {
    // Process server client connections
    if (m_server && m_server->isRunning()) {
        m_server->update();
    }

    // Process client received data
    if (m_client && m_client->isConnected()) {
        m_client->update();
    }
}
```

## Error Handling

Network errors are reported via:
1. **Return values**: Most operations return `bool` for success/failure
2. **Error callbacks**: Set via `setOnError()`
3. **Events**: Published to `network.error` topic
4. **Logging**: Errors logged if `enableNetworkLogging` is true

```cpp
server->setOnError([](auto conn, mcf::NetworkError error, const std::string& msg) {
    std::cerr << "Network error: " << msg << std::endl;
});
```

## Platform Support

### Linux
- Uses POSIX sockets API
- TCP_NODELAY supported for Nagle's algorithm control
- SO_KEEPALIVE for connection keep-alive
- No additional libraries needed (part of libc)

### Windows
- Uses Winsock2 API
- Automatically links `ws2_32.lib`
- WSAStartup/WSACleanup handled automatically

## Thread Safety

All public methods are thread-safe:
- Mutexes protect internal state
- Copy-under-lock pattern for callbacks
- Queue-based receive processing

## Performance Considerations

1. **Buffer Sizes**: Adjust `receiveBufferSize` and `sendBufferSize` based on workload
2. **Nagle's Algorithm**: Disable (`enableNagle = false`) for low latency
3. **Worker Threads**: Set `workerThreads` for parallel processing (future enhancement)
4. **Update Frequency**: Higher `RealtimeModule` FPS = more responsive network processing

## Building

The networking module is built as a static library:

```bash
cd build
cmake -DBUILD_EXAMPLES=ON ..
make -j$(nproc)
```

Executables:
- `bin/networking_server_example` - TCP server example
- `bin/networking_client_example` - TCP client example

## Testing

Run the server and client examples:

```bash
# Terminal 1 - Start server
./bin/networking_server_example

# Terminal 2 - Start client
./bin/networking_client_example

# Or test with telnet
telnet localhost 8080
```

## Future Enhancements

- UDP support
- SSL/TLS encryption
- WebSocket protocol
- HTTP client/server
- Message queuing and buffering
- Connection pooling
- Rate limiting
- Compression support

## License

Part of ModularCppFramework. See main LICENSE file.

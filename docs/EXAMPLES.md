# Exemples ModularCppFramework

Ce fichier documente tous les exemples disponibles dans le framework avec leurs cas d'usage, concepts démontrés, et comment les exécuter.

## Compilation des Exemples

```bash
mkdir build && cd build
cmake -DBUILD_EXAMPLES=ON ..
make -j$(nproc)

# Tous les exemples seront dans build/bin/
ls bin/
```

---

## 1. logger_example.cpp

### Description
Démontre le système de logging flexible avec configuration JSON et différents sinks.

### Concepts Démontrés
- Création de loggers avec LoggerRegistry
- Configuration de sinks (console, file, rotating file)
- Différents niveaux de log (trace, debug, info, warn, error, critical)
- Logs colorés dans la console
- Rotation automatique de fichiers logs
- Intégration LoggerModule avec Application

### Cas d'Usage
- Applications nécessitant du logging structuré
- Debugging avec niveaux de verbosité configurables
- Production avec rotation de logs
- Multi-logger pour différents composants

### Exécution

```bash
./bin/logger_example

# Output attendu:
# [INFO] This is an info message
# [WARN] This is a warning
# [ERROR] This is an error
# [CRITICAL] Critical error!
# Les logs sont aussi écrits dans logs/app.log
```

### Configuration (logger_config.json)

```json
{
    "loggers": {
        "App": {
            "level": "debug",
            "sinks": [
                {"type": "console", "level": "info", "colored": true},
                {"type": "file", "level": "debug", "path": "logs/app.log"},
                {"type": "rotating", "level": "warn", "path": "logs/rotating.log", "maxSize": 10485760, "maxFiles": 5}
            ]
        }
    }
}
```

---

## 2. realtime_app_example.cpp

### Description
Application avec boucle temps réel utilisant RealtimeModule pour fixed timestep updates.

### Concepts Démontrés
- RealtimeModule avec fixed timestep
- IRealtimeUpdatable interface
- Configuration target FPS
- Delta time fixe pour physique déterministe
- Boucle de jeu (game loop)
- FPS tracking

### Cas d'Usage
- Game engines
- Simulations physiques
- Applications nécessitant updates à fréquence fixe
- Rendering loops

### Exécution

```bash
./bin/realtime_app_example

# Output attendu:
# Application initialized
# [RealtimeModule] Target FPS: 60
# [Frame 1] Delta: 0.016667s
# [Frame 2] Delta: 0.016667s
# ...
# Average FPS: 60
```

### Code Clé

```cpp
class MyRealtimeApp : public mcf::Application, public mcf::IRealtimeUpdatable {
protected:
    bool onInitialize() override {
        auto realtimeModule = std::make_shared<mcf::RealtimeModule>();
        realtimeModule->setTargetFPS(60);
        realtimeModule->setFixedTimestep(1.0f/60.0f);
        addModule(realtimeModule);
        return true;
    }

    void onUpdate(float deltaTime) override {
        // Appelé à 60Hz avec deltaTime fixe (0.01666s)
        // Idéal pour physique, logique de jeu
    }
};
```

---

## 3. event_driven_app_example.cpp

### Description
Architecture event-driven utilisant EventBus pour communication découplée entre composants.

### Concepts Démontrés
- EventBus pub/sub pattern
- Événements avec priorités
- Handlers multiples pour un même événement
- Passage de données via std::any
- Découplage de composants
- IEventDriven interface

### Cas d'Usage
- Applications modulaires découplées
- Systèmes réactifs
- Microservices internes
- Event sourcing patterns
- GUI applications

### Exécution

```bash
./bin/event_driven_app_example

# Output attendu:
# [HIGH PRIORITY] Received player.scored: 100
# [LOW PRIORITY] Received player.scored: 100
# Statistics: 150 points
# Event processed by multiple handlers
```

### Code Clé

```cpp
// S'abonner avec priorité
auto handle = eventBus->subscribe("player.scored",
    [](const mcf::Event& e) {
        int score = std::any_cast<int>(e.data);
        std::cout << "Score: " << score << std::endl;
    },
    100  // Priorité haute
);

// Publier événement
mcf::Event event("player.scored", 100);
eventBus->publish("player.scored", event);
```

---

## 4. hot_reload_demo.cpp

### Description
Démontre le hot-reload de plugins avec sérialisation/désérialisation d'état.

### Concepts Démontrés
- FileWatcher pour surveillance de fichiers
- Hot-reload automatique de plugins
- Sérialisation d'état avant déchargement
- Désérialisation d'état après rechargement
- Rollback en cas d'échec
- Dépendances préservées lors du reload

### Cas d'Usage
- Développement de plugins sans redémarrage
- Mise à jour en production sans downtime
- Game engines avec hot-reload de game logic
- Applications longue durée

### Exécution

```bash
./bin/hot_reload_demo

# Dans un autre terminal, modifiez un plugin:
cd plugins/hot_reload_example
touch HotReloadExamplePlugin.cpp  # Force recompilation
cd ../../build
make

# Output attendu dans hot_reload_demo:
# [FileWatcher] Detected change: plugins/hot_reload_example.so
# [PluginManager] Reloading plugin: HotReloadExample
# [Plugin] Serializing state...
# [Plugin] Deserializing state...
# Plugin reloaded successfully!
```

### Code Clé

```cpp
// Activer hot-reload
m_pluginManager.enableHotReload(std::chrono::milliseconds(1000));

// Dans le plugin
std::string serializeState() override {
    return /* état sérialisé */;
}

void deserializeState(const std::string& state) override {
    // Restaurer état
}
```

---

## 5. profiling_example.cpp

### Description
Analyse de performance avec ProfilingModule et collecte de métriques.

### Concepts Démontrés
- ProfilingModule pour métriques
- Macros `MCF_PROFILE_SCOPE` et `MCF_PROFILE_FRAME`
- Statistiques de performance (min, max, avg, count)
- Export de métriques
- Overhead minimal du profiling
- MetricsCollector API

### Cas d'Usage
- Optimisation de performance
- Identification de bottlenecks
- Monitoring en production
- Benchmarking
- Performance regression tests

### Exécution

```bash
./bin/profiling_example

# Output attendu:
# === Performance Metrics ===
#
# Frame:
#   Count: 1000
#   Total: 16.667s
#   Average: 16.667ms
#   Min: 15.234ms
#   Max: 18.456ms
#
# expensiveFunction:
#   Count: 1000
#   Total: 5.234s
#   Average: 5.234ms
#   Min: 4.123ms
#   Max: 7.890ms
```

### Code Clé

```cpp
void expensiveFunction() {
    MCF_PROFILE_SCOPE("expensiveFunction");
    // Code à profiler
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

void gameLoop() {
    MCF_PROFILE_FRAME();  // Track frame time
    expensiveFunction();
}

// Récupérer statistiques
auto& profiler = mcf::ProfilingModule::instance();
auto stats = profiler.getMetricsCollector().getStatistics();
```

---

## 6. filesystem_example.cpp

### Description
Démonstration des utilitaires filesystem cross-platform.

### Concepts Démontrés
- Manipulation de paths (absolute, parent, filename, extension)
- Vérification d'existence de fichiers/répertoires
- Opérations fichiers (copy, remove, size)
- Création de répertoires (récursive)
- Listing de fichiers avec filtres
- Cross-platform (std::filesystem + fallbacks)

### Cas d'Usage
- Applications manipulant des fichiers
- Outils de build/deployment
- Data processing pipelines
- Configuration file management
- Backup/restore systems

### Exécution

```bash
./bin/filesystem_example

# Output attendu:
# Current directory: /home/user/ModularCppFramework/build
# Absolute path: /home/user/data.txt
# Parent directory: /home/user
# Filename: data.txt
# Extension: .txt
# File exists: true
# File size: 1024 bytes
# Directory created: output/nested/path
# Files in directory:
#   - plugin1.so
#   - plugin2.so
```

### Code Clé

```cpp
// Path operations
auto absPath = mcf::FileSystem::getAbsolutePath("data.txt");
auto parent = mcf::FileSystem::getParentPath(absPath);
auto filename = mcf::FileSystem::getFileName(absPath);
auto ext = mcf::FileSystem::getFileExtension(filename);

// File operations
bool exists = mcf::FileSystem::fileExists("data.txt");
size_t size = mcf::FileSystem::getFileSize("data.txt");
mcf::FileSystem::copyFile("src.txt", "dst.txt");

// Directory operations
mcf::FileSystem::createDirectories("output/nested/path");
auto files = mcf::FileSystem::listFiles("plugins", ".so");
```

---

## 7. networking_server_example.cpp

### Description
Serveur TCP multi-clients avec callbacks pour événements de connexion.

### Concepts Démontrés
- NetworkingModule pour TCP
- TcpServer avec multi-clients
- Callbacks asynchrones (onClientConnected, onMessageReceived, onClientDisconnected)
- Message broadcasting
- ThreadPool pour I/O asynchrone
- Client ID tracking

### Cas d'Usage
- Game servers
- Chat servers
- Microservices communication
- IoT device management
- Distributed systems

### Exécution

```bash
./bin/networking_server_example

# Output attendu:
# [Server] Listening on port 8080
# [Server] Client 1 connected from 127.0.0.1:54321
# [Server] Received from client 1: Hello server!
# [Server] Client 1 disconnected

# Dans un autre terminal, connectez avec telnet:
telnet localhost 8080
# Tapez des messages, ils seront reçus par le serveur
```

### Code Clé

```cpp
m_server = std::make_shared<mcf::TcpServer>(8080);

m_server->setOnClientConnected([](int clientId, const std::string& addr) {
    std::cout << "Client " << clientId << " connected from " << addr << std::endl;
});

m_server->setOnMessageReceived([this](int clientId, const std::vector<uint8_t>& data) {
    std::string msg(data.begin(), data.end());
    std::cout << "Received: " << msg << std::endl;

    // Broadcast to all clients
    m_server->broadcast(data);
});

m_server->start();
```

---

## 8. networking_client_example.cpp

### Description
Client TCP avec reconnexion automatique et callbacks.

### Concepts Démontrés
- TcpClient avec reconnexion
- Callbacks asynchrones (onConnected, onMessageReceived, onDisconnected)
- Envoi de messages
- Gestion de déconnexion
- Heartbeat/keepalive

### Cas d'Usage
- Client pour game servers
- Microservices clients
- IoT devices
- Monitoring clients
- Chat clients

### Exécution

```bash
# Démarrez d'abord le serveur
./bin/networking_server_example

# Dans un autre terminal:
./bin/networking_client_example

# Output attendu:
# [Client] Connecting to 127.0.0.1:8080
# [Client] Connected!
# [Client] Sending: Hello from client
# [Client] Received: Echo: Hello from client
```

### Code Clé

```cpp
m_client = std::make_shared<mcf::TcpClient>();

m_client->setOnConnected([]() {
    std::cout << "Connected to server!" << std::endl;
});

m_client->setOnMessageReceived([](const std::vector<uint8_t>& data) {
    std::string msg(data.begin(), data.end());
    std::cout << "Received: " << msg << std::endl;
});

m_client->connect("127.0.0.1", 8080);

// Envoyer message
std::string msg = "Hello server!";
m_client->send({msg.begin(), msg.end()});
```

---

## Matrice de Concepts

| Exemple | EventBus | ServiceLocator | ResourceMgr | Plugins | Modules | Config | Logging | Networking | Profiling | Hot-Reload |
|---------|----------|----------------|-------------|---------|---------|--------|---------|------------|-----------|------------|
| logger_example | | | | | ✅ | ✅ | ✅ | | | |
| realtime_app_example | | | | | ✅ | | | | | |
| event_driven_app | ✅ | | | | | | | | | |
| hot_reload_demo | | | | ✅ | | | | | | ✅ |
| profiling_example | | | | | ✅ | | | | ✅ | |
| filesystem_example | | | | | | | | | | |
| networking_server | | | | | ✅ | | | ✅ | | |
| networking_client | | | | | ✅ | | | ✅ | | |

---

## Combinaisons Avancées

### Game Engine Complet

Combinaison de plusieurs exemples:
- RealtimeModule (60 FPS game loop)
- ProfilingModule (performance monitoring)
- LoggerModule (debugging)
- EventBus (game events)
- Hot-Reload (game logic plugins)

### Microservice

Combinaison pour architecture distribuée:
- NetworkingModule (TCP communication)
- EventBus (internal messaging)
- ServiceLocator (dependency injection)
- LoggerModule (structured logging)
- ConfigurationManager (service configuration)

### Data Pipeline

Combinaison pour traitement de données:
- Plugins (ingestion, transform, export stages)
- ThreadPool (parallel processing)
- EventBus (pipeline events)
- ProfilingModule (performance metrics)
- FileSystem (data I/O)

---

## Créer Votre Propre Exemple

### Template Minimal

```cpp
#include <core/Application.hpp>
#include <iostream>

class MyExample : public mcf::Application {
protected:
    bool onInitialize() override {
        std::cout << "Example initialized!" << std::endl;
        // Votre code ici
        return true;
    }

    void onShutdown() override {
        std::cout << "Example shutdown" << std::endl;
    }
};

int main() {
    MyExample app;
    return app.initialize() ? 0 : 1;
}
```

### Ajouter au Build

Dans `examples/CMakeLists.txt`:

```cmake
add_executable(my_example my_example.cpp)
target_link_libraries(my_example PRIVATE mcf_core)
```

---

## Ressources Additionnelles

- **Documentation complète**: [README.md](README.md)
- **Guide de démarrage**: [QUICK_START.md](QUICK_START.md)
- **Architecture**: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- **Guide plugins**: [docs/PLUGIN_GUIDE.md](docs/PLUGIN_GUIDE.md)
- **API Reference**: Générer avec `doxygen Doxyfile`

## Support

- **Issues**: [GitHub Issues](https://github.com/Sylvain-RZ/ModularCppFramework/issues)
- **Discussions**: [GitHub Discussions](https://github.com/Sylvain-RZ/ModularCppFramework/discussions)

---

**Note**: Tous les exemples sont testés et fonctionnels. Si vous rencontrez des problèmes, vérifiez que vous avez compilé avec `-DBUILD_EXAMPLES=ON`.

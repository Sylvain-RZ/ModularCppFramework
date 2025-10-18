# Guide d'Utilisation - ModularCppFramework

Ce guide explique comment utiliser les composants principaux de ModularCppFramework dans vos applications.

## Table des Matières

- [Créer une Application](#créer-une-application)
- [EventBus - Communication par Événements](#eventbus---communication-par-événements)
- [ServiceLocator - Injection de Dépendances](#servicelocator---injection-de-dépendances)
- [ResourceManager - Gestion de Ressources](#resourcemanager---gestion-de-ressources)
- [ConfigurationManager - Configuration JSON](#configurationmanager---configuration-json)
- [Logger - Système de Logging](#logger---système-de-logging)
- [ThreadPool - Exécution Asynchrone](#threadpool---exécution-asynchrone)
- [FileSystem - Utilitaires Fichiers](#filesystem---utilitaires-fichiers)
- [Modules Intégrés](#modules-intégrés)

---

## Créer une Application

### Application Simple (Event-Driven)

Pour une application event-driven (sans boucle de rendu):

```cpp
#include <core/Application.hpp>

class MyApp : public mcf::Application {
protected:
    bool onInitialize() override {
        // Initialisation custom
        m_logger->info("Application initialized");

        // Charger configuration
        m_configManager.loadFromFile("config.json");

        // Charger plugins
        m_pluginManager.loadPluginsFromDirectory("./plugins");
        m_pluginManager.initializeAll();

        return true;
    }

    void onShutdown() override {
        m_logger->info("Application shutting down");
    }
};

int main() {
    mcf::ApplicationConfig config;
    config.name = "MyApp";
    config.pluginDirectory = "./plugins";

    MyApp app(config);
    return app.initialize() ? 0 : 1;
}
```

### Application Temps Réel (avec boucle de rendu)

Pour une application avec boucle de rendu (jeux, simulations), utilisez **RealtimeModule**:

```cpp
#include <core/Application.hpp>
#include <modules/realtime/RealtimeModule.hpp>

class MyRealtimeApp : public mcf::Application, public mcf::IRealtimeUpdatable {
protected:
    bool onInitialize() override {
        // Ajouter le module realtime
        auto realtimeModule = std::make_shared<mcf::RealtimeModule>();
        realtimeModule->setTargetFPS(60);           // 60 FPS
        realtimeModule->setFixedTimestep(1.0f/60.0f); // Fixed 60Hz updates
        addModule(realtimeModule);

        // Charger plugins
        m_pluginManager.loadPluginsFromDirectory("./plugins");
        m_pluginManager.initializeAll();

        return true;
    }

    // Appelé à 60Hz avec timestep fixe
    void onUpdate(float deltaTime) override {
        // Logique de simulation/physique
        // deltaTime est toujours 1/60 = 0.01666s
    }
};

int main() {
    MyRealtimeApp app;
    app.run();  // Lance la boucle temps réel
    return 0;
}
```

---

## EventBus - Communication par Événements

L'EventBus permet la communication découplée entre composants via publish/subscribe.

### Publier un Événement

```cpp
// Créer un événement avec données
mcf::Event event("player.scored");
event.data = 100;  // Score

// Publier l'événement
eventBus->publish("player.scored", event);
```

### S'Abonner à un Événement

```cpp
// S'abonner avec callback
auto handle = eventBus->subscribe("player.scored",
    [](const mcf::Event& e) {
        int score = std::any_cast<int>(e.data);
        std::cout << "Score: " << score << std::endl;
    },
    100  // Priorité (plus haute = exécutée en premier)
);

// Se désabonner plus tard
eventBus->unsubscribe(handle);
```

### Utilisation dans un Plugin

```cpp
class MyPlugin : public mcf::IPlugin {
    mcf::EventHandle m_handle;

    bool initialize(mcf::PluginContext& context) override {
        auto* eventBus = context.getEventBus();

        // S'abonner à un événement
        m_handle = eventBus->subscribe("app.start",
            [this](const mcf::Event& e) {
                // Réagir au démarrage de l'application
            }
        );

        return true;
    }

    void shutdown() override {
        // Désabonnement automatique lors du shutdown
    }
};
```

### Événements avec Données Complexes

```cpp
struct PlayerData {
    std::string name;
    int health;
    float position[3];
};

// Publier
PlayerData player{"Hero", 100, {1.0f, 2.0f, 3.0f}};
mcf::Event event("player.updated");
event.data = player;
eventBus->publish("player.updated", event);

// S'abonner
eventBus->subscribe("player.updated", [](const mcf::Event& e) {
    auto player = std::any_cast<PlayerData>(e.data);
    std::cout << player.name << " health: " << player.health << std::endl;
});
```

---

## ServiceLocator - Injection de Dépendances

Le ServiceLocator gère l'injection de dépendances avec trois lifetimes: Singleton, Transient, et Scoped.

### Services Singleton

Même instance partagée par tous:

```cpp
// Enregistrer un service singleton
auto myService = std::make_shared<MyService>();
serviceLocator->registerSingleton<IMyService>(myService);

// Résoudre - retourne toujours la même instance
auto service1 = serviceLocator->resolve<IMyService>();
auto service2 = serviceLocator->resolve<IMyService>();
// service1 == service2
```

### Services Transient

Nouvelle instance à chaque résolution:

```cpp
// Enregistrer avec factory (transient)
serviceLocator->registerFactory<IMyService>(
    []() { return std::make_shared<MyService>(); },
    mcf::ServiceLifetime::Transient
);

// Résoudre - nouvelle instance à chaque fois
auto service1 = serviceLocator->resolve<IMyService>();
auto service2 = serviceLocator->resolve<IMyService>();
// service1 != service2
```

### Services Scoped (Nouveau dans v1.0)

Même instance dans un scope, différente entre scopes:

```cpp
// Enregistrer service scopé
serviceLocator->registerFactory<IRequestHandler>(
    []() { return std::make_shared<RequestHandler>(); },
    mcf::ServiceLifetime::Scoped
);

// Utilisation avec RAII (recommandé)
{
    mcf::ServiceScope scope(*serviceLocator);  // Entre dans un nouveau scope

    auto handler1 = serviceLocator->resolve<IRequestHandler>();
    auto handler2 = serviceLocator->resolve<IRequestHandler>();
    // handler1 == handler2 (même instance dans le scope)

} // Scope se termine, instances scopées nettoyées automatiquement

// Nouveau scope = nouvelles instances
{
    mcf::ServiceScope scope(*serviceLocator);
    auto handler3 = serviceLocator->resolve<IRequestHandler>();
    // handler3 != handler1 (nouveau scope)
}
```

### Scopes Imbriqués

```cpp
{
    mcf::ServiceScope outerScope(*serviceLocator);
    auto handler1 = serviceLocator->resolve<IRequestHandler>();

    {
        mcf::ServiceScope innerScope(*serviceLocator);
        auto handler2 = serviceLocator->resolve<IRequestHandler>();
        // handler2 != handler1 (scope différent)
    }

    auto handler3 = serviceLocator->resolve<IRequestHandler>();
    // handler3 == handler1 (même scope)
}
```

### Utilisation dans un Plugin

```cpp
bool initialize(mcf::PluginContext& context) override {
    auto* serviceLocator = context.getServiceLocator();

    // Résoudre un service
    auto logger = serviceLocator->resolve<ILogger>();

    // Enregistrer un service fourni par le plugin
    serviceLocator->registerSingleton<IMyInterface>(
        std::make_shared<MyImplementation>()
    );

    return true;
}
```

---

## ResourceManager - Gestion de Ressources

Le ResourceManager gère le chargement et la mise en cache automatique de ressources.

### Enregistrer un Loader

```cpp
// Enregistrer un loader pour les textures
resourceManager->registerLoader<Texture>(
    [](const std::string& path) {
        return std::make_shared<Texture>(path);
    }
);
```

### Charger des Ressources

```cpp
// Charger une ressource (automatiquement mise en cache)
auto texture1 = resourceManager->load<Texture>("textures/player.png");

// Charger à nouveau - retourne la même instance (cache)
auto texture2 = resourceManager->load<Texture>("textures/player.png");
// texture1 == texture2

// Le cache utilise des weak_ptr, donc les ressources non utilisées
// sont automatiquement libérées
```

### Loader avec Paramètres

```cpp
// Loader avec options supplémentaires
resourceManager->registerLoader<Mesh>(
    [](const std::string& path) {
        MeshLoadOptions options;
        options.generateNormals = true;
        options.generateTangents = true;
        return std::make_shared<Mesh>(path, options);
    }
);
```

---

## ConfigurationManager - Configuration JSON

Le système de configuration JSON avec hot-reload et dot notation.

### Charger une Configuration

```cpp
auto& config = app.getConfigurationManager();
config.loadFromFile("config.json");
```

Exemple `config.json`:

```json
{
    "app": {
        "name": "MyApp",
        "version": "1.0.0"
    },
    "server": {
        "port": 8080,
        "host": "127.0.0.1"
    },
    "debug": {
        "enabled": true,
        "logLevel": "info"
    }
}
```

### Accès avec Dot Notation

```cpp
// Lire des valeurs avec type-safety et valeurs par défaut
std::string appName = config.getString("app.name", "DefaultApp");
int port = config.getInt("server.port", 8080);
bool debug = config.getBool("debug.enabled", false);
float timeout = config.getFloat("network.timeout", 5.0f);
```

### Modifier et Sauvegarder

```cpp
// Modifier une valeur
config.set("app.version", mcf::JsonValue("2.0.0"));
config.set("server.port", mcf::JsonValue(9090));

// Sauvegarder les modifications
config.saveToFile("config.json");
```

### Watcher pour Hot-Reload

```cpp
// Réagir aux changements de configuration
config.watchKey("server.port", [](const std::string& key, const mcf::JsonValue& value) {
    std::cout << "Port changed to: " << value.asInt() << std::endl;
    // Appliquer le changement...
});

// Le fichier est surveillé automatiquement et rechargé en cas de modification
```

### Utilisation dans un Plugin

```cpp
bool initialize(mcf::PluginContext& context) override {
    auto* config = context.getConfigurationManager();
    if (config) {
        // Lire la configuration spécifique au plugin
        m_timeout = config->getInt("myplugin.timeout", 5000);
        m_enabled = config->getBool("myplugin.enabled", true);
    }
    return true;
}
```

---

## Logger - Système de Logging

Système de logging flexible avec niveaux et sinks multiples.

### Créer un Logger

```cpp
// Créer un logger
auto logger = mcf::Logger::create("MyLogger");

// Configurer les sinks
logger->addConsoleSink(mcf::LogLevel::Debug, true);  // Coloré
logger->addFileSink(mcf::LogLevel::Info, "logs/app.log");
logger->addRotatingFileSink(
    mcf::LogLevel::Warn,
    "logs/rotating.log",
    1024*1024*10,  // 10MB
    5              // 5 fichiers
);
```

### Logger des Messages

```cpp
logger->trace("Trace message");
logger->debug("Debug message");
logger->info("Info message");
logger->warn("Warning message");
logger->error("Error message");
logger->critical("Critical message");
```

### Logging Formaté

```cpp
logger->info("User {} logged in from {}", username, ipAddress);
logger->warn("Connection timeout after {} seconds", timeout);
logger->error("Failed to load file: {}", filepath);
```

### Niveaux de Log

```cpp
// Définir le niveau minimum
logger->setLogLevel(mcf::LogLevel::Info);

// Maintenant seuls info, warn, error, critical sont loggés
logger->debug("Not logged");  // Ignoré
logger->info("Logged");       // Affiché
```

---

## ThreadPool - Exécution Asynchrone

Le ThreadPool permet d'exécuter des tâches en arrière-plan.

### Accès au ThreadPool (Nouveau dans v1.0)

```cpp
// Depuis Application
auto& pool = app.getThreadPool();

// Ou créer un pool standalone
mcf::ThreadPool pool(4);  // 4 threads workers
```

### Soumettre une Tâche avec Retour

```cpp
// Soumettre une tâche qui retourne une valeur
auto future = pool.submit([]() {
    // Travail asynchrone
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 42;
});

// Continuer à travailler...

// Attendre le résultat
int result = future.get();
std::cout << "Result: " << result << std::endl;
```

### Soumettre une Tâche Void (Fire-and-Forget)

```cpp
// Tâche sans retour
pool.submit([]() {
    // Travail en arrière-plan
    std::cout << "Background task" << std::endl;
});
```

### Utilisation dans un Plugin

```cpp
bool initialize(mcf::PluginContext& context) override {
    auto* app = context.getApplication();
    if (app) {
        auto& threadPool = app->getThreadPool();

        // Initialisation asynchrone
        threadPool.submit([this]() {
            loadHeavyResources();
        });
    }
    return true;
}
```

### Tâches Multiples

```cpp
std::vector<std::future<int>> futures;

// Lancer plusieurs tâches
for (int i = 0; i < 10; ++i) {
    futures.push_back(pool.submit([i]() {
        return processData(i);
    }));
}

// Attendre tous les résultats
for (auto& future : futures) {
    int result = future.get();
    std::cout << "Result: " << result << std::endl;
}
```

---

## FileSystem - Utilitaires Fichiers

Utilitaires cross-platform pour manipuler fichiers et répertoires.

### Opérations sur Paths

```cpp
// Obtenir le chemin absolu
auto absPath = mcf::FileSystem::getAbsolutePath("relative/path");

// Obtenir le répertoire parent
auto parent = mcf::FileSystem::getParentPath("/path/to/file.txt");
// Retourne: "/path/to"

// Obtenir le nom de fichier
auto filename = mcf::FileSystem::getFileName("/path/to/file.txt");
// Retourne: "file.txt"

// Obtenir l'extension
auto ext = mcf::FileSystem::getFileExtension("file.txt");
// Retourne: ".txt"
```

### Requêtes Fichiers

```cpp
// Vérifier si un fichier existe
bool exists = mcf::FileSystem::fileExists("config.json");

// Vérifier si c'est un répertoire
bool isDir = mcf::FileSystem::isDirectory("plugins");

// Obtenir la taille d'un fichier
size_t size = mcf::FileSystem::getFileSize("data.bin");
```

### Opérations Fichiers

```cpp
// Copier un fichier
mcf::FileSystem::copyFile("src.txt", "dst.txt");

// Supprimer un fichier
mcf::FileSystem::removeFile("temp.dat");

// Créer un répertoire
mcf::FileSystem::createDirectory("output");

// Créer des répertoires imbriqués
mcf::FileSystem::createDirectories("output/nested/path");
```

### Lister des Fichiers

```cpp
// Lister tous les fichiers .so dans plugins/
auto files = mcf::FileSystem::listFiles("plugins", ".so");

for (const auto& file : files) {
    std::cout << "Plugin: " << file << std::endl;
}

// Lister tous les fichiers
auto allFiles = mcf::FileSystem::listFiles("data");
```

---

## Modules Intégrés

### LoggerModule

Configuration automatique du logger via JSON.

```cpp
#include <modules/logger/LoggerModule.hpp>

class MyApp : public mcf::Application {
protected:
    bool onInitialize() override {
        // Ajouter LoggerModule
        addModule<mcf::LoggerModule>();

        // Utiliser les loggers configurés
        auto logger = mcf::LoggerRegistry::instance().getLogger("MyApp");
        logger->info("Application started");
        logger->warn("Warning message");
        logger->error("Error message");

        return true;
    }
};
```

Configuration JSON (`logger_config.json`):

```json
{
    "loggers": {
        "MyApp": {
            "level": "debug",
            "sinks": [
                {"type": "console", "level": "info", "colored": true},
                {"type": "file", "level": "debug", "path": "logs/app.log"}
            ]
        }
    }
}
```

### RealtimeModule (Fixed Timestep)

Boucle temps réel avec timestep fixe pour simulations.

```cpp
#include <modules/realtime/RealtimeModule.hpp>

class MyApp : public mcf::Application, public mcf::IRealtimeUpdatable {
protected:
    bool onInitialize() override {
        auto realtimeModule = std::make_shared<mcf::RealtimeModule>();
        realtimeModule->setTargetFPS(60);           // 60 FPS cible
        realtimeModule->setFixedTimestep(1.0f/60.0f); // Physique à 60Hz
        addModule(realtimeModule);
        return true;
    }

    void onUpdate(float deltaTime) override {
        // Appelé avec deltaTime fixe (1/60 = 0.01666s)
        // Idéal pour physique/simulation
        updatePhysics(deltaTime);
    }
};
```

### ProfilingModule

Collecte de métriques de performance.

```cpp
#include <modules/profiling/ProfilingModule.hpp>
#include <modules/profiling/ProfilingMacros.hpp>

class MyApp : public mcf::Application {
protected:
    bool onInitialize() override {
        addModule<mcf::ProfilingModule>();
        return true;
    }

    void someFunction() {
        MCF_PROFILE_SCOPE("someFunction");  // Profile cette fonction

        // Code à profiler
        expensiveOperation();

        {
            MCF_PROFILE_SCOPE("innerBlock");
            anotherOperation();
        }
    }

    void gameLoop() {
        MCF_PROFILE_FRAME();  // Track frame time
        // ...
    }
};

// Récupérer les métriques
auto& profiler = mcf::ProfilingModule::instance();
auto stats = profiler.getMetricsCollector().getStatistics();
for (const auto& [name, stat] : stats) {
    std::cout << name << ": avg=" << stat.average
              << " min=" << stat.min << " max=" << stat.max << std::endl;
}
```

### NetworkingModule (TCP Client/Server)

Client et serveur TCP asynchrones.

**Serveur TCP:**

```cpp
#include <modules/networking/NetworkingModule.hpp>

class MyServer : public mcf::Application {
protected:
    std::shared_ptr<mcf::TcpServer> m_server;

    bool onInitialize() override {
        addModule<mcf::NetworkingModule>();

        m_server = std::make_shared<mcf::TcpServer>(8080);

        m_server->setOnClientConnected([](int clientId, const std::string& addr) {
            std::cout << "Client " << clientId << " connected from " << addr << std::endl;
        });

        m_server->setOnMessageReceived([](int clientId, const std::vector<uint8_t>& data) {
            std::string msg(data.begin(), data.end());
            std::cout << "Received from " << clientId << ": " << msg << std::endl;
        });

        m_server->start();
        return true;
    }
};
```

**Client TCP:**

```cpp
class MyClient : public mcf::Application {
protected:
    std::shared_ptr<mcf::TcpClient> m_client;

    bool onInitialize() override {
        addModule<mcf::NetworkingModule>();

        m_client = std::make_shared<mcf::TcpClient>();

        m_client->setOnConnected([]() {
            std::cout << "Connected to server" << std::endl;
        });

        m_client->setOnMessageReceived([](const std::vector<uint8_t>& data) {
            std::string msg(data.begin(), data.end());
            std::cout << "Received: " << msg << std::endl;
        });

        m_client->connect("127.0.0.1", 8080);
        return true;
    }
};
```

---

## Prochaines Étapes

- **[Guide Plugins](PLUGIN_GUIDE.md)** - Créer vos propres plugins
- **[Hot-Reload](HOT_RELOAD.md)** - Utiliser le hot-reload
- **[Configuration](CONFIGURATION_GUIDE.md)** - Système de configuration avancé
- **[Exemples](EXAMPLES.md)** - Voir des exemples complets

---

**Retour:** [Documentation SDK](README.md) | **Suivant:** [Guide Plugins](PLUGIN_GUIDE.md)

# Guide de Création de Plugin

Ce guide explique comment créer un plugin pour le framework ModularCppFramework.

## Étape 1: Structure du Plugin

Créez un nouveau dossier dans `plugins/`:

```bash
mkdir -p plugins/my_plugin
cd plugins/my_plugin
```

## Étape 2: Fichier Header (Optionnel)

Si votre plugin est complexe, créez `MyPlugin.hpp`:

```cpp
#pragma once

#include "../../core/IPlugin.hpp"
#include "../../core/PluginMetadata.hpp"
#include "../../core/PluginContext.hpp"

namespace mvk {

class MyPlugin : public IPlugin {
private:
    bool m_initialized;
    PluginMetadata m_metadata;
    PluginContext m_context;

    // Vos données privées
    float m_someState;

public:
    MyPlugin();
    ~MyPlugin() override;

    // Interface IPlugin
    std::string getName() const override;
    std::string getVersion() const override;
    const PluginMetadata& getMetadata() const override;
    bool initialize(PluginContext& context) override;
    void shutdown() override;
    void onUpdate(float deltaTime) override;
    bool isInitialized() const override;

    // Manifest pour chargement dynamique
    static const char* getManifestJson();

    // Vos méthodes publiques
    void myCustomMethod();
};

} // namespace mvk
```

## Étape 3: Implémentation

Créez `MyPlugin.cpp`:

```cpp
#include "MyPlugin.hpp"
#include "../../core/EventBus.hpp"
#include "../../core/ServiceLocator.hpp"

#include <iostream>

namespace mvk {

MyPlugin::MyPlugin()
    : m_initialized(false)
    , m_someState(0.0f) {

    // Configuration des métadonnées
    m_metadata.name = "MyPlugin";
    m_metadata.version = "1.0.0";
    m_metadata.author = "Your Name";
    m_metadata.description = "Description de votre plugin";
    m_metadata.loadPriority = 100;

    // Ajouter des dépendances (optionnel)
    // m_metadata.addDependency("CorePlugin", "1.0.0", "2.0.0", true);
}

MyPlugin::~MyPlugin() {
    if (m_initialized) {
        shutdown();
    }
}

std::string MyPlugin::getName() const {
    return m_metadata.name;
}

std::string MyPlugin::getVersion() const {
    return m_metadata.version;
}

const PluginMetadata& MyPlugin::getMetadata() const {
    return m_metadata;
}

bool MyPlugin::initialize(PluginContext& context) {
    if (m_initialized) {
        return true;
    }

    m_context = context;

    std::cout << "[" << getName() << "] Initializing..." << std::endl;

    // S'abonner à des événements
    if (m_context.getEventBus()) {
        m_context.getEventBus()->subscribe("my.event",
            [this](const Event& event) {
                std::cout << "[" << getName() << "] Received my.event" << std::endl;
                // Traiter l'événement
            },
            100  // Priorité
        );
    }

    // Résoudre des services (optionnel)
    /*
    if (m_context.getServiceLocator()) {
        try {
            auto service = m_context.getServiceLocator()->resolve<IMyService>();
            // Utiliser le service
        } catch (const std::exception& e) {
            std::cerr << "[" << getName() << "] Failed to resolve service: "
                     << e.what() << std::endl;
            return false;
        }
    }
    */

    // Publier un événement d'initialisation
    if (m_context.getEventBus()) {
        Event initEvent("plugin.initialized");
        initEvent.data = getName();
        m_context.getEventBus()->publish("plugin.initialized", initEvent);
    }

    m_initialized = true;
    std::cout << "[" << getName() << "] Initialization complete!" << std::endl;

    return true;
}

void MyPlugin::shutdown() {
    if (!m_initialized) {
        return;
    }

    std::cout << "[" << getName() << "] Shutting down..." << std::endl;

    // Publier événement de shutdown
    if (m_context.getEventBus()) {
        Event shutdownEvent("plugin.shutdown");
        shutdownEvent.data = getName();
        m_context.getEventBus()->publish("plugin.shutdown", shutdownEvent);
    }

    // Nettoyer vos ressources
    m_someState = 0.0f;

    m_initialized = false;
    std::cout << "[" << getName() << "] Shutdown complete!" << std::endl;
}

void MyPlugin::onUpdate(float deltaTime) {
    // Logique de mise à jour
    m_someState += deltaTime;

    // Exemple: publier un événement toutes les 5 secondes
    static float timer = 0.0f;
    timer += deltaTime;

    if (timer >= 5.0f) {
        if (m_context.getEventBus()) {
            Event updateEvent("plugin.update");
            updateEvent.data = m_someState;
            m_context.getEventBus()->publish("plugin.update", updateEvent);
        }
        timer = 0.0f;
    }
}

bool MyPlugin::isInitialized() const {
    return m_initialized;
}

const char* MyPlugin::getManifestJson() {
    return R"({
        "name": "MyPlugin",
        "version": "1.0.0",
        "author": "Your Name",
        "description": "Description de votre plugin",
        "dependencies": [],
        "load_priority": 100
    })";
}

void MyPlugin::myCustomMethod() {
    std::cout << "[" << getName() << "] Custom method called!" << std::endl;
}

} // namespace mvk

// IMPORTANT: Exporter les symboles du plugin
MCF_PLUGIN_EXPORT(mcf::MyPlugin)
```

## Étape 4: CMakeLists.txt

Créez `CMakeLists.txt` pour votre plugin:

```cmake
cmake_minimum_required(VERSION 3.16)

# Nom du plugin (sans le préfixe "lib" sur Unix)
add_library(my_plugin SHARED
    MyPlugin.cpp
    # Ajoutez d'autres fichiers .cpp ici
)

# Lier avec mcf_core
target_link_libraries(my_plugin PRIVATE
    mcf_core
)

# Configuration de sortie
set_target_properties(my_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_DIRECTORY}
    RUNTIME_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_DIRECTORY}
    PREFIX ""  # Pas de préfixe "lib"
)

# Visibilité des symboles (important!)
set_target_properties(my_plugin PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# Installation
install(TARGETS my_plugin
        LIBRARY DESTINATION plugins
        RUNTIME DESTINATION plugins)
```

## Étape 5: Enregistrer le Plugin

Dans `plugins/CMakeLists.txt`, ajoutez:

```cmake
add_subdirectory(my_plugin)
```

## Étape 6: Build

```bash
cd build
cmake ..
make -j$(nproc)
```

Le plugin sera compilé dans `build/plugins/my_plugin.so` (Linux) ou `my_plugin.dll` (Windows).

## Étape 7: Tester

Votre plugin sera automatiquement chargé si vous utilisez:

```cpp
ApplicationConfig config;
config.pluginDirectory = "./plugins";
config.autoLoadPlugins = true;
config.autoInitPlugins = true;

MyApp app(config);
app.run();
```

## Fonctionnalités Avancées

### 1. Utiliser les Services

```cpp
// Dans initialize()
auto resourceManager = m_context.getServiceLocator()->resolve<ResourceManager>();

// Enregistrer votre propre service
auto myService = std::make_shared<MyService>();
m_context.getServiceLocator()->registerSingleton<IMyService>(myService);
```

### 2. Événements Personnalisés

```cpp
// Définir votre type d'événement
struct MyEventData {
    int value;
    std::string message;
};

// Publier
MyEventData data{42, "Hello"};
Event event("my.custom.event", data);
m_context.getEventBus()->publish("my.custom.event", event);

// S'abonner
m_context.getEventBus()->subscribe("my.custom.event",
    [](const Event& e) {
        auto data = std::any_cast<MyEventData>(e.data);
        std::cout << "Value: " << data.value << ", Message: " << data.message << std::endl;
    }
);
```

### 3. Charger des Ressources

```cpp
// Enregistrer un loader (dans initialize)
auto resourceManager = m_context.getServiceLocator()->resolve<ResourceManager>();

resourceManager->registerLoader<MyResource>(
    [](const std::string& path) {
        return std::make_shared<MyResource>(path);
    }
);

// Charger une ressource
auto resource = resourceManager->load<MyResource>("assets/myfile.dat");
```

### 4. Dépendances entre Plugins

```cpp
// Dans le constructeur
m_metadata.addDependency("CorePlugin", "1.0.0", "2.0.0", true);
m_metadata.addDependency("UtilsPlugin", "1.0.0", "", false);  // Optionnel

// Accéder à un autre plugin
auto otherPlugin = m_context.getApplication()
    ->getPluginManager()
    .getPlugin<OtherPlugin>("OtherPlugin");

if (otherPlugin) {
    otherPlugin->someMethod();
}
```

### 5. Configuration du Plugin

```cpp
// Charger une config JSON (nécessite une bibliothèque JSON)
std::ifstream file("plugins/my_plugin/config.json");
// Parse JSON et configurer le plugin
```

## Bonnes Pratiques

### 1. Nommage
- Utilisez un préfixe pour éviter les conflits de noms
- Nom du plugin = Nom de la classe (sans "Plugin" optionnel)

### 2. Gestion d'Erreurs
```cpp
bool MyPlugin::initialize(PluginContext& context) {
    try {
        // Code d'initialisation
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[" << getName() << "] Error: " << e.what() << std::endl;
        return false;
    }
}
```

### 3. Thread-Safety
Si votre plugin utilise des threads:
```cpp
class MyPlugin : public IPlugin {
private:
    std::mutex m_mutex;

public:
    void onUpdate(float deltaTime) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        // Code thread-safe
    }
};
```

### 4. Performance
- Évitez les allocations dans onUpdate()
- Utilisez des pools d'objets si nécessaire
- Profitez du cache de ResourceManager

### 5. Logging
```cpp
// Utilisez un préfixe cohérent
std::cout << "[" << getName() << "] Message" << std::endl;

// Ou récupérez un logger du ServiceLocator
auto logger = m_context.getServiceLocator()->tryResolve<ILogger>();
if (logger) {
    logger->info(getName() + ": Message");
}
```

## Template Rapide

Copiez ce template minimal:

```cpp
#include "../../core/IPlugin.hpp"
#include "../../core/PluginContext.hpp"
#include "../../core/PluginMetadata.hpp"

class MyPlugin : public mcf::IPlugin {
    bool m_initialized = false;
    mcf::PluginMetadata m_metadata;
    mcf::PluginContext m_context;

public:
    MyPlugin() {
        m_metadata.name = "MyPlugin";
        m_metadata.version = "1.0.0";
    }

    std::string getName() const override { return m_metadata.name; }
    std::string getVersion() const override { return m_metadata.version; }
    const mcf::PluginMetadata& getMetadata() const override { return m_metadata; }
    bool isInitialized() const override { return m_initialized; }

    bool initialize(mcf::PluginContext& context) override {
        m_context = context;
        m_initialized = true;
        return true;
    }

    void shutdown() override { m_initialized = false; }
    void onUpdate(float deltaTime) override { }

    static const char* getManifestJson() {
        return R"({"name": "MyPlugin", "version": "1.0.0"})";
    }
};

MCF_PLUGIN_EXPORT(MyPlugin)
```

## Debugging

### Plugin ne se charge pas
- Vérifiez que le .so/.dll est dans le bon dossier
- Vérifiez que MCF_PLUGIN_EXPORT est présent
- Utilisez `ldd` (Linux) ou Dependency Walker (Windows) pour vérifier les dépendances

### Symboles non trouvés
- Vérifiez la visibilité des symboles (CXX_VISIBILITY_PRESET)
- Vérifiez que vous utilisez `extern "C"` (fait par la macro)
- Utilisez `nm -D` (Linux) pour lister les symboles exportés

### Crash au chargement
- Vérifiez que vous liez mcf_core
- Vérifiez la compatibilité C++ (std=c++17)
- Vérifiez les versions des dépendances

## Exemple Complet

Voir [plugins/example_plugin/](../plugins/example_plugin/) pour un exemple complet et fonctionnel.

## Ressources

- [Architecture générale](IMPLEMENTATION.md)
- [README principal](../README.md)

Bon développement! 🚀

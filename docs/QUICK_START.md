# Guide de Démarrage Rapide - ModularCppFramework

## Installation (5 minutes)

### Prérequis

```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake g++ libdl-dev

# Vérifier versions
g++ --version      # Minimum: GCC 7+
cmake --version    # Minimum: 3.16+
```

### Build

```bash
git clone https://github.com/Sylvain-RZ/ModularCppFramework.git
cd ModularCppFramework
mkdir build && cd build
cmake -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON ..
make -j$(nproc)

# Vérifier que tout fonctionne
ctest -V
```

**Résultat attendu**: `100% tests passed, 0 tests failed out of 10`

---

## Créer Votre Première Application (10 minutes)

### 1. Application Minimale

Créez `my_app.cpp`:

```cpp
#include <core/Application.hpp>
#include <iostream>

class MyApp : public mcf::Application {
protected:
    bool onInitialize() override {
        std::cout << "Application initialized!" << std::endl;

        // Charger plugins depuis ./plugins
        m_pluginManager.loadPluginsFromDirectory("./plugins");
        m_pluginManager.initializeAll();

        return true;
    }

    void onShutdown() override {
        std::cout << "Application shutting down" << std::endl;
    }
};

int main() {
    MyApp app;
    return app.initialize() ? 0 : 1;
}
```

### 2. CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyApp)

set(CMAKE_CXX_STANDARD 17)

# Ajouter ModularCppFramework
add_subdirectory(ModularCppFramework)

add_executable(my_app my_app.cpp)
target_link_libraries(my_app PRIVATE mcf_core)
```

### 3. Compiler et Exécuter

```bash
mkdir build && cd build
cmake ..
make
./my_app
```

---

## Créer Votre Premier Plugin (15 minutes)

### 1. Structure du Plugin

```
my_plugin/
├── CMakeLists.txt
├── MyPlugin.hpp
└── MyPlugin.cpp
```

### 2. MyPlugin.hpp

```cpp
#pragma once
#include <core/IPlugin.hpp>

class MyPlugin : public mcf::IPlugin {
private:
    bool m_initialized = false;
    mcf::PluginMetadata m_metadata;
    mcf::PluginContext m_context;

public:
    MyPlugin();

    // IPlugin interface
    std::string getName() const override;
    std::string getVersion() const override;
    const mcf::PluginMetadata& getMetadata() const override;

    bool initialize(mcf::PluginContext& context) override;
    void shutdown() override;
    void onUpdate(float deltaTime) override;
    bool isInitialized() const override;

    static const char* getManifestJson();
};
```

### 3. MyPlugin.cpp

```cpp
#include "MyPlugin.hpp"
#include <iostream>

MyPlugin::MyPlugin() {
    m_metadata.name = "MyPlugin";
    m_metadata.version = "1.0.0";
    m_metadata.author = "Your Name";
    m_metadata.description = "My first plugin";
}

std::string MyPlugin::getName() const {
    return m_metadata.name;
}

std::string MyPlugin::getVersion() const {
    return m_metadata.version;
}

const mcf::PluginMetadata& MyPlugin::getMetadata() const {
    return m_metadata;
}

bool MyPlugin::initialize(mcf::PluginContext& context) {
    m_context = context;
    std::cout << "MyPlugin initialized!" << std::endl;

    // S'abonner à un événement
    context.getEventBus()->subscribe("app.start", [](const mcf::Event& e) {
        std::cout << "App started!" << std::endl;
    });

    m_initialized = true;
    return true;
}

void MyPlugin::shutdown() {
    std::cout << "MyPlugin shutdown" << std::endl;
    m_initialized = false;
}

void MyPlugin::onUpdate(float deltaTime) {
    // Appelé chaque frame si RealtimeModule est actif
}

bool MyPlugin::isInitialized() const {
    return m_initialized;
}

const char* MyPlugin::getManifestJson() {
    return R"({
        "name": "MyPlugin",
        "version": "1.0.0",
        "author": "Your Name",
        "description": "My first plugin"
    })";
}

// IMPORTANT: Export du plugin
MCF_PLUGIN_EXPORT(MyPlugin)
```

### 4. CMakeLists.txt du Plugin

```cmake
add_library(my_plugin SHARED
    MyPlugin.cpp
)

target_link_libraries(my_plugin PRIVATE mcf_core)

set_target_properties(my_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
    PREFIX ""  # Pas de préfixe "lib"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
```

### 5. Compiler

```bash
cd build
cmake ..
make
ls plugins/  # Doit contenir my_plugin.so
```

---

## Exemples Avancés

### Application avec Boucle Temps Réel

```cpp
#include <core/Application.hpp>
#include <modules/realtime/RealtimeModule.hpp>

class MyGame : public mcf::Application, public mcf::IRealtimeUpdatable {
protected:
    bool onInitialize() override {
        // Ajouter RealtimeModule
        auto realtime = std::make_shared<mcf::RealtimeModule>();
        realtime->setTargetFPS(60);
        realtime->setFixedTimestep(1.0f/60.0f);
        addModule(realtime);

        // Charger plugins
        m_pluginManager.loadPluginsFromDirectory("./plugins");
        m_pluginManager.initializeAll();

        return true;
    }

    void onUpdate(float deltaTime) override {
        // Appelé à 60Hz avec deltaTime fixe (0.01666s)
        // Physique, logique de jeu, etc.
    }
};

int main() {
    MyGame game;
    game.run();  // Lance la boucle temps réel
    return 0;
}
```

### Utiliser l'EventBus

```cpp
// Dans votre plugin
bool MyPlugin::initialize(mcf::PluginContext& context) {
    // S'abonner
    auto handle = context.getEventBus()->subscribe(
        "player.scored",
        [](const mcf::Event& e) {
            int score = std::any_cast<int>(e.data);
            std::cout << "Score: " << score << std::endl;
        },
        100  // Priorité
    );

    // Publier
    mcf::Event event("player.scored", 100);
    context.getEventBus()->publish("player.scored", event);

    return true;
}
```

### Utiliser le ServiceLocator

```cpp
// Interface du service
class IMyService {
public:
    virtual ~IMyService() = default;
    virtual void doSomething() = 0;
};

// Implémentation
class MyService : public IMyService {
public:
    void doSomething() override {
        std::cout << "Doing something!" << std::endl;
    }
};

// Dans votre application
bool onInitialize() override {
    // Enregistrer service
    auto service = std::make_shared<MyService>();
    m_serviceLocator.registerSingleton<IMyService>(service);

    // Dans un plugin
    auto svc = context.getServiceLocator()->resolve<IMyService>();
    svc->doSomething();

    return true;
}
```

### Hot-Reload

```cpp
bool onInitialize() override {
    // Charger plugins
    m_pluginManager.loadPluginsFromDirectory("./plugins");
    m_pluginManager.initializeAll();

    // Activer hot-reload (vérifie toutes les 1 seconde)
    m_pluginManager.enableHotReload(std::chrono::milliseconds(1000));

    return true;
}
```

Maintenant, modifiez votre plugin, recompilez, et il sera rechargé automatiquement!

---

## Prochaines Étapes

1. **Lire la documentation complète**: [README.md](README.md)
2. **Explorer les exemples**: `./bin/logger_example`, `./bin/realtime_app_example`, etc.
3. **Comprendre l'architecture**: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
4. **Guide plugins détaillé**: [docs/PLUGIN_GUIDE.md](docs/PLUGIN_GUIDE.md)
5. **Configuration JSON**: [docs/CONFIGURATION_GUIDE.md](docs/CONFIGURATION_GUIDE.md)
6. **Hot-reload avancé**: [docs/HOT_RELOAD.md](docs/HOT_RELOAD.md)

## Ressources

- **Documentation API (Doxygen)**: Générer avec `doxygen Doxyfile`
- **Tests**: Voir `tests/unit/` et `tests/integration/` pour exemples
- **CLAUDE.md**: Contexte architectural complet pour AI assistants

## Support

- **Issues**: [GitHub Issues](https://github.com/Sylvain-RZ/ModularCppFramework/issues)
- **Discussions**: [GitHub Discussions](https://github.com/Sylvain-RZ/ModularCppFramework/discussions)

---

**Temps total**: ~30 minutes pour être opérationnel! 🚀

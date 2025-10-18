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

**Résultat attendu**: `100% tests passed, 0 tests failed out of 25`

---

## Créer Votre Première Application (10 minutes)

**Deux scénarios possibles** :

### Scénario A : Développer DANS ModularCppFramework
Si vous avez cloné le repo MCF et voulez ajouter vos exemples/plugins directement dedans, suivez la structure existante du projet (voir `examples/` et `plugins/`).

### Scénario B : MCF comme Bibliothèque Externe (RECOMMANDÉ)
Si vous créez un **nouveau projet séparé** utilisant MCF comme dépendance :

---

### 1. Ajouter MCF à Votre Projet

**Créer votre projet** :
```bash
mkdir MonProjet && cd MonProjet
git init

# Ajouter MCF comme sous-module
git submodule add https://github.com/Sylvain-RZ/ModularCppFramework.git external/ModularCppFramework
git submodule update --init --recursive

# Structure recommandée
mkdir -p src plugins config
```

**Structure finale** :
```
MonProjet/
├── external/
│   └── ModularCppFramework/   # Sous-module Git
├── src/
│   └── main.cpp               # Votre application
├── plugins/                   # VOS plugins
├── config/
│   └── app_config.json
├── CMakeLists.txt
└── README.md
```

---

### 2. Application Minimale

Créez `src/main.cpp` :

```cpp
#include <core/Application.hpp>
#include <modules/logger/LoggerModule.hpp>
#include <iostream>

class MyApp : public mcf::Application {
protected:
    bool onInitialize() override {
        std::cout << "MyApp initialized!" << std::endl;

        // Charger configuration (optionnel)
        getConfigurationManager().loadFromFile("config/app_config.json");

        // Charger VOS plugins depuis ./plugins
        getPluginManager().loadPluginsFromDirectory("./plugins");
        getPluginManager().initializeAll();

        return true;
    }

    void onShutdown() override {
        std::cout << "MyApp shutting down" << std::endl;
    }
};

int main() {
    MyApp app;

    // Ajouter des modules statiques (optionnel)
    app.addModule<mcf::LoggerModule>();

    return app.run();
}
```

---

### 3. CMakeLists.txt de VOTRE Projet

Créez `CMakeLists.txt` à la racine de **MonProjet** (pas dans MCF !) :

```cmake
cmake_minimum_required(VERSION 3.16)
project(MonProjet)

set(CMAKE_CXX_STANDARD 17)

# Désactiver les tests/exemples de MCF (optionnel)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Ajouter ModularCppFramework
add_subdirectory(external/ModularCppFramework)

# Votre application
add_executable(mon_app src/main.cpp)
target_link_libraries(mon_app PRIVATE mcf_core)

# Optionnel : utiliser des modules
target_link_libraries(mon_app PRIVATE
    mcf_logger_module
    # mcf_networking_module
    # mcf_profiling_module
    # mcf_realtime_module
)

# Si vous créez vos propres plugins
# add_subdirectory(plugins/my_plugin)
```

**Important** : Ce CMakeLists.txt est pour **votre projet**, pas pour MCF. MCF a son propre CMakeLists.txt dans `external/ModularCppFramework/`.

---

### 4. Compiler et Exécuter

```bash
cd MonProjet  # Racine de VOTRE projet
mkdir build && cd build
cmake ..
make -j$(nproc)

# IMPORTANT : Lancer depuis build/ (pas build/bin/)
./bin/mon_app
```

**Résultat attendu** :
```
MyApp initialized!
MyApp shutting down
```

**Note importante sur les plugins** :
- Les plugins compilés (`.so`/`.dll`) sont dans `build/plugins/`
- Le code `loadPluginsFromDirectory("./plugins")` cherche **relatif au répertoire d'exécution**
- Si vous lancez depuis `build/` : ✅ cherche dans `build/plugins/`
- Si vous lancez depuis `build/bin/` : ❌ cherche dans `build/bin/plugins/` (n'existe pas)

**Solutions** :
1. **Recommandé** : Toujours lancer depuis `build/` : `./bin/mon_app`
2. Utiliser un chemin relatif : `loadPluginsFromDirectory("../plugins")`
3. Utiliser un chemin absolu basé sur l'exécutable (voir exemple avancé ci-dessous)

---

### Alternative : CMake FetchContent (Sans Sous-module)

Si vous préférez **ne pas utiliser de sous-modules Git**, utilisez `FetchContent` :

```cmake
cmake_minimum_required(VERSION 3.16)
project(MonProjet)

set(CMAKE_CXX_STANDARD 17)

# Télécharger MCF automatiquement
include(FetchContent)

FetchContent_Declare(
    ModularCppFramework
    GIT_REPOSITORY https://github.com/Sylvain-RZ/ModularCppFramework.git
    GIT_TAG v1.0.2  # Version fixée
)

set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(ModularCppFramework)

# Votre application
add_executable(mon_app src/main.cpp)
target_link_libraries(mon_app PRIVATE mcf_core)
```

**Avantages** : Pas de gestion de sous-modules, build reproductible, idéal pour CI/CD.

---

## Créer Votre Premier Plugin (15 minutes)

### 1. Structure du Plugin dans VOTRE Projet

```
MonProjet/
├── plugins/
│   └── my_plugin/           # VOS sources plugins
│       ├── CMakeLists.txt
│       ├── MyPlugin.hpp
│       └── MyPlugin.cpp
└── build/
    └── plugins/
        └── my_plugin.so     # Plugin compilé (détecté automatiquement)
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

### 4. CMakeLists.txt du Plugin (`plugins/my_plugin/CMakeLists.txt`)

```cmake
add_library(my_plugin SHARED
    MyPlugin.cpp
)

target_link_libraries(my_plugin PRIVATE mcf_core)

set_target_properties(my_plugin PROPERTIES
    # IMPORTANT : Compiler dans build/plugins/ pour détection automatique
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
    PREFIX ""  # Pas de préfixe "lib" sur Linux
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
```

**Explication** :
- `${CMAKE_BINARY_DIR}/plugins` → Compile dans `build/plugins/my_plugin.so`
- Le `PluginManager` cherche automatiquement dans `./plugins` relatif à l'exécution
- Si vous lancez depuis `build/` : trouve `build/plugins/my_plugin.so` ✅

### 5. Ajouter le Plugin au Build Principal

Modifiez le `CMakeLists.txt` **de votre projet** (racine) pour inclure le plugin :

```cmake
# ... (contenu existant)

# Si vous créez vos propres plugins
add_subdirectory(plugins/my_plugin)  # ← Décommenter cette ligne
```

### 6. Compiler et Tester

```bash
cd MonProjet/build
cmake ..
make -j$(nproc)

# Vérifier que le plugin est compilé
ls plugins/  # Doit contenir my_plugin.so (ou .dll sur Windows)

# Lancer l'application
./bin/mon_app
```

**Résultat attendu** :
```
MyApp initialized!
Loading plugin: MyPlugin
MyPlugin initialized!
MyApp shutting down
MyPlugin shutdown
```

Le plugin est **automatiquement détecté** car :
1. Il est compilé dans `build/plugins/my_plugin.so`
2. L'application cherche dans `./plugins` depuis `build/`
3. Le `PluginManager` charge tous les `.so` trouvés

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
    getPluginManager().loadPluginsFromDirectory("./plugins");
    getPluginManager().initializeAll();

    // Activer hot-reload (vérifie toutes les 1 seconde)
    getPluginManager().enableHotReload(std::chrono::milliseconds(1000));

    return true;
}
```

Maintenant, modifiez votre plugin, recompilez, et il sera rechargé automatiquement!

### Gestion Robuste des Chemins de Plugins

Pour éviter les problèmes de détection selon le répertoire d'exécution, voici une solution robuste :

```cpp
#include <core/Application.hpp>
#include <core/FileSystem.hpp>
#include <iostream>

class MyApp : public mcf::Application {
protected:
    bool onInitialize() override {
        std::cout << "MyApp initialized!" << std::endl;

        // Solution 1 : Chemin relatif depuis build/bin/ vers build/plugins/
        getPluginManager().loadPluginsFromDirectory("../plugins");

        // Solution 2 : Chemin basé sur l'exécutable
        // auto exePath = mcf::FileSystem::getParentPath(argv[0]);
        // auto pluginPath = exePath + "/../plugins";
        // getPluginManager().loadPluginsFromDirectory(pluginPath);

        // Solution 3 : Variable d'environnement
        // const char* pluginDir = std::getenv("MCF_PLUGIN_DIR");
        // getPluginManager().loadPluginsFromDirectory(
        //     pluginDir ? pluginDir : "./plugins"
        // );

        getPluginManager().initializeAll();
        return true;
    }

    void onShutdown() override {
        std::cout << "MyApp shutting down" << std::endl;
    }
};

int main(int argc, char** argv) {
    MyApp app;
    app.addModule<mcf::LoggerModule>();
    return app.run();
}
```

**Recommandation** : Utiliser `"../plugins"` si vous lancez toujours depuis `build/bin/`

---

## Récapitulatif : Structure Finale et Détection Automatique

Voici la structure complète avec les chemins importants :

```
MonProjet/
├── external/
│   └── ModularCppFramework/        # Sous-module Git (framework)
├── src/
│   └── main.cpp                    # VOTRE application
├── plugins/                        # SOURCES de vos plugins
│   └── my_plugin/
│       ├── CMakeLists.txt          # Compile vers build/plugins/
│       ├── MyPlugin.hpp
│       └── MyPlugin.cpp
├── config/
│   └── app_config.json             # Votre configuration
├── build/                          # Répertoire de compilation
│   ├── bin/
│   │   └── mon_app                 # Exécutable compilé
│   └── plugins/
│       └── my_plugin.so            # Plugin compilé (DÉTECTÉ ICI)
└── CMakeLists.txt                  # Build de votre projet
```

**Détection automatique - Comment ça marche ?**

1. **Compilation** : Le plugin `my_plugin` est compilé dans `build/plugins/my_plugin.so` grâce à :
   ```cmake
   LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
   ```

2. **Exécution** : Vous lancez l'app depuis `build/` :
   ```bash
   ./bin/mon_app
   ```

3. **Chargement** : Le code cherche dans `./plugins` **relatif au répertoire d'exécution** :
   ```cpp
   getPluginManager().loadPluginsFromDirectory("./plugins");
   // Depuis build/ → cherche dans build/plugins/ ✅
   ```

4. **Résultat** : Le `PluginManager` trouve automatiquement `build/plugins/my_plugin.so` et le charge !

**Tableau de décision** :

| Vous lancez depuis | Code utilise | Cherche dans | Résultat |
|-------------------|--------------|--------------|----------|
| `build/` | `"./plugins"` | `build/plugins/` | ✅ Trouvé |
| `build/bin/` | `"./plugins"` | `build/bin/plugins/` | ❌ Pas trouvé |
| `build/bin/` | `"../plugins"` | `build/plugins/` | ✅ Trouvé |
| N'importe où | Chemin absolu | Chemin absolu | ✅ Trouvé |

**Conclusion** : Oui, les plugins sont détectés automatiquement si :
- ✅ Vous compilez dans `${CMAKE_BINARY_DIR}/plugins`
- ✅ Vous lancez depuis `build/` avec `"./plugins"`
- ✅ Ou vous lancez depuis `build/bin/` avec `"../plugins"`

---

## Prochaines Étapes

1. **Lire la documentation complète**: [README.md](README.md)
2. **Explorer les exemples**: `./bin/logger_example`, `./bin/realtime_app_example`, etc.
3. **Comprendre l'architecture**: [ARCHITECTURE.md](ARCHITECTURE.md)
4. **Guide plugins détaillé**: [PLUGIN_GUIDE.md](PLUGIN_GUIDE.md)
5. **Configuration JSON**: [CONFIGURATION_GUIDE.md](CONFIGURATION_GUIDE.md)
6. **Hot-reload avancé**: [HOT_RELOAD.md](HOT_RELOAD.md)

## Ressources

- **Documentation API (Doxygen)**: Générer avec `doxygen Doxyfile`
- **Tests**: Voir `tests/unit/` et `tests/integration/` pour exemples
- **CLAUDE.md**: Contexte architectural complet pour AI assistants

## Support

- **Issues**: [GitHub Issues](https://github.com/Sylvain-RZ/ModularCppFramework/issues)
- **Discussions**: [GitHub Discussions](https://github.com/Sylvain-RZ/ModularCppFramework/discussions)

---

**Temps total**: ~30 minutes pour être opérationnel! 🚀

# Guide d'Installation - ModularCppFramework

Ce guide couvre toutes les méthodes d'installation de ModularCppFramework pour différents cas d'usage.

## Table des Matières

- [Installation via Gestionnaires de Paquets](#installation-via-gestionnaires-de-paquets)
  - [Option 1: Conan (Recommandé)](#option-1-conan-recommandé)
  - [Option 2: vcpkg](#option-2-vcpkg)
- [Compilation depuis les Sources](#compilation-depuis-les-sources)
- [Intégration dans Votre Projet](#intégration-dans-votre-projet)
  - [Sous-module Git](#sous-module-git-recommandé-pour-débuter)
  - [CMake FetchContent](#cmake-fetchcontent-recommandé-pour-cicd)
  - [Installation Système](#installation-système-pour-utilisateurs-avancés)

---

## Installation via Gestionnaires de Paquets

### Option 1: Conan (Recommandé)

#### Installation Simple

```bash
# Ajouter le remote (si nécessaire)
conan remote add modular-cpp-framework https://your-repo-url

# Installer le package avec tous les modules
conan install modular-cpp-framework/1.0.0@
```

#### Utilisation dans conanfile.txt

```ini
[requires]
modular-cpp-framework/1.0.0

[generators]
cmake_find_package
```

#### Utilisation dans CMakeLists.txt

```cmake
find_package(modular-cpp-framework REQUIRED)
target_link_libraries(your_app PRIVATE mcf::core)
```

#### Options Conan Disponibles

```bash
# Installer seulement le core (sans modules)
conan install modular-cpp-framework/1.0.0@ \
    -o with_logger_module=False \
    -o with_networking_module=False \
    -o with_profiling_module=False \
    -o with_realtime_module=False

# Installer avec exemples et tests (pour développement)
conan install modular-cpp-framework/1.0.0@ \
    -o build_examples=True \
    -o build_tests=True

# Installer avec modules spécifiques
conan install modular-cpp-framework/1.0.0@ \
    -o with_logger_module=True \
    -o with_networking_module=True
```

**Options disponibles:**
- `with_logger_module` (défaut: True) - Module de logging avec configuration JSON
- `with_networking_module` (défaut: True) - Module TCP client/server
- `with_profiling_module` (défaut: True) - Module de profiling
- `with_realtime_module` (défaut: True) - Module temps réel avec fixed timestep
- `build_examples` (défaut: False) - Compiler les exemples
- `build_tests` (défaut: False) - Compiler les tests

---

### Option 2: vcpkg

#### Installation Simple

```bash
# Installer avec tous les modules par défaut
vcpkg install modular-cpp-framework

# Installer avec features spécifiques
vcpkg install modular-cpp-framework[logger,networking,profiling,realtime]

# Installer avec exemples
vcpkg install modular-cpp-framework[examples]
```

#### Utilisation dans CMakeLists.txt

```cmake
find_package(modular-cpp-framework CONFIG REQUIRED)

# Core seulement
target_link_libraries(your_app PRIVATE mcf::core)

# Avec modules
target_link_libraries(your_app PRIVATE
    mcf::core
    mcf::logger
    mcf::networking
    mcf::profiling
    mcf::realtime
)
```

**Features disponibles:**
- `logger` - Logger module
- `networking` - Networking module
- `profiling` - Profiling module
- `realtime` - Realtime module
- `examples` - Compile les exemples

---

## Compilation depuis les Sources

### Prérequis

- **CMake** 3.16 ou supérieur
- **Compilateur C++17**:
  - GCC 7+ (Linux)
  - Clang 5+ (macOS/Linux)
  - MSVC 2017+ (Windows)
- **Dépendances système**:
  - Linux: `libdl`, `pthread`
  - Windows: Support natif

### Étapes de Compilation

```bash
# Cloner le repository
git clone https://github.com/Sylvain-RZ/ModularCppFramework.git
cd ModularCppFramework

# Créer le répertoire de build
mkdir build && cd build

# Configurer avec CMake
cmake ..

# Compiler
make -j$(nproc)

# (Optionnel) Installer dans /usr/local
sudo make install
```

### Options de Build CMake

```bash
# Build avec tests et exemples (recommandé pour développement)
cmake -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON ..

# Build core seulement (production)
cmake -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF ..

# Build release optimisé
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build debug avec symboles
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Installation dans un préfixe personnalisé
cmake -DCMAKE_INSTALL_PREFIX=/opt/mcf ..
```

### Vérification de l'Installation

```bash
# Exécuter les tests (si BUILD_TESTS=ON)
ctest -V

# Exécuter un exemple (si BUILD_EXAMPLES=ON)
./bin/logger_example
```

---

## Intégration dans Votre Projet

Cette section explique comment **intégrer ModularCppFramework comme dépendance** dans votre propre projet.

### Sous-module Git (Recommandé pour débuter)

#### Étape 1: Ajouter comme Sous-module

```bash
cd MonProjet/
git submodule add https://github.com/Sylvain-RZ/ModularCppFramework.git external/ModularCppFramework
git submodule update --init --recursive
```

#### Étape 2: Structure Recommandée

```
MonProjet/
├── external/
│   └── ModularCppFramework/   # Sous-module Git
├── src/
│   └── main.cpp               # Votre application
├── plugins/                   # VOS plugins (compilés séparément)
│   └── my_plugin/
│       ├── MyPlugin.cpp
│       └── CMakeLists.txt
├── config/
│   └── app_config.json        # Votre configuration
├── CMakeLists.txt             # Build de votre projet
└── README.md
```

#### Étape 3: CMakeLists.txt

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

# Si vous utilisez des modules spécifiques
target_link_libraries(mon_app PRIVATE
    mcf_logger_module
    mcf_networking_module
    mcf_profiling_module
    mcf_realtime_module
)

# Si vous créez vos propres plugins
add_subdirectory(plugins/my_plugin)
```

#### Étape 4: Application Minimale

`src/main.cpp`:

```cpp
#include <core/Application.hpp>
#include <modules/logger/LoggerModule.hpp>
#include <iostream>

class MyApp : public mcf::Application {
protected:
    bool onInitialize() override {
        std::cout << "MyApp initializing...\n";

        // Charger configuration
        getConfigurationManager().loadFromFile("config/app_config.json");

        // Charger VOS plugins depuis ./plugins
        getPluginManager().loadPluginsFromDirectory("./plugins");
        getPluginManager().initializeAll();

        return true;
    }

    void onShutdown() override {
        std::cout << "MyApp shutting down...\n";
    }
};

int main() {
    MyApp app;
    app.addModule<mcf::LoggerModule>();
    return app.run();
}
```

#### Mise à Jour du Sous-module

```bash
cd external/ModularCppFramework
git pull origin main
cd ../..
git add external/ModularCppFramework
git commit -m "Update MCF to latest version"
```

---

### CMake FetchContent (Recommandé pour CI/CD)

Cette option télécharge automatiquement MCF lors du build (pas besoin de sous-module).

```cmake
cmake_minimum_required(VERSION 3.16)
project(MonProjet)

set(CMAKE_CXX_STANDARD 17)

# Télécharger ModularCppFramework automatiquement
include(FetchContent)

FetchContent_Declare(
    ModularCppFramework
    GIT_REPOSITORY https://github.com/Sylvain-RZ/ModularCppFramework.git
    GIT_TAG v1.0.2  # Spécifier une version stable
)

# Désactiver tests/exemples
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(ModularCppFramework)

# Votre application
add_executable(mon_app src/main.cpp)
target_link_libraries(mon_app PRIVATE mcf_core)
```

**Avantages:**
- Pas de gestion de sous-modules Git
- Version fixée (reproductible)
- Idéal pour CI/CD
- Téléchargement automatique lors du build

---

### Installation Système (Pour utilisateurs avancés)

#### Installation Globale

```bash
# Cloner et compiler MCF
git clone https://github.com/Sylvain-RZ/ModularCppFramework.git
cd ModularCppFramework
mkdir build && cd build

# Installer dans /usr/local (ou autre préfixe)
cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DBUILD_TESTS=OFF \
      -DBUILD_EXAMPLES=OFF \
      ..
make -j$(nproc)
sudo make install
```

#### Utilisation avec find_package

Dans votre `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)
project(MonProjet)

set(CMAKE_CXX_STANDARD 17)

# Chercher MCF installé globalement
find_package(ModularCppFramework REQUIRED)

add_executable(mon_app src/main.cpp)
target_link_libraries(mon_app PRIVATE mcf::core)
```

**Note:** Cette option nécessite que MCF soit installé sur chaque machine de développement.

---

## Ce qui est Inclus

### Avec `mcf_core` (Header-Only)

- EventBus, ServiceLocator, ResourceManager
- PluginManager, ConfigurationManager
- ThreadPool, Logger, FileSystem
- Application, IPlugin, IModule
- Toutes les interfaces et utilitaires core

### Modules Optionnels (à lier explicitement)

```cmake
target_link_libraries(mon_app PRIVATE
    mcf_logger_module      # Logger avec config JSON
    mcf_networking_module  # TCP Client/Server
    mcf_profiling_module   # Métriques de performance
    mcf_realtime_module    # Timestep fixe
)
```

---

## Organisation des Fichiers Runtime

```
build/
├── bin/
│   └── mon_app              # Votre exécutable
└── plugins/
    └── my_plugin.so         # Vos plugins (.dll sur Windows)
```

Le `PluginManager` cherche automatiquement dans `./plugins` relatif à l'exécutable.

---

## Prochaines Étapes

- **[Guide d'Utilisation](USAGE.md)** - Apprendre à utiliser les composants
- **[Exemples](EXAMPLES.md)** - Voir des exemples fonctionnels
- **[Guide Plugins](PLUGIN_GUIDE.md)** - Créer vos propres plugins
- **[Quick Start](QUICK_START.md)** - Premier projet en 5 minutes

---

## Dépannage

### Erreur: "mcf::core not found"

Vérifiez que vous avez bien ajouté MCF à votre projet:
```cmake
add_subdirectory(external/ModularCppFramework)
target_link_libraries(your_app PRIVATE mcf_core)
```

### Erreur de Compilation C++17

Assurez-vous que C++17 est activé:
```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

### Plugins ne se chargent pas

- Vérifiez que les plugins sont dans `./plugins` relatif à l'exécutable
- Vérifiez que `MCF_PLUGIN_EXPORT(YourPlugin)` est présent
- Utilisez `nm -D plugin.so` (Linux) pour vérifier les symboles exportés

---

**Retour:** [Documentation SDK](README.md) | **Suivant:** [Guide d'Utilisation](USAGE.md)

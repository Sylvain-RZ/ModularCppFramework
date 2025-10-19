# CMake System - ModularCppFramework

Ce répertoire contient les fichiers CMake pour le packaging, la distribution et les utilitaires de développement de ModularCppFramework.

## 🚀 Quick Start

**Créer un nouveau plugin en 30 secondes:**
```bash
python3 tools/create-plugin.py -n MyPlugin -r    # Linux/macOS
python tools/create-plugin.py -n MyPlugin -r     # Windows
```

**Créer une nouvelle application en 30 secondes:**
```bash
python3 tools/create-application.py -n MyApp -r -c -m logger    # Linux/macOS
python tools/create-application.py -n MyApp -r -c -m logger     # Windows
```

Voir [QUICKSTART.md](../docs/sdk/generators/QUICKSTART.md) et [tools/README.md](../tools/README.md) pour plus de détails.

## Fichiers

### ModularCppFrameworkConfig.cmake.in
Fichier de configuration CMake pour `find_package()`. Permet aux utilisateurs de trouver et utiliser MCF via :
```cmake
find_package(ModularCppFramework REQUIRED)
target_link_libraries(my_app PRIVATE mcf::core mcf::networking)
```

### ModularCppFrameworkConfigVersion.cmake.in
Fichier de version CMake pour la compatibilité sémantique. Vérifie que la version trouvée correspond aux exigences.

### MCFPluginGenerator.cmake
**Nouveau!** Générateur de templates pour créer rapidement de nouveaux plugins.

#### Fonction `mcf_generate_plugin()`

Génère automatiquement un nouveau plugin avec tous les fichiers nécessaires.

**Usage** :
```cmake
include(cmake/MCFPluginGenerator.cmake)

mcf_generate_plugin(
    NAME AudioPlugin
    VERSION 1.0.0
    AUTHOR "Audio Team"
    DESCRIPTION "Audio processing plugin"
    PRIORITY 200
    REALTIME              # Ajoute IRealtimeUpdatable
    EVENT_DRIVEN          # Ajoute IEventDriven
)
```

**Génère** :
- `plugins/AudioPlugin/AudioPlugin.cpp` - Code source du plugin
- `plugins/AudioPlugin/CMakeLists.txt` - Configuration de build
- `plugins/AudioPlugin/README.md` - Documentation du plugin

**Script Helper** :
```bash
python3 tools/create-plugin.py -n AudioPlugin -v 1.0.0 -a "Audio Team" -r -e
```

**Arguments** :
- `NAME` (requis) - Nom du plugin
- `VERSION` - Version (défaut: 1.0.0)
- `AUTHOR` - Auteur (défaut: MCF Developer)
- `DESCRIPTION` - Description du plugin
- `PRIORITY` - Priorité de chargement (défaut: 100)
- `REALTIME` - Ajoute interface IRealtimeUpdatable
- `EVENT_DRIVEN` - Ajoute interface IEventDriven
- `DEPENDENCIES` - Liste de dépendances
- `OUTPUT_DIR` - Répertoire de sortie (défaut: plugins/<nom>)

### MCFApplicationGenerator.cmake
**Nouveau!** Générateur de templates pour créer rapidement de nouvelles applications.

#### Fonction `mcf_generate_application()`

Génère automatiquement une nouvelle application MCF avec tous les fichiers nécessaires.

**Usage** :
```cmake
include(cmake/MCFApplicationGenerator.cmake)

mcf_generate_application(
    NAME MyGame
    VERSION 1.0.0
    AUTHOR "Game Team"
    DESCRIPTION "My awesome game"
    MODULES logger profiling
    REALTIME              # Ajoute boucle de mise à jour
    CONFIG                # Génère config.json
)
```

**Génère** :
- `MyGame/src/main.cpp` - Code source de l'application
- `MyGame/CMakeLists.txt` - Configuration de build
- `MyGame/README.md` - Documentation de l'application
- `MyGame/config/config.json` - Configuration (si CONFIG)
- `MyGame/.gitignore` - Git ignore rules

**Script Helper** :
```bash
python3 tools/create-application.py -n MyGame -v 1.0.0 -r -c -m logger,profiling
```

**Arguments** :
- `NAME` (requis) - Nom de l'application
- `VERSION` - Version (défaut: 1.0.0)
- `AUTHOR` - Auteur (défaut: MCF Developer)
- `DESCRIPTION` - Description de l'application
- `MODULES` - Modules à inclure (logger, networking, profiling, realtime)
- `PLUGINS` - Plugins à charger
- `REALTIME` - Ajoute boucle de mise à jour
- `EVENT_DRIVEN` - Ajoute architecture événementielle
- `CONFIG` - Génère config.json
- `OUTPUT_DIR` - Répertoire de sortie (défaut: <name>)

### MCFPackaging.cmake
Fonctions utilitaires pour packager **vos applications** utilisant MCF.

#### Fonction `mcf_package_application()`

Crée un package de votre application incluant binaires, plugins, config et ressources.

**Usage** :
```cmake
include(external/ModularCppFramework/cmake/MCFPackaging.cmake)

mcf_package_application(
    TARGET mon_app
    VERSION 1.0.0
    OUTPUT_NAME "MonApplication"
    PLUGINS my_plugin autre_plugin
    CONFIG_FILES config/app_config.json
    RESOURCES textures/ models/
)
```

**Génère** :
- Target CMake : `package-mon_app`
- Archive : `MonApplication-1.0.0-linux-x86_64.tar.gz`

**Contenu du package** :
```
MonApplication-1.0.0/
├── bin/
│   └── mon_app
├── plugins/
│   ├── my_plugin.so
│   └── autre_plugin.so
├── config/
│   └── app_config.json
├── resources/
│   ├── textures/
│   └── models/
└── README.txt
```

#### Fonction `mcf_install_application()`

Crée des règles d'installation système pour votre application.

**Usage** :
```cmake
mcf_install_application(
    TARGET mon_app
    PLUGINS my_plugin
    CONFIG_FILES config/app_config.json
)
```

**Installation** :
```bash
sudo make install
# → /usr/local/bin/mon_app
# → /usr/local/lib/mon_app/plugins/my_plugin.so
# → /usr/local/etc/mon_app/app_config.json
```

## Utilisation

### 1. Générer un Nouveau Plugin

#### Méthode 1: Script Shell (Recommandé)

```bash
# Plugin basique
python3 tools/create-plugin.py -n MyPlugin

# Plugin avec realtime updates
python3 tools/create-plugin.py -n PhysicsPlugin -r

# Plugin complet avec toutes les options
python3 tools/create-plugin.py \
    -n NetworkPlugin \
    -v 2.0.0 \
    -a "Network Team" \
    -d "Network communication plugin" \
    -p 200 \
    -r -e
```

**Options** :
- `-n, --name` - Nom du plugin (requis)
- `-v, --version` - Version (défaut: 1.0.0)
- `-a, --author` - Auteur (défaut: MCF Developer)
- `-d, --description` - Description
- `-p, --priority` - Priorité de chargement (défaut: 100)
- `-r, --realtime` - Ajoute IRealtimeUpdatable
- `-e, --event-driven` - Ajoute IEventDriven
- `-h, --help` - Aide

#### Méthode 2: CMake Direct

Créez un fichier temporaire `generate.cmake` :
```cmake
include(cmake/MCFPluginGenerator.cmake)

mcf_generate_plugin(
    NAME MyPlugin
    VERSION 1.0.0
    REALTIME
)
```

Exécutez :
```bash
cmake -P generate.cmake
```

#### Après Génération

1. Ajoutez votre plugin à `plugins/CMakeLists.txt` :
```cmake
add_subdirectory(MyPlugin)
```

2. Implémentez la logique dans `plugins/MyPlugin/MyPlugin.cpp`

3. Compilez :
```bash
cd build
cmake ..
make -j$(nproc)
```

Le plugin sera dans `build/plugins/myplugin.so`

### 2. Packager le Framework MCF

Depuis le repo MCF :
```bash
cd ModularCppFramework/build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF ..
make -j4
make package-release
```

Produit : `modular-cpp-framework-1.0.2-Linux-x86_64.tar.gz`

### 3. Packager Votre Application

#### Dans le repo MCF

```cmake
# examples/my_app/CMakeLists.txt
include(${CMAKE_SOURCE_DIR}/cmake/MCFPackaging.cmake)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE mcf_core)

mcf_package_application(
    TARGET my_app
    VERSION 1.0.0
    OUTPUT_NAME "MyApp"
)
```

```bash
cd ModularCppFramework/build
make package-my_app
# → MyApp-1.0.0-linux-x86_64.tar.gz
```

#### Projet Externe

```cmake
# MonProjet/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(MonProjet VERSION 1.0.0)

# Ajouter MCF
add_subdirectory(external/ModularCppFramework)

# Inclure le système de packaging
include(external/ModularCppFramework/cmake/MCFPackaging.cmake)

add_executable(mon_app src/main.cpp)
target_link_libraries(mon_app PRIVATE mcf_core)

add_library(my_plugin SHARED plugins/MyPlugin.cpp)
target_link_libraries(my_plugin PRIVATE mcf_core)
set_target_properties(my_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
)

mcf_package_application(
    TARGET mon_app
    VERSION ${PROJECT_VERSION}
    OUTPUT_NAME "MonProjet"
    PLUGINS my_plugin
    CONFIG_FILES config/app.json
)
```

```bash
cd MonProjet/build
cmake ..
make -j4
make package-mon_app
# → MonProjet-1.0.0-linux-x86_64.tar.gz
```

## Targets Disponibles

| Target | Description | Sortie |
|--------|-------------|--------|
| `package-release` | Package le framework MCF | `modular-cpp-framework-*.tar.gz` |
| `package-<app>` | Package votre application | `<OutputName>-*.tar.gz` |
| `install` | Installation système | `/usr/local/...` |

## Structure des Packages

### Framework MCF
```
modular-cpp-framework-1.0.2/
├── include/mcf/          # Headers (core + modules)
├── lib/                  # Bibliothèques compilées (.a)
├── cmake/                # Fichiers CMake pour find_package()
├── docs/                 # Documentation complète
├── LICENSE
├── README.md
└── CHANGELOG.md
```

### Votre Application
```
MonApp-1.0.0/
├── bin/                  # Exécutables
├── plugins/              # Plugins (.so/.dll)
├── config/               # Configuration
├── resources/            # Ressources (textures, models, etc.)
└── README.txt
```

## Documentation Complète

- **[PLUGIN_GENERATOR.md](../docs/sdk/generators/PLUGIN_GENERATOR.md)** - Guide complet du générateur de plugins
- **[examples/](examples/)** - Exemples d'utilisation du générateur
- **[templates/](templates/)** - Templates de génération de plugins

## Exemples

Voir [docs/development/PACKAGING.md](../docs/development/PACKAGING.md) pour des exemples complets de packaging.

## Notes

- Les packages sont nommés automatiquement : `<name>-<version>-<platform>-<arch>.tar.gz`
- Les chemins utilisent generator expressions pour build vs install
- Les modules INTERFACE (logger) n'ont pas de .a (header-only)
- Les modules STATIC (networking, profiling, realtime) sont packagés comme .a

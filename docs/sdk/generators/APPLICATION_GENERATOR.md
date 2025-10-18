# MCF Application Generator - Guide Complet

Le système de génération d'applications MCF permet de créer rapidement de nouveaux projets utilisant ModularCppFramework avec une structure standardisée et toutes les fonctionnalités nécessaires.

## Table des Matières

1. [Installation](#installation)
2. [Utilisation Rapide](#utilisation-rapide)
3. [Options Disponibles](#options-disponibles)
4. [Types d'Applications](#types-dapplications)
5. [Modules Disponibles](#modules-disponibles)
6. [Exemples](#exemples)
7. [Structure Générée](#structure-générée)
8. [Workflow de Développement](#workflow-de-développement)

## Installation

Le générateur est inclus dans le dépôt MCF. Aucune installation supplémentaire n'est nécessaire.

**Fichiers:**
- `cmake/MCFApplicationGenerator.cmake` - Fonctions CMake
- `cmake/create-application.sh` - Script shell helper
- `cmake/templates/Application_*.in` - Templates de génération

## Utilisation Rapide

### Méthode 1: Script Shell (Recommandé)

```bash
# Depuis la racine du projet MCF
./cmake/create-application.sh -n MyApp
```

Cela génère une application basique dans `MyApp/`.

### Méthode 2: Make

```bash
make -f cmake/Makefile app NAME=MyApp
```

### Méthode 3: CMake Direct

```cmake
# generate.cmake
include(cmake/MCFApplicationGenerator.cmake)

mcf_generate_application(
    NAME MyApp
    VERSION 1.0.0
)
```

```bash
cmake -P generate.cmake
```

## Options Disponibles

### Script Shell

```bash
./cmake/create-application.sh [options]
```

| Option | Description | Requis | Défaut |
|--------|-------------|--------|--------|
| `-n, --name NAME` | Nom de l'application | ✅ Oui | - |
| `-v, --version VERSION` | Version de l'application | Non | 1.0.0 |
| `-a, --author AUTHOR` | Auteur de l'application | Non | MCF Developer |
| `-d, --description DESC` | Description | Non | Application built with MCF |
| `-m, --modules MODULES` | Modules à inclure (virgule) | Non | - |
| `-p, --plugins PLUGINS` | Plugins à charger (virgule) | Non | - |
| `-r, --realtime` | Ajoute boucle de mise à jour | Non | - |
| `-e, --event-driven` | Ajoute architecture événementielle | Non | - |
| `-c, --config` | Génère config.json | Non | - |
| `-o, --output DIR` | Répertoire de sortie | Non | ./<name> |
| `-h, --help` | Affiche l'aide | Non | - |

### Fonction CMake

```cmake
mcf_generate_application(
    NAME <name>               # Requis
    [VERSION <version>]       # Optionnel, défaut: 1.0.0
    [AUTHOR <author>]         # Optionnel
    [DESCRIPTION <desc>]      # Optionnel
    [MODULES <mod1> <mod2>]   # Optionnel
    [PLUGINS <p1> <p2>]       # Optionnel
    [REALTIME]               # Flag optionnel
    [EVENT_DRIVEN]           # Flag optionnel
    [CONFIG]                 # Flag optionnel
    [OUTPUT_DIR <dir>]       # Optionnel, défaut: <name>
)
```

## Types d'Applications

### 1. Application Basique

**Cas d'usage:** Application simple sans boucle de mise à jour ni événements.

```bash
./cmake/create-application.sh -n MyApp
```

**Caractéristiques:**
- Structure minimale
- Initialisation et shutdown
- Pas de boucle de mise à jour
- Idéal pour CLI tools, batch processing

### 2. Application Realtime

**Cas d'usage:** Application avec boucle de mise à jour (jeux, simulations).

```bash
./cmake/create-application.sh -n MyGame -r -c
```

**Caractéristiques:**
- Méthode `onUpdate(float deltaTime)` générée
- Boucle de mise à jour automatique
- Configuration FPS dans config.json
- Idéal pour jeux, simulateurs, animations

### 3. Application Event-Driven

**Cas d'usage:** Application réactive basée sur événements.

```bash
./cmake/create-application.sh -n MyServer -e -c
```

**Caractéristiques:**
- Méthode `setupEvents()` générée
- Souscriptions EventBus configurées
- Pas de boucle active
- Idéal pour serveurs, services, outils

### 4. Application Full-Featured

**Cas d'usage:** Application complète avec toutes les fonctionnalités.

```bash
./cmake/create-application.sh -n MyGame -r -e -c -m logger,profiling
```

**Caractéristiques:**
- Update loop realtime
- Architecture événementielle
- Configuration JSON
- Modules intégrés
- Idéal pour applications complexes

## Modules Disponibles

Les modules suivants peuvent être inclus avec `-m` ou `MODULES`:

| Module | Description | Usage |
|--------|-------------|-------|
| `logger` | Système de logging | `-m logger` |
| `networking` | TCP client/server | `-m networking` |
| `profiling` | Métriques de performance | `-m profiling` |
| `realtime` | Fixed timestep updates | `-m realtime` |

**Exemple multi-modules:**
```bash
-m logger,networking,profiling
```

## Exemples

### Exemple 1: Jeu Simple

```bash
./cmake/create-application.sh \
    -n SimpleGame \
    -v 1.0.0 \
    -a "Game Team" \
    -d "A simple 2D game" \
    -m logger,profiling,realtime \
    -r \
    -c
```

**Génère:**
- Application avec update loop
- Modules logger, profiling, realtime
- Configuration JSON
- Prêt pour développement de jeu

### Exemple 2: Serveur Network

```bash
./cmake/create-application.sh \
    -n GameServer \
    -v 2.0.0 \
    -a "Server Team" \
    -d "Multiplayer game server" \
    -m logger,networking \
    -e \
    -c
```

**Génère:**
- Application event-driven
- Modules logger, networking
- Configuration pour serveur
- Prêt pour développement réseau

### Exemple 3: Outil CLI

```bash
./cmake/create-application.sh \
    -n DataProcessor \
    -v 1.0.0 \
    -d "Batch data processing tool" \
    -m logger
```

**Génère:**
- Application basique
- Module logger seulement
- Idéal pour traitement batch

### Exemple 4: Application avec Plugins

```bash
./cmake/create-application.sh \
    -n ModularApp \
    -v 1.0.0 \
    -m logger,profiling \
    -p physics_plugin,audio_plugin \
    -r \
    -c
```

**Génère:**
- Application avec plugins pré-configurés
- Stubs pour charger physics_plugin et audio_plugin
- Configuration complète

## Structure Générée

Pour une application nommée `MyApp`, le générateur crée:

```
MyApp/
├── src/
│   └── main.cpp              # Code source principal
├── include/                  # Headers (vide par défaut)
├── config/
│   └── config.json           # Configuration (si -c)
├── CMakeLists.txt            # Configuration build
├── README.md                 # Documentation
└── .gitignore                # Git ignore rules
```

### src/main.cpp

Contient:
- Classe `MyApp` héritant de `mcf::Application`
- Méthode `onInitialize()` avec modules enregistrés
- Méthode `onShutdown()` pour cleanup
- Méthode `onUpdate(float)` si realtime
- Méthode `setupEvents()` si event-driven
- Fonction `main()` pour lancer l'application

### CMakeLists.txt

Contient:
- Configuration C++17
- Recherche/inclusion de MCF
- Target executable
- Link vers modules sélectionnés
- Copie des fichiers de configuration
- Support packaging optionnel

### config/config.json

Contient:
- Configuration de l'application (nom, version, FPS)
- Configuration de logging
- Configuration de profiling
- Configuration de plugins

### README.md

Contient:
- Informations de l'application
- Instructions de build
- Guide d'utilisation
- Documentation de développement

## Workflow de Développement

### 1. Génération

```bash
./cmake/create-application.sh -n MyGame -r -c -m logger,profiling
```

### 2. Configuration MCF

Si MCF n'est pas dans le parent directory, éditez `MyGame/CMakeLists.txt`:

```cmake
# Option A: MCF comme subdirectory
add_subdirectory(external/ModularCppFramework)

# Option B: MCF installé system-wide
find_package(ModularCppFramework REQUIRED)

# Option C: MCF avec chemin personnalisé
set(MCF_ROOT "/path/to/ModularCppFramework")
add_subdirectory(${MCF_ROOT}/core ${CMAKE_BINARY_DIR}/mcf_core)
```

### 3. Implémentation

Éditez `src/main.cpp`:

```cpp
bool onInitialize() override {
    // Votre code d'initialisation

    // Charger resources
    // Initialiser systèmes
    // Setup état initial

    return true;
}

void onUpdate(float deltaTime) override {
    // Votre logique de mise à jour

    // Traiter input
    // Update game state
    // Render
}
```

### 4. Compilation

```bash
cd MyGame
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 5. Exécution

```bash
./bin/mygame
```

### 6. Ajout de Fonctionnalités

#### Ajouter de Nouveaux Fichiers

Créez `src/GameState.cpp` et éditez `CMakeLists.txt`:

```cmake
add_executable(mygame
    src/main.cpp
    src/GameState.cpp
    src/Renderer.cpp
)
```

#### Ajouter des Modules

Éditez `CMakeLists.txt`:

```cmake
target_link_libraries(mygame PRIVATE
    mcf_core
    mcf_networking_module  # Nouveau
)
```

Éditez `src/main.cpp`:

```cpp
#include "modules/networking/NetworkingModule.hpp"

bool onInitialize() override {
    addModule<mcf::NetworkingModule>();  // Nouveau
    return true;
}
```

#### Ajouter des Plugins

1. Créez le plugin (voir PLUGIN_GENERATOR.md)
2. Chargez-le dans `onInitialize()`:

```cpp
m_pluginManager.loadPlugin("plugins/my_plugin.so");
```

### 7. Packaging

```bash
cd build
make package-mygame
```

Génère: `MyGame-1.0.0-<platform>-<arch>.tar.gz`

## Bonnes Pratiques

### 1. Organisation du Code

**Structure recommandée:**
```
MyApp/
├── src/
│   ├── main.cpp            # Entry point
│   ├── Application.cpp     # Logique principale
│   ├── systems/            # Game systems
│   ├── components/         # Components
│   └── utils/              # Utilitaires
├── include/
│   └── MyApp/              # Headers publics
├── assets/                 # Resources
│   ├── textures/
│   ├── models/
│   └── sounds/
├── config/
│   ├── config.json         # Config principale
│   └── levels/             # Configs de niveaux
└── plugins/                # Plugins personnalisés
```

### 2. Configuration

Utilisez `config.json` pour:
- Paramètres d'application (FPS, resolution, etc.)
- Configuration de modules
- Chemins de resources
- Options de debug

**Évitez:** Hardcoder des valeurs dans le code

### 3. Modules vs Plugins

**Utilisez des modules pour:**
- Fonctionnalités core requises
- Code compilé avec l'application
- Performance critique

**Utilisez des plugins pour:**
- Fonctionnalités optionnelles
- Extensions tierces
- Hot-reload pendant développement

### 4. Gestion d'Événements

Centralisez les événements:

```cpp
// Définissez vos événements
namespace Events {
    constexpr char PLAYER_DIED[] = "game.player.died";
    constexpr char LEVEL_COMPLETE[] = "game.level.complete";
}

// Utilisez-les
eventBus->publish(Events::PLAYER_DIED, event);
eventBus->subscribe(Events::LEVEL_COMPLETE, handler);
```

### 5. Logging

Configurez les niveaux de log selon l'environnement:

```json
{
    "logging": {
        "level": "debug",    // Debug build
        "level": "info"      // Release build
    }
}
```

## Troubleshooting

### Application ne compile pas - MCF Not Found

**Problème:** `ModularCppFramework not found`

**Solution:** Mettez à jour `CMakeLists.txt` avec le bon chemin MCF:

```cmake
# Si MCF est dans external/
add_subdirectory(external/ModularCppFramework)

# Si MCF est ailleurs
set(MCF_ROOT "../ModularCppFramework")
```

### Runtime Error - Config Not Found

**Problème:** `config.json not found`

**Solution:** Les configs sont copiées dans `build/config/`:

```bash
# Vérifiez
ls build/config/

# Ou lancez depuis build/
cd build && ./bin/myapp
```

### Modules Not Linking

**Problème:** Undefined references to module symbols

**Solution:** Vérifiez les liens dans `CMakeLists.txt`:

```cmake
target_link_libraries(myapp PRIVATE
    mcf_core
    mcf_networking_module   # Assurez-vous que c'est présent
)
```

### Plugin Loading Fails

**Problème:** `dlopen: file not found`

**Solution:**
1. Vérifiez que le plugin est compilé: `ls plugins/`
2. Utilisez le bon chemin: `plugins/my_plugin.so`
3. Vérifiez les permissions: `chmod +x plugins/my_plugin.so`

## Voir Aussi

- [PLUGIN_GENERATOR.md](PLUGIN_GENERATOR.md) - Guide de création de plugins
- [README.md](README.md) - Documentation cmake principale
- [QUICKSTART.md](QUICKSTART.md) - Démarrage rapide
- [../docs/sdk/](../docs/sdk/) - Documentation complète MCF

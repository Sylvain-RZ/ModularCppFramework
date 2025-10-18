# MCF Plugin Generator - Guide Complet

Le système de génération de plugins MCF permet de créer rapidement de nouveaux plugins avec une structure standardisée et toutes les interfaces nécessaires.

## Table des Matières

1. [Installation](#installation)
2. [Utilisation Rapide](#utilisation-rapide)
3. [Options Disponibles](#options-disponibles)
4. [Types de Plugins](#types-de-plugins)
5. [Exemples](#exemples)
6. [Structure Générée](#structure-générée)
7. [Personnalisation](#personnalisation)

## Installation

Le générateur est inclus dans le dépôt MCF. Aucune installation supplémentaire n'est nécessaire.

**Fichiers:**
- `cmake/MCFPluginGenerator.cmake` - Fonctions CMake
- `cmake/create-plugin.sh` - Script shell helper
- `cmake/templates/` - Templates de génération

## Utilisation Rapide

### Méthode 1: Script Shell (Recommandé)

```bash
# Depuis la racine du projet MCF
./cmake/create-plugin.sh -n MyPlugin
```

Cela génère un plugin basique dans `plugins/MyPlugin/`.

### Méthode 2: CMake Direct

```cmake
# generate.cmake
include(cmake/MCFPluginGenerator.cmake)

mcf_generate_plugin(
    NAME MyPlugin
    VERSION 1.0.0
)
```

```bash
cmake -P generate.cmake
```

## Options Disponibles

### Script Shell

```bash
./cmake/create-plugin.sh [options]
```

| Option | Description | Requis | Défaut |
|--------|-------------|--------|--------|
| `-n, --name NAME` | Nom du plugin | ✅ Oui | - |
| `-v, --version VERSION` | Version du plugin | Non | 1.0.0 |
| `-a, --author AUTHOR` | Auteur du plugin | Non | MCF Developer |
| `-d, --description DESC` | Description | Non | Plugin for ModularCppFramework |
| `-p, --priority NUM` | Priorité de chargement | Non | 100 |
| `-r, --realtime` | Ajoute IRealtimeUpdatable | Non | - |
| `-e, --event-driven` | Ajoute IEventDriven | Non | - |
| `-h, --help` | Affiche l'aide | Non | - |

### Fonction CMake

```cmake
mcf_generate_plugin(
    NAME <name>              # Requis
    [VERSION <version>]      # Optionnel, défaut: 1.0.0
    [AUTHOR <author>]        # Optionnel
    [DESCRIPTION <desc>]     # Optionnel
    [PRIORITY <priority>]    # Optionnel, défaut: 100
    [DEPENDENCIES <dep1> <dep2> ...] # Optionnel
    [REALTIME]              # Flag optionnel
    [EVENT_DRIVEN]          # Flag optionnel
    [OUTPUT_DIR <dir>]      # Optionnel, défaut: plugins/<name>
)
```

## Types de Plugins

### 1. Plugin Basique (IPlugin)

**Cas d'usage:** Plugin simple sans mises à jour en temps réel ni gestion d'événements.

```bash
./cmake/create-plugin.sh -n ConfigLoaderPlugin
```

**Interfaces:** `IPlugin`

### 2. Plugin Realtime (IPlugin + IRealtimeUpdatable)

**Cas d'usage:** Plugin nécessitant des mises à jour à chaque frame (physique, animation, IA).

```bash
./cmake/create-plugin.sh -n PhysicsPlugin -r
```

**Interfaces:** `IPlugin`, `IRealtimeUpdatable`

**Méthode ajoutée:**
```cpp
void onRealtimeUpdate(float deltaTime) override {
    // Appelé à chaque frame avec deltaTime en secondes
}
```

### 3. Plugin Event-Driven (IPlugin + IEventDriven)

**Cas d'usage:** Plugin réagissant uniquement aux événements (notifications, logs, métriques).

```bash
./cmake/create-plugin.sh -n NotificationPlugin -e
```

**Interfaces:** `IPlugin`, `IEventDriven`

**Méthode ajoutée:**
```cpp
void onEvent(const mcf::Event& event) override {
    // Appelé pour chaque événement
}
```

### 4. Plugin Full-Featured (IPlugin + IRealtimeUpdatable + IEventDriven)

**Cas d'usage:** Plugin complexe combinant updates temps réel et gestion d'événements (gameplay, networking).

```bash
./cmake/create-plugin.sh -n GameLogicPlugin -r -e
```

**Interfaces:** `IPlugin`, `IRealtimeUpdatable`, `IEventDriven`

## Exemples

### Exemple 1: Plugin Audio

```bash
./cmake/create-plugin.sh \
    -n AudioPlugin \
    -v 2.0.0 \
    -a "Audio Team" \
    -d "Audio processing and playback system" \
    -p 200 \
    -r
```

### Exemple 2: Plugin Network

```bash
./cmake/create-plugin.sh \
    -n NetworkPlugin \
    -v 1.5.0 \
    -a "Network Team" \
    -d "Network communication layer" \
    -p 300 \
    -r -e
```

### Exemple 3: Plugin Analytics

```bash
./cmake/create-plugin.sh \
    -n AnalyticsPlugin \
    -v 1.0.0 \
    -a "Analytics Team" \
    -d "Collects and reports analytics data" \
    -e
```

### Exemple 4: Multiple plugins via CMake

```cmake
# generate_all.cmake
include(cmake/MCFPluginGenerator.cmake)

# Core gameplay plugins
mcf_generate_plugin(NAME PlayerController REALTIME PRIORITY 500)
mcf_generate_plugin(NAME EnemyAI REALTIME PRIORITY 400)
mcf_generate_plugin(NAME LevelManager EVENT_DRIVEN PRIORITY 300)

# Utility plugins
mcf_generate_plugin(NAME SaveSystem EVENT_DRIVEN)
mcf_generate_plugin(NAME AchievementTracker EVENT_DRIVEN)
```

```bash
cmake -P generate_all.cmake
```

## Structure Générée

Pour un plugin nommé `MyPlugin`, le générateur crée:

```
plugins/MyPlugin/
├── MyPlugin.cpp          # Code source du plugin
├── CMakeLists.txt        # Configuration de build
└── README.md             # Documentation du plugin
```

### MyPlugin.cpp

Contient:
- Classe `MyPlugin` héritant des interfaces demandées
- Constructeur avec métadonnées configurées
- Méthodes `initialize()` et `shutdown()` avec TODOs
- Stubs pour EventBus, ServiceLocator, ConfigurationManager
- Export de symboles via `MCF_PLUGIN_EXPORT()`
- Méthode `getManifestJson()` pour le chargement dynamique

### CMakeLists.txt

Contient:
- Target `add_library(myplugin SHARED)`
- Link vers `mcf_core`
- Configuration de visibilité des symboles
- Output directory vers `${PLUGIN_OUTPUT_DIRECTORY}`
- Règles d'installation

### README.md

Contient:
- Informations du plugin (nom, version, auteur)
- Instructions de configuration
- Exemples d'utilisation
- Guide de développement

## Workflow de Développement

### 1. Génération

```bash
./cmake/create-plugin.sh -n MyPlugin -r
```

### 2. Ajout au Build

Éditez `plugins/CMakeLists.txt`:
```cmake
add_subdirectory(MyPlugin)
```

### 3. Implémentation

Éditez `plugins/MyPlugin/MyPlugin.cpp`:
```cpp
bool initialize(PluginContext& context) override {
    m_context = context;

    // Votre code d'initialisation
    if (m_context.getEventBus()) {
        m_context.getEventBus()->subscribe("game.start",
            [this](const Event& e) {
                // Handle game start
            }
        );
    }

    m_initialized = true;
    return true;
}

void onRealtimeUpdate(float deltaTime) override {
    // Votre logique de mise à jour
}
```

### 4. Compilation

```bash
cd build
cmake ..
make -j$(nproc)
```

Le plugin compilé sera dans `build/plugins/myplugin.so` (Linux) ou `build/plugins/myplugin.dll` (Windows).

### 5. Test

Créez un test dans `tests/`:
```cpp
#include <catch2/catch_amalgamated.hpp>
#include "../plugins/MyPlugin/MyPlugin.cpp"

TEST_CASE("MyPlugin initialization", "[plugin]") {
    mcf::MyPlugin plugin;
    REQUIRE(plugin.getName() == "MyPlugin");
}
```

## Personnalisation

### Modifier les Templates

Les templates sont dans `cmake/templates/`:

1. **Plugin.cpp.in** - Template du code source
2. **PluginCMakeLists.txt.in** - Template CMakeLists.txt
3. **PluginREADME.md.in** - Template README

**Variables disponibles:**
- `@PLUGIN_NAME@` - Nom de classe (MyPlugin)
- `@PLUGIN_NAME_LOWER@` - Nom en minuscules (myplugin)
- `@PLUGIN_NAME_UPPER@` - Nom en majuscules (MYPLUGIN)
- `@PLUGIN_VERSION@` - Version (1.0.0)
- `@PLUGIN_AUTHOR@` - Auteur
- `@PLUGIN_DESCRIPTION@` - Description
- `@PLUGIN_PRIORITY@` - Priorité
- `@PLUGIN_INTERFACES@` - Héritage d'interfaces
- `@PLUGIN_INTERFACE_INCLUDES@` - Includes d'interfaces
- `@PLUGIN_INTERFACE_METHODS@` - Implémentations de méthodes
- `@PLUGIN_DEPS_JSON@` - Dépendances JSON

### Ajouter des Dépendances

Via shell:
```bash
# Note: Dependencies ne sont pas supportées par le script shell actuellement
# Utilisez CMake direct:
```

Via CMake:
```cmake
mcf_generate_plugin(
    NAME DependentPlugin
    DEPENDENCIES PhysicsPlugin AudioPlugin
)
```

Cela génère le JSON de dépendances:
```json
"dependencies": [
    {"name": "PhysicsPlugin", "minVersion": "1.0.0"},
    {"name": "AudioPlugin", "minVersion": "1.0.0"}
]
```

## Bonnes Pratiques

### 1. Conventions de Nommage

- **Nom de plugin:** PascalCase (MyPlugin, AudioSystem)
- **Fichier source:** Même nom que la classe (MyPlugin.cpp)
- **Target CMake:** snake_case (my_plugin, audio_system)

### 2. Priorités de Chargement

- **0-99:** Plugins utilitaires de bas niveau
- **100-199:** Plugins système
- **200-299:** Plugins de services
- **300-499:** Plugins d'application
- **500+:** Plugins de gameplay

### 3. Gestion de Version

Utilisez semantic versioning (MAJOR.MINOR.PATCH):
- MAJOR: Breaking changes
- MINOR: New features (backward compatible)
- PATCH: Bug fixes

### 4. Dépendances

- Déclarez toutes les dépendances explicitement
- Utilisez des version ranges (`minVersion`, `maxVersion`)
- Marquez les dépendances optionnelles si applicable

## Troubleshooting

### Plugin ne compile pas

**Erreur:** `undefined reference to mcf::IPlugin::~IPlugin()`

**Solution:** Vérifiez que le plugin linke bien `mcf_core`:
```cmake
target_link_libraries(my_plugin PRIVATE mcf_core)
```

### Symboles non exportés

**Erreur:** `dlsym: undefined symbol: createPlugin`

**Solution:** Vérifiez que `MCF_PLUGIN_EXPORT()` est présent à la fin du fichier:
```cpp
MCF_PLUGIN_EXPORT(mcf::MyPlugin)
```

### Plugin ne se charge pas

**Erreur:** Plugin pas trouvé dans `plugins/`

**Solution:** Vérifiez le `LIBRARY_OUTPUT_DIRECTORY` dans CMakeLists.txt:
```cmake
set_target_properties(my_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_DIRECTORY}
)
```

## Voir Aussi

- [README.md](README.md) - Documentation principale du système cmake
- [examples/](examples/) - Exemples d'utilisation
- [templates/](templates/) - Templates de génération
- [../docs/sdk/PLUGIN_GUIDE.md](../docs/sdk/PLUGIN_GUIDE.md) - Guide complet de développement de plugins

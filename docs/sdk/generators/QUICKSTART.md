# Quick Start - MCF Generators

Commencez à créer des plugins et applications en 30 secondes!

## Générateurs Disponibles

- **[Plugin Generator](#plugins)** - Créer des plugins MCF
- **[Application Generator](#applications)** - Créer des applications MCF

---

# Plugins

## Méthode 1: Script Shell (Le plus simple)

```bash
# Depuis la racine du projet
./cmake/create-plugin.sh -n MyPlugin
```

C'est tout! Votre plugin est généré dans `plugins/MyPlugin/`.

## Méthode 2: Makefile (Raccourcis pratiques)

```bash
# Plugin basique
make -f cmake/Makefile plugin-basic NAME=MyPlugin

# Plugin realtime
make -f cmake/Makefile plugin-realtime NAME=PhysicsEngine

# Plugin event-driven
make -f cmake/Makefile plugin-event NAME=NotificationSystem

# Plugin complet (realtime + events)
make -f cmake/Makefile plugin-full NAME=GameLogic
```

## Méthode 3: CMake Direct

```cmake
# generate.cmake
include(cmake/MCFPluginGenerator.cmake)

mcf_generate_plugin(
    NAME MyPlugin
    REALTIME
)
```

```bash
cmake -P generate.cmake
```

## Après Génération

### 1. Ajoutez le plugin au build

Éditez `plugins/CMakeLists.txt` et ajoutez:
```cmake
add_subdirectory(MyPlugin)
```

### 2. Implémentez votre logique

Éditez `plugins/MyPlugin/MyPlugin.cpp`:
```cpp
bool initialize(PluginContext& context) override {
    m_context = context;

    // Votre code ici
    std::cout << "[MyPlugin] Hello World!" << std::endl;

    m_initialized = true;
    return true;
}
```

### 3. Compilez

```bash
cd build
cmake ..
make -j$(nproc)
```

Votre plugin sera dans `build/plugins/myplugin.so`!

## Options Avancées

### Avec version et auteur

```bash
./cmake/create-plugin.sh \
    -n AudioPlugin \
    -v 2.0.0 \
    -a "Audio Team" \
    -d "Audio processing system"
```

### Avec priorité de chargement

```bash
./cmake/create-plugin.sh -n CorePlugin -p 500
```

Les priorités plus élevées sont chargées en premier (défaut: 100).

### Plugin realtime (mise à jour chaque frame)

```bash
./cmake/create-plugin.sh -n PhysicsPlugin -r
```

Ajoute la méthode `onRealtimeUpdate(float deltaTime)`.

### Plugin event-driven (réagit aux événements)

```bash
./cmake/create-plugin.sh -n LoggerPlugin -e
```

Ajoute la méthode `onEvent(const Event& event)`.

### Plugin complet

```bash
./cmake/create-plugin.sh -n GamePlugin -r -e -p 300
```

Combine realtime + events.

## Exemples Pratiques

### Plugin de Physique

```bash
./cmake/create-plugin.sh \
    -n PhysicsEngine \
    -v 1.0.0 \
    -a "Physics Team" \
    -d "2D physics simulation" \
    -p 400 \
    -r
```

Génère un plugin avec:
- Interface `IRealtimeUpdatable` pour la simulation physique
- Priorité élevée (400) pour charger avant le gameplay
- Métadonnées complètes

### Plugin d'Analytics

```bash
./cmake/create-plugin.sh \
    -n Analytics \
    -d "Collects and reports user analytics" \
    -e
```

Génère un plugin event-driven pour collecter les événements.

### Plugin Réseau

```bash
./cmake/create-plugin.sh \
    -n NetworkManager \
    -v 2.0.0 \
    -r -e \
    -p 500
```

Génère un plugin complet avec realtime (polling réseau) et events (messages reçus).

## Aide

```bash
./cmake/create-plugin.sh --help
make -f cmake/Makefile help
```

## Documentation Complète

- [PLUGIN_GENERATOR.md](PLUGIN_GENERATOR.md) - Guide complet
- [README.md](README.md) - Documentation du système cmake
- [examples/](examples/) - Exemples avancés

## Dépannage

### "Permission denied"

```bash
chmod +x ./cmake/create-plugin.sh
```

### Plugin ne compile pas

Vérifiez que vous avez bien ajouté le plugin à `plugins/CMakeLists.txt`:
```cmake
add_subdirectory(MyPlugin)
```

### Symboles non exportés

Vérifiez que `MCF_PLUGIN_EXPORT(mcf::MyPlugin)` est présent à la fin du fichier cpp.

## C'est Tout!

Vous êtes maintenant prêt à créer des plugins MCF. Bonne chance! 🚀

---

# Applications

Créez une nouvelle application MCF complète en quelques secondes!

## Méthode 1: Script Shell (Le plus simple)

```bash
# Depuis la racine du projet
./cmake/create-application.sh -n MyApp
```

C'est tout! Votre application est générée dans `MyApp/`.

## Méthode 2: Makefile (Raccourcis pratiques)

```bash
# Application basique
make -f cmake/Makefile app NAME=MyApp

# Application realtime (avec update loop)
make -f cmake/Makefile app-realtime NAME=MyGame

# Application complète (realtime + events + config + modules)
make -f cmake/Makefile app-full NAME=MyGame
```

## Méthode 3: CMake Direct

```cmake
# generate.cmake
include(cmake/MCFApplicationGenerator.cmake)

mcf_generate_application(
    NAME MyApp
    REALTIME
    CONFIG
)
```

```bash
cmake -P generate.cmake
```

## Après Génération

### 1. Naviguez dans le répertoire

```bash
cd MyApp
```

### 2. Compilez

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 3. Exécutez

```bash
./bin/myapp
```

Votre application est maintenant en cours d'exécution!

## Options Avancées

### Avec modules

```bash
./cmake/create-application.sh \
    -n MyGame \
    -m logger,profiling \
    -r -c
```

Génère une application avec:
- Modules logger et profiling intégrés
- Boucle de mise à jour realtime
- Fichier de configuration config.json

### Application de serveur

```bash
./cmake/create-application.sh \
    -n GameServer \
    -m networking,logger \
    -e -c
```

Génère une application avec:
- Module networking pour TCP
- Module logger
- Architecture événementielle
- Configuration JSON

### Application complète

```bash
./cmake/create-application.sh \
    -n FullGame \
    -v 2.0.0 \
    -a "Game Team" \
    -d "My awesome game" \
    -m logger,profiling,networking,realtime \
    -r -e -c
```

Génère une application avec toutes les options!

## Aide

```bash
./cmake/create-application.sh --help
make -f cmake/Makefile help-app
```

## Documentation Complète

- [APPLICATION_GENERATOR.md](APPLICATION_GENERATOR.md) - Guide complet
- [README.md](README.md) - Documentation du système cmake

---

C'est Tout!

Vous êtes maintenant prêt à créer des plugins ET des applications MCF. Bonne chance! 🚀

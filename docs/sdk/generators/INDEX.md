# CMake System - Index des Fichiers

Index complet de tous les fichiers du système CMake de ModularCppFramework.

## 📂 Structure des Fichiers

```
cmake/
├── 📖 Documentation
│   ├── README.md                      - Documentation principale
│   ├── QUICKSTART.md                  - Guide de démarrage rapide (30 sec)
│   ├── PLUGIN_GENERATOR.md            - Guide complet du générateur
│   └── INDEX.md                       - Ce fichier
│
├── 🛠️ Outils de Développement
│   ├── create-plugin.sh               - Script shell pour générer des plugins
│   ├── Makefile                       - Raccourcis make pour la génération
│   └── package-headers.sh             - Script de packaging des headers
│
├── 📦 Système de Packaging
│   ├── MCFPackaging.cmake             - Fonctions de packaging d'applications
│   ├── ModularCppFrameworkConfig.cmake.in        - Config pour find_package()
│   └── ModularCppFrameworkConfigVersion.cmake.in - Version sémantique
│
├── 🔧 Générateur de Plugins
│   ├── MCFPluginGenerator.cmake       - Fonctions CMake de génération
│   └── templates/                     - Templates de génération
│       ├── Plugin.cpp.in              - Template code source
│       ├── PluginCMakeLists.txt.in    - Template CMakeLists.txt
│       └── PluginREADME.md.in         - Template README
│
└── 📚 Exemples
    └── examples/
        ├── README.md                  - Documentation des exemples
        └── generate-plugin-example.cmake - Exemples de génération
```

## 📖 Guides de Documentation

### Pour les Utilisateurs

1. **[QUICKSTART.md](QUICKSTART.md)** - Commencer en 30 secondes
   - Création rapide de plugins
   - Exemples de base
   - Commandes essentielles

2. **[README.md](README.md)** - Documentation complète
   - Vue d'ensemble du système
   - Guide d'utilisation du packaging
   - Guide d'utilisation du générateur de plugins

3. **[PLUGIN_GENERATOR.md](PLUGIN_GENERATOR.md)** - Guide détaillé du générateur
   - Toutes les options disponibles
   - Types de plugins (basic, realtime, event-driven, full)
   - Exemples avancés
   - Troubleshooting
   - Personnalisation des templates

### Pour les Développeurs

4. **[examples/README.md](examples/README.md)** - Exemples d'utilisation
   - Utilisation depuis CMake
   - Utilisation depuis shell
   - Personnalisation des templates

## 🛠️ Outils Disponibles

### Générateur de Plugins

| Outil | Type | Usage | Documentation |
|-------|------|-------|---------------|
| `create-plugin.sh` | Shell Script | `./cmake/create-plugin.sh -n MyPlugin -r` | [QUICKSTART.md](QUICKSTART.md) |
| `Makefile` | Make Targets | `make -f cmake/Makefile plugin-realtime NAME=MyPlugin` | [PLUGIN_GENERATOR.md](PLUGIN_GENERATOR.md) |
| `MCFPluginGenerator.cmake` | CMake Functions | `mcf_generate_plugin(NAME MyPlugin REALTIME)` | [PLUGIN_GENERATOR.md](PLUGIN_GENERATOR.md) |

### Packaging

| Outil | Type | Usage | Documentation |
|-------|------|-------|---------------|
| `MCFPackaging.cmake` | CMake Functions | `mcf_package_application(...)` | [README.md](README.md) |
| `package-headers.sh` | Shell Script | Packaging des headers | [README.md](README.md) |

## 📦 Templates Disponibles

Situés dans `templates/`, utilisés par le générateur de plugins:

| Template | Description | Variables |
|----------|-------------|-----------|
| `Plugin.cpp.in` | Code source du plugin | `@PLUGIN_NAME@`, `@PLUGIN_VERSION@`, etc. |
| `PluginCMakeLists.txt.in` | Configuration CMake | `@PLUGIN_NAME@`, `@PLUGIN_NAME_LOWER@` |
| `PluginREADME.md.in` | Documentation du plugin | Toutes les variables de métadonnées |

## 🎯 Cas d'Usage par Outil

### Créer un Nouveau Plugin

**Débutant:**
```bash
./cmake/create-plugin.sh -n MyPlugin
```

**Utilisateur Make:**
```bash
make -f cmake/Makefile plugin-realtime NAME=PhysicsEngine
```

**Expert CMake:**
```cmake
include(cmake/MCFPluginGenerator.cmake)
mcf_generate_plugin(NAME MyPlugin VERSION 2.0.0 REALTIME EVENT_DRIVEN)
```

### Packager une Application

```cmake
include(cmake/MCFPackaging.cmake)

mcf_package_application(
    TARGET my_app
    VERSION 1.0.0
    OUTPUT_NAME "MyApplication"
    PLUGINS my_plugin
    CONFIG_FILES config/app.json
)
```

### Générer Plusieurs Plugins

**Via Makefile:**
```bash
make -f cmake/Makefile examples
```

**Via CMake:**
```bash
cmake -P cmake/examples/generate-plugin-example.cmake
```

## 📊 Statistiques

- **Total de fichiers:** 15
- **Lignes de code:** ~1,832
- **Templates:** 3
- **Guides:** 4
- **Scripts:** 2
- **Fonctions CMake:** 2 fichiers principaux

## 🔗 Liens Externes

- [Documentation MCF](../docs/)
- [Guide de développement de plugins](../docs/sdk/PLUGIN_GUIDE.md)
- [Guide de packaging](../docs/development/PACKAGING.md)

## ⚡ Actions Rapides

| Je veux... | Commande |
|-----------|----------|
| Créer un plugin basique | `./cmake/create-plugin.sh -n MyPlugin` |
| Créer un plugin realtime | `./cmake/create-plugin.sh -n MyPlugin -r` |
| Créer un plugin event-driven | `./cmake/create-plugin.sh -n MyPlugin -e` |
| Créer un plugin complet | `./cmake/create-plugin.sh -n MyPlugin -r -e` |
| Voir les options | `./cmake/create-plugin.sh --help` |
| Voir les exemples | `make -f cmake/Makefile help` |

## 📝 Notes de Version

- **v1.0** (2025-10-18) - Système initial de génération de plugins
  - Script shell interactif
  - Templates configurables
  - Support IRealtimeUpdatable et IEventDriven
  - Documentation complète
  - Exemples et Makefile helpers

# CMake System - Index des Fichiers

Index complet de tous les fichiers du système CMake de ModularCppFramework.

## 📂 Structure des Fichiers

```
ModularCppFramework/
├── 📖 Documentation
│   └── docs/sdk/generators/
│       ├── README.md                      - Documentation principale
│       ├── QUICKSTART.md                  - Guide de démarrage rapide (30 sec)
│       ├── PLUGIN_GENERATOR.md            - Guide complet du générateur de plugins
│       ├── APPLICATION_GENERATOR.md       - Guide complet du générateur d'applications
│       └── INDEX.md                       - Ce fichier
│
├── 🛠️ Scripts Python (Cross-platform)
│   └── tools/
│       ├── create-plugin.py               - Générateur de plugins (Python 3.6+)
│       ├── create-application.py          - Générateur d'applications (Python 3.6+)
│       └── package-application.py         - Outil de packaging (Python 3.6+)
│
├── 🔧 Système CMake
│   └── cmake/
│       ├── MCFPluginGenerator.cmake       - Fonctions CMake pour plugins
│       ├── MCFApplicationGenerator.cmake  - Fonctions CMake pour applications
│       ├── MCFPackaging.cmake             - Fonctions de packaging
│       └── templates/                     - Templates de génération
│           ├── Plugin.cpp.in              - Template plugin source
│           ├── PluginCMakeLists.txt.in    - Template plugin CMakeLists
│           ├── PluginREADME.md.in         - Template plugin README
│           ├── Application_*.in           - Templates application
│           └── ...
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
| `create-plugin.py` | Python Script (Cross-platform) | `python3 tools/create-plugin.py -n MyPlugin -r` | [QUICKSTART.md](QUICKSTART.md) |
| `MCFPluginGenerator.cmake` | CMake Functions | `mcf_generate_plugin(NAME MyPlugin REALTIME)` | [PLUGIN_GENERATOR.md](PLUGIN_GENERATOR.md) |

### Générateur d'Applications

| Outil | Type | Usage | Documentation |
|-------|------|-------|---------------|
| `create-application.py` | Python Script (Cross-platform) | `python3 tools/create-application.py -n MyApp -r` | [QUICKSTART.md](QUICKSTART.md) |
| `MCFApplicationGenerator.cmake` | CMake Functions | `mcf_generate_application(NAME MyApp REALTIME)` | [APPLICATION_GENERATOR.md](APPLICATION_GENERATOR.md) |

### Packaging

| Outil | Type | Usage | Documentation |
|-------|------|-------|---------------|
| `package-application.py` | Python Script (Cross-platform) | `python3 tools/package-application.py -t package-my_app` | [README.md](README.md) |
| `MCFPackaging.cmake` | CMake Functions | `mcf_package_application(...)` | [README.md](README.md) |

## 📦 Templates Disponibles

Situés dans `templates/`, utilisés par le générateur de plugins:

| Template | Description | Variables |
|----------|-------------|-----------|
| `Plugin.cpp.in` | Code source du plugin | `@PLUGIN_NAME@`, `@PLUGIN_VERSION@`, etc. |
| `PluginCMakeLists.txt.in` | Configuration CMake | `@PLUGIN_NAME@`, `@PLUGIN_NAME_LOWER@` |
| `PluginREADME.md.in` | Documentation du plugin | Toutes les variables de métadonnées |

## 🎯 Cas d'Usage par Outil

### Créer un Nouveau Plugin

**Débutant (Cross-platform):**
```bash
# Linux/macOS
python3 tools/create-plugin.py -n MyPlugin

# Windows
python tools/create-plugin.py -n MyPlugin
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

### Créer une Nouvelle Application

**Débutant (Cross-platform):**
```bash
# Linux/macOS
python3 tools/create-application.py -n MyApp -r -c

# Windows
python tools/create-application.py -n MyApp -r -c
```

**Expert CMake:**
```cmake
include(cmake/MCFApplicationGenerator.cmake)
mcf_generate_application(NAME MyApp REALTIME CONFIG)
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

| Je veux... | Commande (Linux/macOS) | Commande (Windows) |
|-----------|----------------------|-------------------|
| Créer un plugin basique | `python3 tools/create-plugin.py -n MyPlugin` | `python tools/create-plugin.py -n MyPlugin` |
| Créer un plugin realtime | `python3 tools/create-plugin.py -n MyPlugin -r` | `python tools/create-plugin.py -n MyPlugin -r` |
| Créer un plugin event-driven | `python3 tools/create-plugin.py -n MyPlugin -e` | `python tools/create-plugin.py -n MyPlugin -e` |
| Créer un plugin complet | `python3 tools/create-plugin.py -n MyPlugin -r -e` | `python tools/create-plugin.py -n MyPlugin -r -e` |
| Créer une application | `python3 tools/create-application.py -n MyApp` | `python tools/create-application.py -n MyApp` |
| Packager une application | `python3 tools/package-application.py -t package-my_app` | `python tools/package-application.py -t package-my_app` |
| Voir les options | `python3 tools/create-plugin.py --help` | `python tools/create-plugin.py --help` |

## 📝 Notes de Version

- **v1.0** (2025-10-18) - Système initial de génération de plugins
  - Script shell interactif
  - Templates configurables
  - Support IRealtimeUpdatable et IEventDriven
  - Documentation complète
  - Exemples et Makefile helpers

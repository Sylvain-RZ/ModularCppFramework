# MCF Generators - Guide Utilisateur

Système complet de génération automatique de plugins et d'applications pour ModularCppFramework.

> **Note:** Cette documentation fait partie du SDK ModularCppFramework.
> Elle est destinée aux développeurs utilisant MCF pour créer des applications et plugins.

## 📚 Guides Disponibles

### Démarrage Rapide
- **[QUICKSTART.md](QUICKSTART.md)** - Créer un plugin ou une application en 30 secondes

### Guides Complets
- **[PLUGIN_GENERATOR.md](PLUGIN_GENERATOR.md)** - Guide détaillé du générateur de plugins
- **[APPLICATION_GENERATOR.md](APPLICATION_GENERATOR.md)** - Guide détaillé du générateur d'applications

### Référence
- **[INDEX.md](INDEX.md)** - Index de tous les fichiers du système
- **[SUMMARY.txt](SUMMARY.txt)** - Récapitulatif texte complet

## 🚀 Utilisation Rapide

### Créer un Plugin

```bash
# Linux/macOS
python3 tools/create-plugin.py -n MyPlugin -r

# Windows
python tools/create-plugin.py -n MyPlugin -r
```

### Créer une Application

```bash
# Linux/macOS
python3 tools/create-application.py -n MyApp -r -c -m logger

# Windows
python tools/create-application.py -n MyApp -r -c -m logger
```

### Packager une Application

```bash
# Linux/macOS
python3 tools/package-application.py -t package-my_app --extract --test

# Windows
python tools/package-application.py -t package-my_app --extract --test
```

## 📁 Organisation

```
ModularCppFramework/
├── tools/                              # Scripts Python cross-platform
│   ├── create-plugin.py                # Générateur de plugins (Python 3.6+)
│   ├── create-application.py           # Générateur d'applications (Python 3.6+)
│   └── package-application.py          # Outil de packaging (Python 3.6+)
├── cmake/                              # Système CMake
│   ├── MCFPluginGenerator.cmake        # Fonctions CMake plugins
│   ├── MCFApplicationGenerator.cmake   # Fonctions CMake applications
│   ├── MCFPackaging.cmake              # Fonctions CMake packaging
│   └── templates/                      # Templates de génération
└── docs/sdk/generators/                # Documentation (ce dossier)
    ├── README.md                       # Ce fichier
    ├── QUICKSTART.md                   # Démarrage rapide
    ├── PLUGIN_GENERATOR.md             # Guide plugins
    ├── APPLICATION_GENERATOR.md        # Guide applications
    ├── INDEX.md                        # Index complet
    └── SUMMARY.txt                     # Récapitulatif
```

## 🔧 Types de Génération

### Plugins

| Type | Description | Commande (Linux/macOS) |
|------|-------------|----------------------|
| Basique | IPlugin seulement | `python3 tools/create-plugin.py -n MyPlugin` |
| Realtime | + IRealtimeUpdatable | `python3 tools/create-plugin.py -n MyPlugin -r` |
| Event-Driven | + IEventDriven | `python3 tools/create-plugin.py -n MyPlugin -e` |
| Full | + Realtime + Events | `python3 tools/create-plugin.py -n MyPlugin -r -e` |

### Applications

| Type | Description | Commande (Linux/macOS) |
|------|-------------|----------------------|
| Basique | Structure minimale | `python3 tools/create-application.py -n MyApp` |
| Realtime | + Update loop | `python3 tools/create-application.py -n MyApp -r` |
| Event-Driven | + Events | `python3 tools/create-application.py -n MyApp -e` |
| Full | + All features | `python3 tools/create-application.py -n MyApp -r -e -c -m logger` |

## 📖 Modules Disponibles

Pour les applications, les modules suivants peuvent être inclus:

- **logger** - Système de logging
- **networking** - TCP client/server
- **profiling** - Métriques de performance
- **realtime** - Fixed timestep updates

Usage: `-m logger,networking,profiling`

## 💡 Exemples

### Plugin de Physique

```bash
# Linux/macOS
python3 tools/create-plugin.py \
    -n PhysicsEngine \
    -v 1.0.0 \
    -a "Physics Team" \
    -d "2D physics simulation" \
    -p 400 \
    -r

# Windows
python tools/create-plugin.py -n PhysicsEngine -v 1.0.0 -a "Physics Team" -d "2D physics simulation" -p 400 -r
```

### Jeu Complet

```bash
# Linux/macOS
python3 tools/create-application.py \
    -n SimpleGame \
    -v 1.0.0 \
    -a "Game Team" \
    -d "A simple 2D game" \
    -m logger,profiling,realtime \
    -r -c

# Windows
python tools/create-application.py -n SimpleGame -v 1.0.0 -a "Game Team" -d "A simple 2D game" -m logger,profiling,realtime -r -c
```

### Serveur Network

```bash
# Linux/macOS
python3 tools/create-application.py \
    -n GameServer \
    -v 2.0.0 \
    -a "Server Team" \
    -m logger,networking \
    -e -c

# Windows
python tools/create-application.py -n GameServer -v 2.0.0 -a "Server Team" -m logger,networking -e -c
```

## 🎁 Packaging d'Applications

Une fois votre application développée, utilisez le script de packaging pour créer des archives distribuables:

### Utilisation Basique

```bash
# Linux/macOS
python3 tools/package-application.py -t package-my_app

# Packager tous les exemples MCF
python3 tools/package-application.py -t package-mcf-examples

# Windows
python tools/package-application.py -t package-my_app
```

### Options Avancées

```bash
# Linux/macOS - Package avec clean build
python3 tools/package-application.py -t package-my_app --clean

# Package, extraction et tests automatiques
python3 tools/package-application.py -t package-my_app --extract --test

# Package et copie vers un répertoire de distribution
python3 tools/package-application.py -t package-my_app -o /path/to/dist

# Package avec configuration spécifique
python3 tools/package-application.py -t package-my_app -c Debug -j 4

# Windows
python tools/package-application.py -t package-my_app --clean
```

### Résultat

Le script génère une archive `.tar.gz` contenant:
- Exécutables compilés (dans `bin/`)
- Plugins (dans `plugins/`)
- Fichiers de configuration (dans `config/`)
- Ressources (dans `resources/`)
- README.txt avec instructions d'utilisation

Exemple: `MyApp-1.0.0-Linux-x86_64.tar.gz`

## 🆘 Aide

```bash
# Linux/macOS
python3 tools/create-plugin.py --help
python3 tools/create-application.py --help
python3 tools/package-application.py --help

# Windows
python tools/create-plugin.py --help
python tools/create-application.py --help
python tools/package-application.py --help
```

## 📝 Workflow Complet

1. **Créer une application:**
   ```bash
   # Linux/macOS
   python3 tools/create-application.py -n MyGame -r -c -m logger,profiling

   # Windows
   python tools/create-application.py -n MyGame -r -c -m logger,profiling
   ```

2. **Créer des plugins:**
   ```bash
   # Linux/macOS
   python3 tools/create-plugin.py -n PhysicsPlugin -r
   python3 tools/create-plugin.py -n AudioPlugin -r

   # Windows
   python tools/create-plugin.py -n PhysicsPlugin -r
   python tools/create-plugin.py -n AudioPlugin -r
   ```

3. **Compiler et exécuter:**
   ```bash
   cd MyGame
   mkdir build && cd build
   cmake ..
   make -j$(nproc)    # Linux/macOS
   # ou
   cmake --build . --config Release    # Windows
   ./bin/mygame
   ```

4. **Packager pour distribution:**
   ```bash
   # Linux/macOS - Depuis la racine de MyGame
   python3 ../tools/package-application.py -t package-mygame --extract --test

   # Windows
   python ..\tools\package-application.py -t package-mygame --extract --test

   # Résultat: MyGame-1.0.0-Linux-x86_64.tar.gz (Linux) ou MyGame-1.0.0-Windows-x86_64.zip (Windows)
   ```

## 🔗 Liens Utiles

- [Documentation SDK](../) - Documentation principale MCF SDK
- [Guide de développement de plugins](../PLUGIN_GUIDE.md) - Guide complet plugins MCF
- [Architecture MCF](../ARCHITECTURE.md) - Détails architecture du framework
- [Exemples](../EXAMPLES.md) - Exemples d'utilisation MCF
- [Système CMake](../../../cmake/) - Détails du système CMake

---

**Note:** Cette documentation fait partie du SDK ModularCppFramework destiné aux développeurs.

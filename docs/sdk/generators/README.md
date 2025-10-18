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
# Depuis la racine du projet
./tools/create-plugin.sh -n MyPlugin -r
```

### Créer une Application

```bash
# Depuis la racine du projet
./tools/create-application.sh -n MyApp -r -c -m logger
```

### Packager une Application

```bash
# Depuis la racine du projet
./tools/package-application.sh -t package-my_app --extract --test
```

### Via Makefile

```bash
make -f cmake/Makefile plugin-realtime NAME=Physics
make -f cmake/Makefile app-full NAME=MyGame
```

## 📁 Organisation

```
ModularCppFramework/
├── tools/                              # Scripts de génération
│   ├── create-plugin.sh                # Générateur de plugins
│   ├── create-application.sh           # Générateur d'applications
│   └── package-application.sh          # Outil de packaging
├── cmake/                              # Système CMake
│   ├── MCFPluginGenerator.cmake        # Fonctions plugins
│   ├── MCFApplicationGenerator.cmake   # Fonctions applications
│   ├── MCFPackaging.cmake              # Fonctions packaging
│   ├── templates/                      # Templates de génération
│   └── Makefile                        # Raccourcis make
└── docs/development/generators/        # Documentation (ce dossier)
    ├── README.md                       # Ce fichier
    ├── QUICKSTART.md                   # Démarrage rapide
    ├── PLUGIN_GENERATOR.md             # Guide plugins
    ├── APPLICATION_GENERATOR.md        # Guide applications
    ├── INDEX.md                        # Index complet
    └── SUMMARY.txt                     # Récapitulatif
```

## 🔧 Types de Génération

### Plugins

| Type | Description | Commande |
|------|-------------|----------|
| Basique | IPlugin seulement | `./tools/create-plugin.sh -n MyPlugin` |
| Realtime | + IRealtimeUpdatable | `./tools/create-plugin.sh -n MyPlugin -r` |
| Event-Driven | + IEventDriven | `./tools/create-plugin.sh -n MyPlugin -e` |
| Full | + Realtime + Events | `./tools/create-plugin.sh -n MyPlugin -r -e` |

### Applications

| Type | Description | Commande |
|------|-------------|----------|
| Basique | Structure minimale | `./tools/create-application.sh -n MyApp` |
| Realtime | + Update loop | `./tools/create-application.sh -n MyApp -r` |
| Event-Driven | + Events | `./tools/create-application.sh -n MyApp -e` |
| Full | + All features | `./tools/create-application.sh -n MyApp -r -e -c -m logger` |

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
./tools/create-plugin.sh \
    -n PhysicsEngine \
    -v 1.0.0 \
    -a "Physics Team" \
    -d "2D physics simulation" \
    -p 400 \
    -r
```

### Jeu Complet

```bash
./tools/create-application.sh \
    -n SimpleGame \
    -v 1.0.0 \
    -a "Game Team" \
    -d "A simple 2D game" \
    -m logger,profiling,realtime \
    -r -c
```

### Serveur Network

```bash
./tools/create-application.sh \
    -n GameServer \
    -v 2.0.0 \
    -a "Server Team" \
    -m logger,networking \
    -e -c
```

## 🎁 Packaging d'Applications

Une fois votre application développée, utilisez le script de packaging pour créer des archives distribuables:

### Utilisation Basique

```bash
# Packager une application
./tools/package-application.sh -t package-my_app

# Packager tous les exemples MCF
./tools/package-application.sh -t package-mcf-examples
```

### Options Avancées

```bash
# Package avec clean build
./tools/package-application.sh -t package-my_app --clean

# Package, extraction et tests automatiques
./tools/package-application.sh -t package-my_app --extract --test

# Package et copie vers un répertoire de distribution
./tools/package-application.sh -t package-my_app -o /path/to/dist

# Package avec configuration spécifique
./tools/package-application.sh -t package-my_app -c Debug -j 4
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
# Aide plugins
./tools/create-plugin.sh --help

# Aide applications
./tools/create-application.sh --help

# Aide packaging
./tools/package-application.sh --help

# Aide Makefile
make -f cmake/Makefile help
make -f cmake/Makefile help-plugin
make -f cmake/Makefile help-app
```

## 📝 Workflow Complet

1. **Créer une application:**
   ```bash
   ./tools/create-application.sh -n MyGame -r -c -m logger,profiling
   ```

2. **Créer des plugins:**
   ```bash
   ./tools/create-plugin.sh -n PhysicsPlugin -r
   ./tools/create-plugin.sh -n AudioPlugin -r
   ```

3. **Compiler et exécuter:**
   ```bash
   cd MyGame
   mkdir build && cd build
   cmake ..
   make -j$(nproc)
   ./bin/mygame
   ```

4. **Packager pour distribution:**
   ```bash
   # Depuis la racine de MyGame
   ../tools/package-application.sh -t package-mygame --extract --test

   # Résultat: MyGame-1.0.0-Linux-x86_64.tar.gz
   ```

## 🔗 Liens Utiles

- [Documentation SDK](../) - Documentation principale MCF SDK
- [Guide de développement de plugins](../PLUGIN_GUIDE.md) - Guide complet plugins MCF
- [Architecture MCF](../ARCHITECTURE.md) - Détails architecture du framework
- [Exemples](../EXAMPLES.md) - Exemples d'utilisation MCF
- [Système CMake](../../../cmake/) - Détails du système CMake

---

**Note:** Cette documentation fait partie du SDK ModularCppFramework destiné aux développeurs.

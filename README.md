# ModularCppFramework - Production-Ready Modular C++ Application Framework

[![CI](https://github.com/Sylvain-RZ/ModularCppFramework/workflows/CI/badge.svg)](https://github.com/Sylvain-RZ/ModularCppFramework/actions)
[![Tests](https://img.shields.io/badge/tests-27%2F27%20passing-brightgreen)]()
[![Quality](https://img.shields.io/badge/quality-100%2F100-brightgreen)]()
[![Documentation](https://img.shields.io/badge/docs-100%25-brightgreen)]()
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)]()
[![License](https://img.shields.io/badge/license-MIT-blue)]()

Un framework C++17 header-only de qualité production pour créer des applications modulaires avec système de plugins dynamiques, hot-reload, et architecture event-driven.

## 🎯 Statut: Production-Ready v1.0.3

✅ **100% tests passent** (27/27) • ✅ **CI/CD configuré** • ✅ **Documentation complète** • ✅ **8 exemples fonctionnels** • 🆕 **Générateurs automatiques**

## ⚡ Caractéristiques Principales

### Core Features
- **Système de Plugins Dynamiques**: Chargement/déchargement à l'exécution (.so/.dll)
- **Résolution de Dépendances**: DAG avec détection de cycles et versioning sémantique
- **EventBus**: Communication découplée via publish/subscribe avec priorités
- **ServiceLocator**: Injection de dépendances (Singleton/Transient/Scoped)
- **ResourceManager**: Gestion centralisée avec cache et référence comptée
- **Module System**: Modules statiques pour fonctionnalités core

### Advanced Features
- **Hot-Reload**: Rechargement automatique de plugins avec FileWatcher
- **Configuration Manager**: Système JSON avec hot-reload et dot notation
- **Logger**: Système de logging flexible (console, file, rotating) avec niveaux
- **Thread-Safe**: Architecture "copy-under-lock" pour tous les systèmes
- **ThreadPool**: Exécution asynchrone de tâches
- **FileSystem**: Utilitaires cross-platform

### Built-in Modules
- **LoggerModule**: Intégration Logger + ConfigurationManager
- **RealtimeModule**: Boucle temps réel avec fixed timestep pour simulations
- **ProfilingModule**: Collecte de métriques de performance avec macros
- **NetworkingModule**: TCP client/server asynchrone avec callbacks

## 🚀 Démarrage Rapide

### Installation Rapide

```bash
# Option 1: Via Conan (Recommandé)
conan install modular-cpp-framework/1.0.3@

# Option 2: Via vcpkg
vcpkg install modular-cpp-framework

# Option 3: Compilation depuis les sources
git clone https://github.com/Sylvain-RZ/ModularCppFramework.git
cd ModularCppFramework
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Générateurs Automatiques - Créer une Application en 30 Secondes

**Nouveau!** MCF inclut des générateurs automatiques cross-platform pour créer rapidement plugins et applications:

```bash
# Créer une application complète en une commande
python3 tools/create-application.py -n MyGame -r -c -m logger,profiling

# Créer un plugin realtime en une commande
python3 tools/create-plugin.py -n PhysicsPlugin -r

# Sur Windows, utilisez 'python' au lieu de 'python3'
python tools/create-application.py -n MyGame -r -c -m logger,profiling
```

**Voir** → [Guide Quick Start Générateurs](docs/sdk/generators/QUICKSTART.md) pour plus de détails.

### Premier Programme (Manuel)

```cpp
#include <core/Application.hpp>
#include <modules/logger/LoggerModule.hpp>

class MyApp : public mcf::Application {
protected:
    bool onInitialize() override {
        // Charger configuration
        getConfigurationManager().loadFromFile("config.json");

        // Charger plugins
        getPluginManager().loadPluginsFromDirectory("./plugins");
        getPluginManager().initializeAll();

        return true;
    }
};

int main() {
    MyApp app;
    app.addModule<mcf::LoggerModule>();
    return app.run();
}
```

## 📚 Documentation

La documentation est organisée en deux catégories principales:

### Pour Utilisateurs du SDK

**Documentation complète** → [docs/sdk/](docs/sdk/)

#### Démarrage & Installation
- **[Guide Démarrage Rapide](docs/sdk/QUICK_START.md)** - Premiers pas en 5 minutes
- **[Guide d'Installation](docs/sdk/INSTALLATION.md)** - Toutes les options d'installation détaillées
- **[Guide d'Utilisation](docs/sdk/USAGE.md)** - Utilisation des composants principaux
- **[Exemples](docs/sdk/EXAMPLES.md)** - 8 exemples détaillés et fonctionnels

#### Générateurs Automatiques
- **[Générateurs](docs/sdk/generators/)** - 🚀 **Créer plugins et applications en 30 secondes**
  - [Quick Start Générateurs](docs/sdk/generators/QUICKSTART.md) - Démarrage rapide
  - [Générateur de Plugins](docs/sdk/generators/PLUGIN_GENERATOR.md) - Guide complet
  - [Générateur d'Applications](docs/sdk/generators/APPLICATION_GENERATOR.md) - Guide complet

#### Guides Techniques
- **[Architecture](docs/sdk/ARCHITECTURE.md)** - Architecture détaillée avec diagrammes
- **[Guide Plugins](docs/sdk/PLUGIN_GUIDE.md)** - Création de plugins step-by-step
- **[Hot-Reload](docs/sdk/HOT_RELOAD.md)** - Guide complet du hot-reload
- **[Configuration](docs/sdk/CONFIGURATION_GUIDE.md)** - Système de configuration JSON
- **[Packaging Applications](docs/sdk/APPLICATION_PACKAGING.md)** - Distribuer vos applications

### Pour Développeurs et Mainteneurs

**Documentation développement** → [docs/development/](docs/development/)

- **[Guide de Build](docs/development/BUILD.md)** - Instructions de compilation détaillées
- **[Implémentation](docs/development/IMPLEMENTATION.md)** - Détails techniques d'implémentation
- **[Couverture Tests](docs/development/TEST_COVERAGE.md)** - Stratégie de tests et couverture
- **[Tests des Outils](docs/development/TOOLS_TESTING.md)** - Tests des générateurs et scripts
- **[Packaging SDK](docs/development/PACKAGING.md)** - Distribution via Conan/vcpkg

## 🏗️ Structure du Projet

```
ModularCppFramework/
├── core/                        # Bibliothèque header-only (20 fichiers)
├── modules/                     # Modules statiques (4 modules)
│   ├── logger/                  # LoggerModule
│   ├── realtime/                # RealtimeModule
│   ├── profiling/               # ProfilingModule
│   └── networking/              # NetworkingModule
├── plugins/                     # Plugins dynamiques (exemples)
├── examples/                    # Applications exemple (8 exemples)
├── tests/                       # Tests (27 tests, 100% passent)
├── tools/                       # 🆕 Scripts de génération (Python)
│   ├── create-plugin.py         # Générateur de plugins
│   ├── create-application.py    # Générateur d'applications
│   ├── package-application.py   # Outil de packaging
│   └── README.md                # Documentation des outils
├── cmake/                       # 🆕 Système CMake & Templates
│   ├── MCFPluginGenerator.cmake      # Fonctions CMake plugins
│   ├── MCFApplicationGenerator.cmake # Fonctions CMake applications
│   ├── MCFPackaging.cmake            # Fonctions packaging
│   └── templates/               # Templates de génération
└── docs/                        # Documentation complète
    ├── sdk/                     # Documentation utilisateurs
    │   └── generators/          # 🆕 Doc générateurs
    └── development/             # Documentation développeurs
```

**Détails complets** → Voir [Structure Détaillée](docs/sdk/ARCHITECTURE.md#structure-du-projet)

## 💡 Cas d'Usage

- **Game Engines**: RealtimeModule + Hot-Reload pour game logic
- **Applications Modulaires**: Plugins pour extensions tierces
- **Microservices**: NetworkingModule + EventBus
- **Data Pipelines**: Plugins pour ingestion/transformation/export
- **Outils CLI Extensibles**: Système de plugins
- **Simulations**: Fixed timestep avec RealtimeModule

## 🔧 Prérequis

- CMake 3.16+
- Compilateur C++17 (GCC 7+, Clang 5+, MSVC 2017+)
- Linux: libdl, pthread
- Windows: Support natif

## 🧪 Tests et Qualité

```bash
# Exécuter tous les tests
cd build
cmake -DBUILD_TESTS=ON ..
make -j$(nproc)
ctest -V

# Résultat: 100% tests passed, 0 tests failed out of 27
```

**Statistiques:**
- 27 tests (100% passent)
- 17 tests unitaires
- 8 tests d'intégration
- 2 tests additionnels (configuration, logger)
- Couverture complète de tous les composants core

**Détails** → Voir [Test Coverage](docs/development/TEST_COVERAGE.md)

## 🏆 Pourquoi ModularCppFramework?

### Points Forts

1. **Header-Only Core**: Intégration ultra-simple
2. **Hot-Reload Production-Ready**: Rechargement sans redémarrage
3. **Architecture Moderne**: Design patterns C++17
4. **DI Complet**: ServiceLocator avec Scoped lifetime
5. **Documentation 100%**: Toutes les APIs documentées
6. **Tests Complets**: 100% de réussite
7. **Thread-Safe**: Architecture "copy-under-lock"
8. **CI/CD**: GitHub Actions multi-plateforme
9. **Multi-Platform**: Linux, Windows, macOS

### Comparaison

| Feature | MCF | Qt Plugin | POCO | Boost.Extension |
|---------|-----|-----------|------|-----------------|
| Header-Only Core | ✅ | ❌ | ❌ | ✅ |
| Hot-Reload | ✅ | ❌ | ❌ | ❌ |
| Dependency Resolution | ✅ DAG | ⚠️ Basic | ❌ | ❌ |
| DI avec Scoped | ✅ | ❌ | ✅ | ❌ |
| Documentation | ✅ 100% | ✅ | ✅ | ⚠️ |

## 🤝 Contribution

Les contributions sont les bienvenues!

1. Fork le repository
2. Créer une branche feature (`git checkout -b feature/AmazingFeature`)
3. Commit vos changements (`git commit -m 'Add AmazingFeature'`)
4. Push vers la branche (`git push origin feature/AmazingFeature`)
5. Ouvrir une Pull Request

**Guidelines** → Voir [CONTRIBUTING.md](CONTRIBUTING.md)

## 📖 Liens Utiles

- **Documentation Complète**: [docs/](docs/)
- **Guide Démarrage**: [docs/sdk/QUICK_START.md](docs/sdk/QUICK_START.md)
- **Exemples**: [docs/sdk/EXAMPLES.md](docs/sdk/EXAMPLES.md)
- **API Reference**: Générer avec `doxygen Doxyfile`
- **Issues**: [GitHub Issues](https://github.com/Sylvain-RZ/ModularCppFramework/issues)
- **Discussions**: [GitHub Discussions](https://github.com/Sylvain-RZ/ModularCppFramework/discussions)

## 📄 License

MIT License - Voir [LICENSE](LICENSE) pour détails

---

**ModularCppFramework** - Framework C++17 modulaire de qualité production
Made with ❤️ by the community • [⭐ Star on GitHub](https://github.com/Sylvain-RZ/ModularCppFramework)

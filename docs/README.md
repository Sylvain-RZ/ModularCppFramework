# Documentation ModularCppFramework

La documentation est organisée en **deux catégories** selon votre rôle:

---

## 📘 [Documentation SDK](sdk/) - Pour Utilisateurs

**Vous utilisez ModularCppFramework dans votre projet?** Consultez la documentation SDK.

Cette section contient tous les guides pour développer des applications avec MCF:

### Démarrage
- [**QUICK_START.md**](sdk/QUICK_START.md) - Guide de démarrage rapide (5 minutes)
- [**INSTALLATION.md**](sdk/INSTALLATION.md) - Installation détaillée (Conan, vcpkg, sources, intégration)
- [**USAGE.md**](sdk/USAGE.md) - Utilisation des composants principaux
- [**EXAMPLES.md**](sdk/EXAMPLES.md) - Documentation des 8 exemples fournis

### Générateurs Automatiques 🚀
- [**generators/**](sdk/generators/) - Système de génération automatique de plugins et applications
  - [**QUICKSTART.md**](sdk/generators/QUICKSTART.md) - Créer un plugin/app en 30 secondes
  - [**PLUGIN_GENERATOR.md**](sdk/generators/PLUGIN_GENERATOR.md) - Guide complet générateur de plugins
  - [**APPLICATION_GENERATOR.md**](sdk/generators/APPLICATION_GENERATOR.md) - Guide complet générateur d'applications
  - [**INDEX.md**](sdk/generators/INDEX.md) - Index complet du système

### Guides Techniques
- [**ARCHITECTURE.md**](sdk/ARCHITECTURE.md) - Comprendre l'architecture du framework
- [**PLUGIN_GUIDE.md**](sdk/PLUGIN_GUIDE.md) - Créer vos propres plugins
- [**HOT_RELOAD.md**](sdk/HOT_RELOAD.md) - Utiliser le système de hot-reload
- [**CONFIGURATION_GUIDE.md**](sdk/CONFIGURATION_GUIDE.md) - Système de configuration JSON

### Distribution
- [**APPLICATION_PACKAGING.md**](sdk/APPLICATION_PACKAGING.md) - Packager vos applications pour distribution

➡️ **[Accéder à la documentation SDK](sdk/)**

---

## 🔧 [Documentation Development](development/) - Pour Mainteneurs

**Vous contribuez au framework ModularCppFramework?** Consultez la documentation development.

Cette section contient les guides techniques pour maintenir et développer MCF:

- [**BUILD.md**](development/BUILD.md) - Guide de build complet (compilation, options, debugging, CI/CD)
- [**IMPLEMENTATION.md**](development/IMPLEMENTATION.md) - Détails techniques d'implémentation
- [**TEST_COVERAGE.md**](development/TEST_COVERAGE.md) - Stratégie de tests et couverture
- [**TOOLS_TESTING.md**](development/TOOLS_TESTING.md) - Tests des générateurs et scripts
- [**PACKAGING.md**](development/PACKAGING.md) - Packager et distribuer le SDK

➡️ **[Accéder à la documentation development](development/)**

---

## Quelle documentation dois-je consulter?

| Vous êtes... | Consultez... |
|--------------|--------------|
| 👨‍💻 Développeur utilisant MCF dans un projet | [**docs/sdk/**](sdk/) |
| 🎮 Utilisateur final d'une app MCF | Consultez la doc de l'application |
| 🔧 Contributeur / Mainteneur du framework | [**docs/development/**](development/) |
| 📦 Créateur de package Conan/vcpkg | [**docs/development/PACKAGING.md**](development/PACKAGING.md) |

---

## Génération de la Documentation API (Doxygen)

Pour générer la documentation complète de l'API:

```bash
# Depuis la racine du projet
doxygen Doxyfile

# Ouvrir dans un navigateur
xdg-open docs/doxygen/html/index.html
```

**Statut Doxygen**: ✅ 100% des APIs publiques documentées
- 21 fichiers d'en-tête core
- ~250+ méthodes documentées
- ~400+ tags @param
- ~200+ tags @return

---

## Structure de la Documentation

```
docs/
├── README.md                    # Ce fichier
├── sdk/                         # Pour utilisateurs du SDK
│   ├── README.md
│   ├── QUICK_START.md          # ⭐ Commencez ici!
│   ├── INSTALLATION.md         # Installation détaillée
│   ├── USAGE.md                # Utilisation des composants
│   ├── EXAMPLES.md
│   ├── generators/             # 🚀 Générateurs automatiques
│   │   ├── README.md
│   │   ├── QUICKSTART.md       # Créer plugin/app en 30s
│   │   ├── PLUGIN_GENERATOR.md
│   │   ├── APPLICATION_GENERATOR.md
│   │   └── INDEX.md
│   ├── ARCHITECTURE.md
│   ├── PLUGIN_GUIDE.md
│   ├── HOT_RELOAD.md
│   ├── CONFIGURATION_GUIDE.md
│   └── APPLICATION_PACKAGING.md
└── development/                # Pour mainteneurs
    ├── README.md
    ├── BUILD.md                # Guide de build
    ├── IMPLEMENTATION.md
    ├── TEST_COVERAGE.md
    ├── TOOLS_TESTING.md        # Tests des outils
    └── PACKAGING.md
```

---

## Liens Rapides

- **Documentation principale**: [../README.md](../README.md)
- **Exemples de code**: [../examples/](../examples/)
- **Tests**: [../tests/](../tests/)
- **Code source**: [../core/](../core/) et [../modules/](../modules/)

---

**Bon développement avec ModularCppFramework! 🚀**

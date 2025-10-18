# Documentation SDK - Pour Utilisateurs

Cette documentation est destinée aux **développeurs utilisant ModularCppFramework** dans leurs projets.

## 🚀 Démarrage Rapide

**Nouveau dans MCF?** Commencez par:
1. **[QUICK_START.md](QUICK_START.md)** - Guide de démarrage rapide (5 minutes)
2. **[generators/QUICKSTART.md](generators/QUICKSTART.md)** - Créer un plugin/app en 30 secondes
3. **[EXAMPLES.md](EXAMPLES.md)** - Explorer les 8 exemples fournis

## Guides de Démarrage

- **[QUICK_START.md](QUICK_START.md)** - Guide de démarrage rapide (5 minutes)
- **[INSTALLATION.md](INSTALLATION.md)** - Guide d'installation détaillé (Conan, vcpkg, sources, intégration)
- **[USAGE.md](USAGE.md)** - Guide d'utilisation des composants principaux
- **[EXAMPLES.md](EXAMPLES.md)** - Documentation des 8 exemples fournis

## 🛠️ Outils de Génération Automatique

**Le moyen le plus rapide de créer des plugins et applications MCF!**

- **[generators/](generators/)** - 🚀 Système complet de générateurs
  - [**QUICKSTART.md**](generators/QUICKSTART.md) - Créer un plugin/app en 30 secondes ⭐
  - [**PLUGIN_GENERATOR.md**](generators/PLUGIN_GENERATOR.md) - Guide complet générateur de plugins
  - [**APPLICATION_GENERATOR.md**](generators/APPLICATION_GENERATOR.md) - Guide complet générateur d'applications
  - [**INDEX.md**](generators/INDEX.md) - Index de tous les fichiers du système

**Exemples rapides:**
```bash
# Créer un plugin realtime
./tools/create-plugin.sh -n PhysicsPlugin -r

# Créer une application complète
./tools/create-application.sh -n MyGame -r -c -m logger,profiling
```

## Guides Techniques

- **[ARCHITECTURE.md](ARCHITECTURE.md)** - Architecture du framework avec diagrammes
- **[PLUGIN_GUIDE.md](PLUGIN_GUIDE.md)** - Guide complet de création de plugins (manuel)
- **[HOT_RELOAD.md](HOT_RELOAD.md)** - Guide du système de hot-reload
- **[CONFIGURATION_GUIDE.md](CONFIGURATION_GUIDE.md)** - Système de configuration JSON

## Guides de Distribution

- **[APPLICATION_PACKAGING.md](APPLICATION_PACKAGING.md)** - Packager vos applications pour distribution

---

**Navigation:**
- ⬆️ [Documentation principale](../) - Retour à l'index docs/
- 🔧 [Documentation développement](../development/) - Pour contributeurs du framework
- 🏠 [README principal](../../README.md) - Retour au README racine

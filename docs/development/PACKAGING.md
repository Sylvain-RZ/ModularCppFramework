# Guide de Packaging SDK - ModularCppFramework

Ce guide explique comment **packager et distribuer le framework MCF** (SDK) pour que d'autres développeurs puissent l'utiliser.

> **Note**: Pour packager vos **applications** (binaires pour utilisateurs finaux), consultez [APPLICATION_PACKAGING.md](../sdk/APPLICATION_PACKAGING.md).

## Table des Matières

1. [Packager le Framework SDK](#1-packager-le-framework-sdk)
2. [Installation Système](#2-installation-système)
3. [Gestionnaires de Paquets](#3-gestionnaires-de-paquets)
4. [Distribution et Déploiement](#4-distribution-et-déploiement)

---

## 1. Packager le Framework SDK

Le framework MCF peut être distribué de plusieurs façons pour permettre aux développeurs de l'utiliser dans leurs projets.

### 1.1 Package Release (Recommandé)

Créez une archive standalone contenant tout le nécessaire pour utiliser MCF.

**Commande:**

```bash
cd ModularCppFramework
mkdir build && cd build

# Configuration pour release
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTS=OFF \
      -DBUILD_EXAMPLES=OFF \
      ..

# Compilation
make -j$(nproc)

# Création du package SDK
make package-release
```

**Résultat:**
```
build/modular-cpp-framework-1.0.2-Linux-x86_64.tar.gz
```

**Contenu du package SDK:**
```
modular-cpp-framework-1.0.2/
├── include/mcf/
│   ├── core/              # Headers core (header-only)
│   └── modules/           # Headers modules
├── lib/                   # Bibliothèques compilées
│   ├── libmcf_logger_module.a
│   ├── libmcf_networking_module.a
│   ├── libmcf_profiling_module.a
│   └── libmcf_realtime_module.a
├── cmake/                 # Fichiers CMake pour find_package()
│   ├── ModularCppFrameworkConfig.cmake
│   ├── ModularCppFrameworkConfigVersion.cmake
│   └── MCFPackaging.cmake  # Utilitaires de packaging d'applications
├── docs/                  # Documentation complète (10 guides)
├── README.md
├── LICENSE
└── CHANGELOG.md
```

**Utilisation par les développeurs:**

Ils peuvent décompresser le package et l'utiliser dans leur projet:

```cmake
# Dans leur CMakeLists.txt
list(APPEND CMAKE_PREFIX_PATH "/chemin/vers/modular-cpp-framework-1.0.2")
find_package(ModularCppFramework 1.0 REQUIRED)

add_executable(mon_app src/main.cpp)
target_link_libraries(mon_app PRIVATE mcf::core mcf::logger)

# Les fonctions de packaging sont automatiquement disponibles!
mcf_package_application(
    TARGET mon_app
    VERSION 1.0.0
    OUTPUT_NAME "MonApp"
)
```

### 1.2 Contenu du Package SDK

Le package SDK inclut **tout** le nécessaire pour que les développeurs utilisent MCF:

| Composant | Description | Utilisation |
|-----------|-------------|-------------|
| **Headers core/** | Headers header-only | Inclus avec `mcf::core` |
| **Headers modules/** | Headers des modules | Inclus avec `mcf::logger`, etc. |
| **Libraries .a** | Bibliothèques pré-compilées | Linkées automatiquement |
| **MCFPackaging.cmake** | Utilitaires de packaging | Fonctions `mcf_package_application()` et `mcf_package_application_bundle()` |
| **ModularCppFrameworkConfig.cmake** | Config CMake | Pour `find_package()` |
| **Documentation** | Guides complets | 10 fichiers markdown |

---

## 2. Installation Système

Pour une installation système complète (recommandé pour développement).

### 2.1 Installation Standard

```bash
cd ModularCppFramework
mkdir build && cd build

# Configuration
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DBUILD_TESTS=OFF \
      -DBUILD_EXAMPLES=OFF \
      ..

# Compilation
make -j$(nproc)

# Installation
sudo make install
```

**Résultat:**
```
/usr/local/
├── include/mcf/
│   ├── core/              # Headers core (header-only)
│   └── modules/           # Headers modules
├── lib/
│   ├── libmcf_logger_module.a
│   ├── libmcf_networking_module.a
│   ├── libmcf_profiling_module.a
│   ├── libmcf_realtime_module.a
│   └── cmake/ModularCppFramework/
│       ├── ModularCppFrameworkConfig.cmake
│       ├── ModularCppFrameworkConfigVersion.cmake
│       ├── MCFPackaging.cmake
│       └── ModularCppFrameworkTargets.cmake
└── share/doc/modular-cpp-framework/
    ├── README.md
    ├── QUICK_START.md
    ├── APPLICATION_PACKAGING.md
    └── ...
```

**Utilisation par les développeurs:**

Après installation, MCF est disponible globalement:

```cmake
# Dans leur CMakeLists.txt
find_package(ModularCppFramework 1.0 REQUIRED)

add_executable(mon_app src/main.cpp)
target_link_libraries(mon_app PRIVATE mcf::core mcf::logger)

# Packaging automatiquement disponible
mcf_package_application(TARGET mon_app VERSION 1.0.0)
```

### 2.2 Installation Personnalisée

Modifier le préfixe d'installation:

```bash
# Installation dans un répertoire personnalisé
cmake -DCMAKE_INSTALL_PREFIX=$HOME/mcf-sdk ..
make install
# → ~/mcf-sdk/include/mcf/...

# Utilisation dans un projet
export CMAKE_PREFIX_PATH=$HOME/mcf-sdk:$CMAKE_PREFIX_PATH
cmake ..
```

---

## 3. Gestionnaires de Paquets

MCF supporte plusieurs gestionnaires de paquets pour faciliter la distribution.

### 3.1 Conan (Recommandé)

**Création du package Conan:**

```bash
# Dans le repo MCF
conan create . --build=missing
```

**Utilisation par les développeurs:**

**conanfile.txt:**
```ini
[requires]
modular-cpp-framework/1.0.2

[generators]
CMakeDeps
CMakeToolchain

[options]
modular-cpp-framework/*:with_logger_module=True
modular-cpp-framework/*:with_networking_module=True
```

**CMakeLists.txt:**
```cmake
cmake_minimum_required(VERSION 3.16)
project(MonApp VERSION 1.0.0)

find_package(ModularCppFramework REQUIRED)

add_executable(mon_app src/main.cpp)
target_link_libraries(mon_app PRIVATE mcf::core mcf::logger)

# Packaging disponible automatiquement
mcf_package_application(TARGET mon_app VERSION 1.0.0)
```

**Installation:**
```bash
mkdir build && cd build
conan install .. --build=missing
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
make
```

**Options Conan disponibles:**

| Option | Description | Défaut |
|--------|-------------|--------|
| `with_logger_module` | Inclure LoggerModule | True |
| `with_networking_module` | Inclure NetworkingModule | True |
| `with_profiling_module` | Inclure ProfilingModule | True |
| `with_realtime_module` | Inclure RealtimeModule | True |

### 3.2 vcpkg

**Installation via vcpkg:**

```bash
# Ajouter MCF au registry vcpkg (si applicable)
vcpkg install modular-cpp-framework

# Avec features spécifiques
vcpkg install modular-cpp-framework[logger,networking]
```

**CMakeLists.txt avec vcpkg:**

```cmake
find_package(ModularCppFramework CONFIG REQUIRED)
target_link_libraries(mon_app PRIVATE mcf::core mcf::logger)

# Packaging disponible
mcf_package_application(TARGET mon_app VERSION 1.0.0)
```

**Features vcpkg disponibles:**

- `logger` - Logger module
- `networking` - Networking module
- `profiling` - Profiling module
- `realtime` - Realtime module

### 3.3 FetchContent (CMake)

**Utilisation directe depuis Git:**

```cmake
include(FetchContent)

FetchContent_Declare(ModularCppFramework
    GIT_REPOSITORY https://github.com/Sylvain-RZ/ModularCppFramework.git
    GIT_TAG v1.0.2
)

# Désactiver tests/exemples
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(ModularCppFramework)

# Utilisation
target_link_libraries(mon_app PRIVATE mcf_core mcf_logger_module)

# Packaging disponible
include(${modularcppframework_SOURCE_DIR}/cmake/MCFPackaging.cmake)
mcf_package_application(TARGET mon_app VERSION 1.0.0)
```

### 3.4 Git Submodule

**Ajouter MCF comme sous-module:**

```bash
cd MonProjet
git submodule add https://github.com/Sylvain-RZ/ModularCppFramework.git external/ModularCppFramework
git submodule update --init --recursive
```

**CMakeLists.txt:**

```cmake
# Désactiver tests/exemples
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(external/ModularCppFramework)

# Utilisation
target_link_libraries(mon_app PRIVATE mcf_core mcf_logger_module)

# Packaging disponible
include(external/ModularCppFramework/cmake/MCFPackaging.cmake)
mcf_package_application(TARGET mon_app VERSION 1.0.0)
```

---

## 4. Distribution et Déploiement

### 4.1 Récapitulatif des Méthodes

| Méthode | Commande | Cas d'usage |
|---------|----------|-------------|
| **Package Release** | `make package-release` | Distribution manuelle (GitHub releases, etc.) |
| **Installation Système** | `sudo make install` | Développement local, serveurs |
| **Conan** | `conan create .` | Distribution via repository Conan |
| **vcpkg** | `vcpkg install` | Intégration avec vcpkg registry |
| **FetchContent** | Git URL dans CMake | CI/CD, projets autonomes |
| **Git Submodule** | `git submodule add` | Contrôle de version strict |

### 4.2 Multi-Plateforme

Le packaging génère automatiquement des noms de package spécifiques à la plateforme:

**Linux:**
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make package-release
# → modular-cpp-framework-1.0.2-Linux-x86_64.tar.gz
```

**Windows (MinGW):**
```bash
cmake -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles" ..
mingw32-make package-release
# → modular-cpp-framework-1.0.2-Windows-AMD64.tar.gz
```

**macOS:**
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make package-release
# → modular-cpp-framework-1.0.2-Darwin-arm64.tar.gz
```

### 4.3 Intégration CI/CD

**GitHub Actions exemple:**

```yaml
name: Package SDK

on:
  release:
    types: [created]

jobs:
  package-linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Build SDK Package
        run: |
          mkdir build && cd build
          cmake -DCMAKE_BUILD_TYPE=Release \
                -DBUILD_TESTS=OFF \
                -DBUILD_EXAMPLES=OFF ..
          make -j$(nproc)
          make package-release

      - name: Upload Release Asset
        uses: actions/upload-release-asset@v1
        with:
          upload_url: ${{ github.event.release.upload_url }}
          asset_path: build/modular-cpp-framework-*.tar.gz
          asset_name: modular-cpp-framework-linux-x86_64.tar.gz
          asset_content_type: application/gzip

  package-windows:
    runs-on: windows-latest
    # Similar pour Windows...

  package-macos:
    runs-on: macos-latest
    # Similar pour macOS...
```

### 4.4 Versioning et Releases

**Bonnes pratiques de versioning:**

```bash
# Utiliser semantic versioning (MAJOR.MINOR.PATCH)
# v1.0.0 - Release initiale
# v1.1.0 - Nouvelles features (backward compatible)
# v1.1.1 - Bugfixes
# v2.0.0 - Breaking changes

# Créer un tag Git
git tag -a v1.0.2 -m "Release v1.0.2: Windows compatibility fixes"
git push origin v1.0.2

# Créer le package pour cette version
cmake ..
make package-release
```

---

## 5. Checklist de Distribution

Avant de distribuer le SDK, vérifiez:

**Compilation et Tests:**
- [ ] Compile sans warnings (`-Wall -Wextra`)
- [ ] Tous les tests passent (25/25)
- [ ] Compatible avec C++17 standard
- [ ] Multi-plateforme testé (Linux, Windows, macOS)

**Documentation:**
- [ ] README.md à jour avec version actuelle
- [ ] CHANGELOG.md contient les changements
- [ ] Tous les guides docs/ sont à jour
- [ ] Exemples fonctionnent avec la nouvelle version

**Packaging:**
- [ ] Version correcte dans CMakeLists.txt
- [ ] Version correcte dans conanfile.py
- [ ] Version correcte dans vcpkg.json
- [ ] `make package-release` fonctionne
- [ ] Package extrait et testé sur machine propre

**Fichiers:**
- [ ] LICENSE présent
- [ ] Headers installés correctement
- [ ] Bibliothèques modules compilées (.a)
- [ ] MCFPackaging.cmake inclus
- [ ] ModularCppFrameworkConfig.cmake généré

**Tests d'Intégration:**
- [ ] `find_package(ModularCppFramework)` fonctionne
- [ ] Linking avec `mcf::core` fonctionne
- [ ] Fonctions `mcf_package_application()` disponibles
- [ ] Projet externe peut utiliser MCF sans erreurs

---

## 6. Résumé

### Commandes Rapides

**Distribuer le SDK:**
```bash
# Package release
make package-release

# Installation système
sudo make install

# Package Conan
conan create . --build=missing
```

**Par les utilisateurs:**
```cmake
# Option 1: Package extrait
list(APPEND CMAKE_PREFIX_PATH "/path/to/mcf")
find_package(ModularCppFramework REQUIRED)

# Option 2: Installation système
find_package(ModularCppFramework REQUIRED)

# Option 3: Conan
find_package(ModularCppFramework REQUIRED)

# Option 4: FetchContent
FetchContent_Declare(...)
FetchContent_MakeAvailable(ModularCppFramework)
```

### Recommandations

**Pour distribution publique:**
- ✅ Utiliser `make package-release` pour releases GitHub
- ✅ Publier sur Conan Center pour faciliter l'usage
- ✅ Tagger les versions avec Git tags
- ✅ Inclure MCFPackaging.cmake dans le SDK

**Pour développeurs:**
- ✅ Installation système pour développement local
- ✅ Conan pour gestion de dépendances
- ✅ FetchContent pour projets autonomes CI/CD

**Bonnes pratiques:**
- Toujours compiler en `-DCMAKE_BUILD_TYPE=Release` pour distribution
- Désactiver tests/exemples (`-DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF`)
- Versionner avec semantic versioning (MAJOR.MINOR.PATCH)
- Tester le package sur machine propre avant distribution
- Documenter les breaking changes dans CHANGELOG.md
- Inclure LICENSE et README dans tous les packages

---

## Voir Aussi

**Guides connexes:**
- [APPLICATION_PACKAGING.md](../sdk/APPLICATION_PACKAGING.md) - Guide de packaging d'applications pour utilisateurs finaux
- [QUICK_START.md](../sdk/QUICK_START.md) - Guide de démarrage rapide pour nouveaux utilisateurs
- [ARCHITECTURE.md](../sdk/ARCHITECTURE.md) - Architecture du framework
- [README.md](../../README.md) - Documentation principale

**Ressources externes:**
- [CMake Packaging Guide](https://cmake.org/cmake/help/latest/manual/cmake-packages.7.html)
- [Conan Documentation](https://docs.conan.io/)
- [vcpkg Guide](https://vcpkg.io/en/getting-started.html)
- [Semantic Versioning](https://semver.org/)

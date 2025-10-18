# Guide de Build - ModularCppFramework

Ce guide détaille toutes les options de compilation et de build pour les développeurs et mainteneurs du framework.

## Table des Matières

- [Prérequis Détaillés](#prérequis-détaillés)
- [Build Standard](#build-standard)
- [Options de Build CMake](#options-de-build-cmake)
- [Builds Spécialisés](#builds-spécialisés)
- [Exécution des Tests](#exécution-des-tests)
- [Exécution des Exemples](#exécution-des-exemples)
- [Debugging et Développement](#debugging-et-développement)
- [Build Multi-Plateforme](#build-multi-plateforme)
- [CI/CD Local](#cicd-local)

---

## Prérequis Détaillés

### Compilateurs Supportés

| Plateforme | Compilateur | Version Minimale | Testé avec |
|------------|-------------|------------------|------------|
| Linux | GCC | 7.0+ | GCC 9.4, 11.4 |
| Linux | Clang | 5.0+ | Clang 10, 14 |
| macOS | Apple Clang | 10.0+ | Xcode 12+ |
| Windows | MSVC | 2017+ | MSVC 2019, 2022 |
| Windows | MinGW | GCC 7.0+ | MinGW-w64 8.1 |

### CMake

- Version minimale: **3.16**
- Version recommandée: **3.20+**
- Fonctionnalités utilisées:
  - `target_link_libraries` avec visibilité PRIVATE/PUBLIC
  - `add_library` INTERFACE pour header-only
  - `target_compile_features` pour C++17

### Dépendances Système

#### Linux (Ubuntu/Debian)

```bash
# Compilateur
sudo apt-get install build-essential cmake

# Dépendances runtime
sudo apt-get install libdl-dev libpthread-stubs0-dev

# Optionnel: pour génération docs
sudo apt-get install doxygen graphviz

# Optionnel: pour coverage
sudo apt-get install lcov
```

#### Linux (Fedora/RHEL)

```bash
sudo dnf install gcc gcc-c++ cmake
sudo dnf install glibc-devel

# Optionnel
sudo dnf install doxygen graphviz lcov
```

#### macOS

```bash
# Installer Xcode Command Line Tools
xcode-select --install

# Installer CMake via Homebrew
brew install cmake

# Optionnel
brew install doxygen graphviz
```

#### Windows

- **Visual Studio 2017+** avec C++ workload
- Ou **MinGW-w64** pour builds GCC
- **CMake** (télécharger depuis cmake.org)

---

## Build Standard

### Build Rapide (Sans Tests/Exemples)

```bash
# Cloner le repository
git clone https://github.com/Sylvain-RZ/ModularCppFramework.git
cd ModularCppFramework

# Créer le build directory
mkdir build && cd build

# Configurer
cmake ..

# Compiler
make -j$(nproc)
```

**Résultat:**
- Core library: header-only (pas de binaire)
- Modules compilés: `libmcf_logger_module.a`, etc.
- Pas de tests ni exemples

### Build Complet (Développement)

```bash
mkdir build && cd build

# Configurer avec tests et exemples
cmake -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON ..

# Compiler
make -j$(nproc)

# Vérifier
ls bin/         # Exemples
ls bin/tests/   # Tests
```

**Résultat:**
- Core + modules
- 25 exécutables de test
- 8 applications exemple
- 2 plugins exemple (.so)

---

## Options de Build CMake

### Options Principales

| Option | Défaut | Description |
|--------|--------|-------------|
| `BUILD_TESTS` | OFF | Compiler les tests unitaires et d'intégration |
| `BUILD_EXAMPLES` | OFF | Compiler les applications exemple |
| `CMAKE_BUILD_TYPE` | Debug | Type de build (Debug, Release, RelWithDebInfo, MinSizeRel) |
| `CMAKE_INSTALL_PREFIX` | /usr/local | Répertoire d'installation |

### Exemples d'Utilisation

```bash
# Build debug avec tests
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON ..

# Build release optimisé
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build avec debug info + optimisations
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTS=ON ..

# Build minimal (taille)
cmake -DCMAKE_BUILD_TYPE=MinSizeRel ..

# Installation dans un préfixe custom
cmake -DCMAKE_INSTALL_PREFIX=/opt/mcf ..
```

---

## Builds Spécialisés

### Build avec Sanitizers (Détection de bugs)

#### AddressSanitizer (Détection fuites mémoire)

```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
      -DBUILD_TESTS=ON \
      ..
make -j$(nproc)

# Exécuter tests avec ASAN
ASAN_OPTIONS=detect_leaks=1 ctest -V
```

#### ThreadSanitizer (Détection race conditions)

```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=thread -fno-omit-frame-pointer" \
      -DBUILD_TESTS=ON \
      ..
make -j$(nproc)
ctest -V
```

#### UndefinedBehaviorSanitizer

```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=undefined -fno-omit-frame-pointer" \
      -DBUILD_TESTS=ON \
      ..
make -j$(nproc)
ctest -V
```

### Build avec Coverage (Couverture de code)

```bash
mkdir build-coverage && cd build-coverage

# Configurer avec flags coverage
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="--coverage -fprofile-arcs -ftest-coverage" \
      -DBUILD_TESTS=ON \
      ..

make -j$(nproc)

# Exécuter tous les tests
ctest

# Générer rapport coverage avec lcov
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/external/*' '*/tests/*' --output-file coverage_filtered.info

# Générer HTML
genhtml coverage_filtered.info --output-directory coverage_html

# Ouvrir dans navigateur
xdg-open coverage_html/index.html
```

### Build avec Profiling (Performance)

```bash
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_CXX_FLAGS="-pg" \
      -DBUILD_EXAMPLES=ON \
      ..
make -j$(nproc)

# Exécuter un exemple pour générer gmon.out
./bin/profiling_example

# Analyser avec gprof
gprof ./bin/profiling_example gmon.out > analysis.txt
```

### Build Static (Sans shared libraries)

```bash
cmake -DBUILD_SHARED_LIBS=OFF ..
make -j$(nproc)
```

---

## Exécution des Tests

### Tous les Tests

```bash
cd build

# Exécuter avec CTest (verbose)
ctest -V

# Résultat attendu:
# 100% tests passed, 0 tests failed out of 25
# Total Test time (real) = ~24 sec
```

### Tests Individuels

```bash
# Tests unitaires
./bin/tests/test_eventbus
./bin/tests/test_eventbus_edge_cases
./bin/tests/test_service_locator
./bin/tests/test_resource_manager
./bin/tests/test_dependency_resolver
./bin/tests/test_filesystem
./bin/tests/test_file_watcher
./bin/tests/test_thread_pool
./bin/tests/test_json_parser_edge_cases
./bin/tests/test_plugin_loader
./bin/tests/test_plugin_loader_edge_cases
./bin/tests/test_plugin_manager_edge_cases
./bin/tests/test_logger_edge_cases
./bin/tests/test_module_system

# Tests d'intégration
./bin/tests/integration_test_app
./bin/tests/integration_test_error_recovery
./bin/tests/integration_test_hot_reload
./bin/tests/integration_test_hot_reload_real_plugins
./bin/tests/integration_test_plugin_communication
./bin/tests/integration_test_plugin_dependencies
./bin/tests/integration_test_stress
./bin/tests/integration_test_config_hot_reload

# Autres tests
./bin/tests/test_config
./bin/tests/test_logger
```

### Tests par Catégorie

```bash
# Seulement tests unitaires
ctest -R "^test_" -V

# Seulement tests d'intégration
ctest -R "^integration_" -V

# Tests spécifiques
ctest -R "eventbus" -V
ctest -R "plugin" -V
```

### Tests avec Timeout

```bash
# Timeout de 30 secondes par test
ctest --timeout 30 -V
```

---

## Exécution des Exemples

### Compiler les Exemples

```bash
cmake -DBUILD_EXAMPLES=ON ..
make -j$(nproc)
```

### Exemples Disponibles

```bash
# Logger avec configuration JSON
./bin/logger_example

# Application temps réel (60 FPS)
./bin/realtime_app_example

# Application event-driven
./bin/event_driven_app_example

# Hot-reload de plugins
./bin/hot_reload_demo

# Profiling et métriques
./bin/profiling_example

# Utilitaires filesystem
./bin/filesystem_example

# Serveur TCP (écoute sur port 8080)
./bin/server_example

# Client TCP (dans un autre terminal)
./bin/client_example
```

### Exemples avec Configuration

Certains exemples nécessitent des fichiers de configuration:

```bash
# Créer config pour logger_example
cat > logger_config.json << EOF
{
    "loggers": {
        "App": {
            "level": "debug",
            "sinks": [
                {"type": "console", "colored": true},
                {"type": "file", "path": "logs/app.log"}
            ]
        }
    }
}
EOF

# Exécuter
./bin/logger_example
```

---

## Debugging et Développement

### Build Debug avec Symboles

```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-g3 -O0" \
      -DBUILD_TESTS=ON \
      ..
make -j$(nproc)
```

### Debug avec GDB

```bash
# Compiler en debug
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON ..
make -j$(nproc)

# Lancer avec GDB
gdb ./bin/tests/test_eventbus

# Dans GDB:
(gdb) break EventBus::publish
(gdb) run
(gdb) backtrace
```

### Debug avec Valgrind (Fuites mémoire)

```bash
# Installer valgrind
sudo apt-get install valgrind

# Exécuter un test
valgrind --leak-check=full --show-leak-kinds=all ./bin/tests/test_eventbus

# Ou tous les tests
ctest -T memcheck
```

### Debug Plugin Loading

```bash
# Lister symboles exportés d'un plugin
nm -D plugins/example_plugin.so | grep -E "createPlugin|destroyPlugin"

# Vérifier dépendances dynamiques
ldd plugins/example_plugin.so

# Tracer chargement dynamique
strace -e trace=open,openat ./bin/test_app 2>&1 | grep ".so"

# Avec ltrace pour voir dlopen/dlsym
ltrace -e 'dlopen+dlsym' ./bin/test_app
```

### Verbose Build

```bash
# Voir toutes les commandes de compilation
make VERBOSE=1

# Ou avec CMake
cmake -DCMAKE_VERBOSE_MAKEFILE=ON ..
make
```

---

## Build Multi-Plateforme

### Linux

```bash
# GCC
cmake -DCMAKE_CXX_COMPILER=g++ ..

# Clang
cmake -DCMAKE_CXX_COMPILER=clang++ ..
```

### macOS

```bash
# Apple Clang (défaut)
cmake ..

# Spécifier SDK
cmake -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 ..
```

### Windows (Visual Studio)

```bash
# Ouvrir "x64 Native Tools Command Prompt"
mkdir build && cd build

# Générer projet Visual Studio
cmake -G "Visual Studio 16 2019" -A x64 ..

# Compiler
cmake --build . --config Release

# Ou ouvrir le .sln dans Visual Studio
```

### Windows (MinGW)

```bash
# Installer MinGW-w64
# Dans terminal MinGW:
mkdir build && cd build

cmake -G "MinGW Makefiles" ..
mingw32-make -j4
```

### Cross-Compilation

```bash
# Exemple: Linux → ARM
cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-arm.cmake ..
```

---

## CI/CD Local

### Reproduire le CI GitHub Actions Localement

```bash
# Ubuntu build (comme CI)
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON ..
make -j$(nproc)
ctest -V

# Coverage (comme CI sur Linux Debug)
mkdir build-coverage && cd build-coverage
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="--coverage" \
      -DBUILD_TESTS=ON \
      ..
make -j$(nproc)
ctest
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

### Vérification Pré-Commit

```bash
# Script de vérification complète
#!/bin/bash
set -e

echo "=== Build Debug avec Tests ==="
mkdir -p build-debug && cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON ..
make -j$(nproc)
ctest -V
cd ..

echo "=== Build Release ==="
mkdir -p build-release && cd build-release
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON ..
make -j$(nproc)
ctest -V
cd ..

echo "=== Check Code Quality ==="
# Chercher TODOs/FIXMEs
grep -r "TODO\|FIXME" core/ modules/ || true

# Vérifier line endings (LF only)
find . -name "*.cpp" -o -name "*.hpp" | xargs file | grep CRLF && exit 1 || true

echo "=== All Checks Passed! ==="
```

---

## Génération de Documentation

### Doxygen

```bash
# Installer Doxygen
sudo apt-get install doxygen graphviz

# Générer documentation
doxygen Doxyfile

# Ouvrir dans navigateur
xdg-open docs/doxygen/html/index.html
```

### Script de Génération

```bash
#!/bin/bash
# generate_docs.sh

if ! command -v doxygen &> /dev/null; then
    echo "Doxygen not found. Install with: sudo apt-get install doxygen"
    exit 1
fi

echo "Generating Doxygen documentation..."
doxygen Doxyfile

if [ $? -eq 0 ]; then
    echo "Documentation generated successfully in docs/doxygen/html/"
    xdg-open docs/doxygen/html/index.html 2>/dev/null || open docs/doxygen/html/index.html 2>/dev/null
else
    echo "Error generating documentation"
    exit 1
fi
```

---

## Installation

### Installation Système

```bash
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DBUILD_TESTS=OFF \
      -DBUILD_EXAMPLES=OFF \
      ..
make -j$(nproc)
sudo make install
```

**Fichiers installés:**
- Headers: `/usr/local/include/mcf/`
- Modules: `/usr/local/lib/libmcf_*_module.a`
- CMake config: `/usr/local/lib/cmake/ModularCppFramework/`

### Désinstallation

```bash
cd build
sudo make uninstall
# Ou manuellement:
sudo rm -rf /usr/local/include/mcf
sudo rm -f /usr/local/lib/libmcf_*
sudo rm -rf /usr/local/lib/cmake/ModularCppFramework
```

---

## Dépannage

### Erreur: "C++17 not supported"

```bash
# Vérifier version compilateur
g++ --version  # Doit être >= 7.0
clang++ --version  # Doit être >= 5.0

# Forcer C++17
cmake -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON ..
```

### Erreur: "libdl not found" (Linux)

```bash
sudo apt-get install libdl-dev
```

### Erreur: "pthread not found" (Linux)

```bash
sudo apt-get install libpthread-stubs0-dev
```

### Build très lent

```bash
# Utiliser tous les cores
make -j$(nproc)

# Ou spécifier nombre de jobs
make -j8

# Utiliser ccache pour cache compilation
sudo apt-get install ccache
cmake -DCMAKE_CXX_COMPILER_LAUNCHER=ccache ..
```

### Tests échouent aléatoirement

Possible race condition:
```bash
# Exécuter tests un par un
ctest --output-on-failure --schedule-random --repeat until-fail:10
```

---

## Métriques de Build

**Temps de Build (Intel i7, 8 cores, 16GB RAM):**
- Core only: ~0.5s (header-only)
- Core + Modules: ~5s
- Full (Tests + Examples): ~45s

**Tailles:**
- Headers core: ~150 KB
- Modules compilés: ~2 MB
- Tests binaires: ~15 MB
- Total repo avec .git: ~50 MB

---

**Retour:** [Documentation Development](README.md) | **Voir aussi:** [Test Coverage](TEST_COVERAGE.md)

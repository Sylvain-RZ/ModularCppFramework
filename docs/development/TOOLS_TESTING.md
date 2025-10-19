# Tools Testing Guide

Ce document décrit la suite de tests pour les scripts Python de génération et de packaging dans le répertoire `tools/`.

## Vue d'ensemble

ModularCppFramework inclut une suite de tests C++ utilisant Catch2 pour valider le bon fonctionnement des scripts Python de génération et de packaging. Ces tests garantissent que les outils fonctionnent correctement sur toutes les plateformes (Linux, Windows, macOS).

## Tests Implémentés

### Fichier de Test: `tests/unit/test_tools_scripts.cpp`

Ce fichier contient tous les tests pour les scripts tools, organisés par catégorie:

#### 1. Tests pour `create-plugin.py`

- **Test d'aide**: Vérifie que l'option `--help` fonctionne
- **Création de plugin basique**: Vérifie la génération d'un plugin minimal
- **Plugin realtime**: Vérifie l'ajout de l'interface `IRealtimeUpdatable`
- **Plugin event-driven**: Vérifie l'ajout de l'interface `IEventDriven`
- **Plugin complet**: Vérifie la génération avec toutes les interfaces
- **Gestion d'erreurs**: Vérifie le comportement avec des arguments invalides

#### 2. Tests pour `create-application.py`

- **Test d'aide**: Vérifie que l'option `--help` fonctionne
- **Création d'application basique**: Vérifie la génération d'une application minimale
- **Application realtime**: Vérifie l'ajout du support realtime
- **Application avec configuration**: Vérifie la génération des fichiers de configuration
- **Application avec modules**: Vérifie l'inclusion de modules (logger, profiling, etc.)

#### 3. Tests pour `package-application.py`

- **Test d'aide**: Vérifie que l'option `--help` fonctionne
- **Packaging des exemples MCF**: Vérifie la création du package complet
- **Option de répertoire de sortie**: Vérifie la copie du package vers un répertoire spécifique

#### 4. Tests d'Intégration

- **Création de plugin et vérification CMake**: Vérifie que le plugin généré est valide pour CMake
- **Création d'application complète**: Vérifie la structure complète d'une application
- **Tests multi-plateforme**: Vérifie les permissions et l'exécutabilité des scripts

## Classes Utilitaires

### `ScriptExecutor`

Classe helper pour exécuter des commandes et capturer leur sortie:

```cpp
auto result = ScriptExecutor::execute("python3 tools/create-plugin.py --help");
if (result.success()) {
    std::cout << result.output << std::endl;
}
```

**Caractéristiques**:
- Capture stdout et stderr séparément
- Retourne le code de sortie normalisé
- Support multi-plateforme (Linux, Windows, macOS)
- Gère automatiquement `python3` (Linux/macOS) et `python` (Windows)
- Redirection temporaire des flux

### `ToolsTestFixture`

Fixture de test fournissant un environnement isolé pour les tests:

```cpp
ToolsTestFixture fixture;
fixture.SetUp();  // Crée un répertoire temporaire
// ... exécuter les tests ...
fixture.TearDown();  // Nettoie le répertoire temporaire
```

**Méthodes utiles**:
- `getTestPath(relative)`: Retourne un chemin dans le répertoire de test temporaire
- `getScriptPath(script)`: Retourne le chemin complet d'un script
- `fileContains(path, substring)`: Vérifie qu'un fichier contient une chaîne
- `directoryContains(dir, filename)`: Vérifie qu'un répertoire contient un fichier

## Compilation et Exécution

### Compiler les Tests

```bash
cd build
cmake ..
make test_tools_scripts
```

### Exécuter les Tests

```bash
# Tous les tests tools
./bin/tests/test_tools_scripts

# Tests spécifiques
./bin/tests/test_tools_scripts "[tools][create-plugin]"
./bin/tests/test_tools_scripts "[tools][create-application]"
./bin/tests/test_tools_scripts "[tools][package]"

# Avec sortie détaillée
./bin/tests/test_tools_scripts -r compact
./bin/tests/test_tools_scripts -s  # Très détaillé
```

### Exécuter via CTest

```bash
cd build
ctest -R ToolsScripts -V
```

## Support Multi-Plateforme

Les tests sont conçus pour fonctionner sur plusieurs plateformes:

### Linux
- Scripts shell exécutés directement
- Vérification des permissions d'exécution
- Chemins avec séparateur `/`

### Windows (Prévu)
- Scripts PowerShell ou batch équivalents
- Adaptation des chemins avec `\`
- Gestion différente des codes de sortie

### macOS (Prévu)
- Similaire à Linux
- Support des chemins spécifiques macOS
- Adaptation pour les bundles d'application

## Architecture des Tests

### Détection du Chemin du Projet

Les tests déterminent automatiquement le chemin racine du projet:

```cpp
// Linux
readlink("/proc/self/exe", exe_path, sizeof(exe_path));

// macOS
_NSGetExecutablePath(exe_path, &size);

// Windows
GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
```

Puis remonte l'arborescence:
```
build/bin/tests/test_tools_scripts  (exécutable)
       ↓
build/bin/tests  (dirname)
       ↓
build/bin        (dirname)
       ↓
build            (dirname)
       ↓
project_root     (dirname)
```

### Isolation des Tests

Chaque test utilise un répertoire temporaire unique:
```
/tmp/mcf_tools_test_<pid>/
```

Cela garantit que:
- Les tests peuvent s'exécuter en parallèle
- Aucune pollution entre les tests
- Nettoyage automatique via `TearDown()`

## Limites Actuelles

### 1. Chemins de Sortie Fixes

Les scripts `create-plugin.py` et `create-application.py` génèrent toujours dans les répertoires `plugins/` et le répertoire parent respectivement. Il n'y a pas d'option pour spécifier un chemin de sortie personnalisé.

**Impact**: Les tests qui tentent de créer des plugins/applications dans des chemins temporaires échoueront actuellement.

**Solution future**: L'option `-o, --output PATH` existe déjà dans les scripts Python.

### 2. Nettoyage des Artefacts

Les tests doivent nettoyer les plugins/applications créés dans le projet réel.

**Recommandation**: Utiliser des noms de test préfixés (ex: `TEST_Plugin_xyz`) pour faciliter l'identification et le nettoyage.

### 3. Tests d'Intégration Limités

Les tests actuels vérifient principalement:
- Que les scripts s'exécutent sans erreur
- Que les fichiers sont créés
- Que le contenu de base est présent

Ils ne vérifient pas encore:
- Que le code généré compile effectivement
- Que les plugins peuvent être chargés dynamiquement
- Que les applications fonctionnent correctement

## Améliiorations Futures

### 1. Tests de Compilation

Ajouter des tests qui compilent réellement le code généré:

```cpp
TEST_CASE("Generated plugin compiles", "[tools][integration][compilation]") {
    // 1. Générer un plugin
    // 2. Exécuter cmake + make
    // 3. Vérifier que la compilation réussit
    // 4. Vérifier que le .so est créé
}
```

### 2. Tests de Chargement Dynamique

Vérifier que les plugins générés peuvent être chargés:

```cpp
TEST_CASE("Generated plugin loads", "[tools][integration][loading]") {
    // 1. Générer un plugin
    // 2. Compiler le plugin
    // 3. Utiliser PluginLoader pour charger le plugin
    // 4. Vérifier les symboles exportés
}
```

### 3. Tests de Validation de Contenu

Vérifier plus en détail le contenu généré:

```cpp
TEST_CASE("Generated CMakeLists is valid", "[tools][validation]") {
    // Vérifier que:
    // - Toutes les variables requises sont définies
    // - Les chemins d'include sont corrects
    // - Les dépendances sont listées
}
```

### 4. Tests de Performance

Mesurer le temps de génération:

```cpp
BENCHMARK("Plugin generation time") {
    return ScriptExecutor::execute("python3 tools/create-plugin.py -n BenchPlugin");
};
```

### 5. Tests de Compatibilité Windows

Créer des tests spécifiques pour Windows:

```cpp
#ifdef _WIN32
TEST_CASE("Windows plugin generation", "[tools][windows]") {
    // Tests spécifiques Windows
}
#endif
```

## Contribution

Pour ajouter de nouveaux tests:

1. Ajouter le test dans `tests/unit/test_tools_scripts.cpp`
2. Utiliser le fixture `ToolsTestFixture` pour l'isolation
3. Utiliser les tags Catch2 appropriés: `[tools]`, `[create-plugin]`, `[create-application]`, `[package]`, `[integration]`
4. Documenter les nouveaux tests dans ce fichier

## Exemples de Tests

### Test Simple

```cpp
TEST_CASE("create-plugin.py creates basic plugin", "[tools][create-plugin]") {
    ToolsTestFixture fixture;

    auto result = ScriptExecutor::execute("python3 tools/create-plugin.py -n SimplePlugin");

    REQUIRE(result.success());
    REQUIRE(fixture.fs.exists(fixture.projectRoot + "/plugins/SimplePlugin"));
}
```

### Test avec Vérification de Contenu

```cpp
TEST_CASE("Generated plugin has correct header", "[tools][create-plugin]") {
    ToolsTestFixture fixture;

    ScriptExecutor::execute("python3 tools/create-plugin.py -n HeaderTest -r");

    std::string headerPath = fixture.projectRoot + "/plugins/HeaderTest/HeaderTest.hpp";
    REQUIRE(fixture.fileContains(headerPath, "class HeaderTest"));
    REQUIRE(fixture.fileContains(headerPath, "public mcf::IPlugin"));
    REQUIRE(fixture.fileContains(headerPath, "public mcf::IRealtimeUpdatable"));
}
```

## Références

- [Catch2 Documentation](https://github.com/catchorg/Catch2/tree/devel/docs)
- [TEST_COVERAGE.md](TEST_COVERAGE.md) - Stratégie de test globale
- [PLUGIN_GUIDE.md](../sdk/PLUGIN_GUIDE.md) - Guide de création de plugins
- [APPLICATION_PACKAGING.md](../sdk/APPLICATION_PACKAGING.md) - Guide de packaging

---

**Note**: Cette documentation fait partie du projet ModularCppFramework et est destinée aux contributeurs et mainteneurs du framework.

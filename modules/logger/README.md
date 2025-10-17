# Logger Module

Module de logging complet et configurable pour ModularCppFramework avec support de configuration JSON.

## Fonctionnalités

- **Niveaux de log**: Trace, Debug, Info, Warning, Error, Critical
- **Multiple sinks**: Console, File, Rotating File
- **Formatage personnalisable**: Templates de format de message
- **Thread-safe**: Utilisation sécurisée en multi-threading
- **Configuration JSON**: Configuration complète via JSON
- **Hot-reload**: Rechargement de la configuration à chaud
- **Colorisation**: Support des couleurs ANSI pour la console
- **Rotation de fichiers**: Gestion automatique de la taille des logs

## Installation

Le module est header-only et s'intègre automatiquement avec le framework.

```cmake
target_link_libraries(your_target PRIVATE mcf_core mcf_logger_module)
```

## Utilisation de base

### 1. Sans configuration (utilisation directe)

```cpp
#include "core/Logger.hpp"

// Utiliser le logger par défaut avec les macros
MCF_INFO("Application started");
MCF_WARNING("This is a warning");
MCF_ERROR("An error occurred");

// Créer un logger personnalisé
auto logger = mcf::LoggerRegistry::instance().getLogger("my_app");
logger->info("Custom logger message");
logger->error("Error from custom logger");
```

### 2. Avec le module (recommandé)

```cpp
#include "modules/logger/LoggerModule.hpp"
#include "core/Application.hpp"

class MyApp : public mcf::Application {
public:
    bool initialize() override {
        // Ajouter le module logger (haute priorité, s'initialise tôt)
        auto logger_module = addModule<mcf::LoggerModule>();

        // Le module lit automatiquement la configuration depuis ConfigurationManager
        return Application::initialize();
    }
};

int main() {
    MyApp app;

    // Charger la configuration
    app.getConfigurationManager()->load("config/logging.json");

    // Initialiser et lancer
    app.initialize();
    app.run();

    return 0;
}
```

## Configuration JSON

### Format de base

```json
{
  "logging": {
    "global_level": "info",
    "pattern": "[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v",
    "loggers": [
      {
        "name": "app",
        "level": "debug",
        "sinks": [
          {
            "type": "console",
            "level": "info",
            "color": true
          }
        ]
      }
    ]
  }
}
```

### Niveaux de log disponibles

- `"trace"` - Très détaillé, pour le debugging
- `"debug"` - Informations de débogage
- `"info"` - Messages informatifs
- `"warning"` ou `"warn"` - Avertissements
- `"error"` - Erreurs
- `"critical"` ou `"crit"` - Erreurs critiques
- `"off"` - Désactiver le logging

### Types de sinks

#### Console Sink

```json
{
  "type": "console",
  "level": "info",
  "color": true
}
```

- `color`: Active/désactive la colorisation ANSI (défaut: true)
- `level`: Niveau minimum pour ce sink

#### File Sink

```json
{
  "type": "file",
  "level": "debug",
  "path": "logs/app.log",
  "truncate": false
}
```

- `path`: Chemin du fichier de log (obligatoire)
- `truncate`: true pour vider le fichier au démarrage (défaut: false)
- `level`: Niveau minimum pour ce sink

#### Rotating File Sink

```json
{
  "type": "rotating",
  "level": "info",
  "path": "logs/app_rotating.log",
  "max_size": 10485760,
  "max_files": 5
}
```

- `path`: Chemin du fichier de log de base (obligatoire)
- `max_size`: Taille maximale en octets avant rotation (défaut: 10MB)
- `max_files`: Nombre de fichiers de backup à conserver (défaut: 5)
- `level`: Niveau minimum pour ce sink

### Format des messages

Le pattern de formatage supporte les codes suivants:

- `%Y-%m-%d %H:%M:%S.%e` - Timestamp complet avec millisecondes
- `%n` - Nom du logger
- `%l` - Niveau de log (INFO, DEBUG, etc.)
- `%v` - Message
- `%s` - Nom du fichier source
- `%#` - Numéro de ligne
- `%!` - Nom de la fonction
- `%t` - Thread ID
- `%%` - Caractère % littéral

Exemple de pattern:
```
"[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v"
```

Sortie:
```
[2025-10-17 19:56:42.123] [app] [INFO] Application started
```

## Configuration complète exemple

```json
{
  "logging": {
    "global_level": "info",
    "pattern": "[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v",
    "loggers": [
      {
        "name": "app",
        "level": "debug",
        "sinks": [
          {
            "type": "console",
            "level": "info",
            "color": true
          },
          {
            "type": "file",
            "level": "debug",
            "path": "logs/app.log",
            "truncate": false
          }
        ]
      },
      {
        "name": "network",
        "level": "trace",
        "sinks": [
          {
            "type": "console",
            "level": "warning",
            "color": true
          },
          {
            "type": "rotating",
            "level": "trace",
            "path": "logs/network.log",
            "max_size": 5242880,
            "max_files": 3
          }
        ]
      },
      {
        "name": "performance",
        "level": "debug",
        "sinks": [
          {
            "type": "file",
            "level": "debug",
            "path": "logs/performance.log",
            "truncate": true
          }
        ]
      }
    ]
  }
}
```

## Utilisation avancée

### Créer un logger programmatiquement

```cpp
auto logger_module = app.addModule<mcf::LoggerModule>();

// Créer un logger avec console et fichier
auto logger = logger_module->createLogger(
    "my_logger",                    // Nom
    mcf::LogLevel::Debug,           // Niveau
    true,                           // Console
    true,                           // Fichier
    "logs/my_logger.log"            // Chemin du fichier
);

logger->info("Logger created programmatically");
```

### Utiliser les macros avec métadonnées

Les macros incluent automatiquement le fichier, la ligne et la fonction:

```cpp
auto logger = mcf::LoggerRegistry::instance().getLogger("app");

MCF_LOG_INFO(logger, "This includes file/line/function info");
MCF_LOG_ERROR(logger, "Error with metadata");
```

### Accéder aux loggers configurés

```cpp
// Obtenir un logger par nom
auto app_logger = mcf::LoggerRegistry::instance().getLogger("app");
auto network_logger = mcf::LoggerRegistry::instance().getLogger("network");

// Utiliser les loggers
app_logger->info("Application event");
network_logger->debug("Network packet received");
```

### Recharger la configuration

```cpp
auto logger_module = /* obtenir le module */;

// Recharger depuis le ConfigurationManager
logger_module->reloadConfiguration();
```

### Flush manuel

```cpp
auto logger = mcf::LoggerRegistry::instance().getLogger("app");

logger->info("Important message");
logger->flush(); // Force l'écriture immédiate

// Ou flush tous les loggers
mcf::LoggerRegistry::instance().flushAll();
```

### Watch de configuration

Le module peut surveiller les changements de configuration:

```cpp
// Activé par défaut
auto logger_module = addModule<mcf::LoggerModule>(true);

// Désactivé
auto logger_module = addModule<mcf::LoggerModule>(false);
```

## Intégration avec les plugins

Les plugins peuvent obtenir des loggers via le système:

```cpp
class MyPlugin : public mcf::IPlugin {
private:
    std::shared_ptr<mcf::Logger> m_logger;

public:
    bool initialize(const mcf::PluginContext& context) override {
        // Obtenir un logger
        m_logger = mcf::LoggerRegistry::instance().getLogger("my_plugin");

        m_logger->info("Plugin initialized");
        return true;
    }

    void onUpdate(float deltaTime) override {
        m_logger->debug("Update called: " + std::to_string(deltaTime));
    }
};
```

## Performance

- **Thread-safe**: Utilise le pattern "copy-under-lock" pour éviter les deadlocks
- **Filtrage précoce**: Les messages sous le niveau configuré sont ignorés immédiatement
- **Buffering**: Les fichiers utilisent le buffering standard de C++
- **Lock-free logging**: Les callbacks sont exécutés hors du lock pour maximiser les performances

## Bonnes pratiques

1. **Utiliser les niveaux appropriés**:
   - `trace`: Détails d'exécution très fins
   - `debug`: Informations de débogage
   - `info`: Événements importants de l'application
   - `warning`: Situations anormales mais gérées
   - `error`: Erreurs nécessitant attention
   - `critical`: Erreurs catastrophiques

2. **Organiser par catégories**:
   - Créer des loggers séparés pour différents composants
   - Exemple: "app", "network", "renderer", "physics"

3. **Configurer les niveaux par environnement**:
   - Development: `debug` ou `trace`
   - Production: `info` ou `warning`

4. **Utiliser la rotation pour les logs longs**:
   - Évite les fichiers de log trop volumineux
   - Configure `max_size` et `max_files` appropriés

5. **Flush sur événements critiques**:
   ```cpp
   logger->critical("Critical error occurred");
   logger->flush(); // S'assurer que le message est écrit
   ```

## Tests

Lancer les tests du logger:

```bash
cd build
make test_logger
./bin/tests/test_logger
```

Les tests couvrent:
- Logging basique
- Niveaux de log
- File sinks
- Rotating file sinks
- Multiple sinks
- Registry
- Thread safety
- Module integration
- Macros
- Formatage
- Configuration reload

## Exemples de sortie

### Console avec couleurs

```
[32m[2025-10-17 19:56:42.123] [app] [INFO] Application started[0m
[33m[2025-10-17 19:56:42.456] [app] [WARN] Configuration file not found[0m
[31m[2025-10-17 19:56:42.789] [app] [ERROR] Failed to load resource[0m
```

### Fichier de log

```
[2025-10-17 19:56:42.123] [app] [INFO] Application started
[2025-10-17 19:56:42.456] [app] [DEBUG] Loading configuration from config.json
[2025-10-17 19:56:42.789] [network] [TRACE] Packet received: 1024 bytes
[2025-10-17 19:56:43.012] [renderer] [INFO] Frame rendered in 16.67ms
```

## Architecture

Le module est composé de:

- **Logger**: Classe principale de logging avec nom et niveau
- **LogSink**: Interface pour les destinations de log
  - **ConsoleSink**: Sortie console (stdout/stderr)
  - **FileSink**: Fichier de log simple
  - **RotatingFileSink**: Fichier avec rotation automatique
- **LogFormatter**: Formatage des messages selon un pattern
- **LoggerRegistry**: Singleton pour gérer tous les loggers
- **LoggerModule**: Module intégré avec Application et ConfigurationManager

## Dépendances

- C++17
- Headers standard: `<chrono>`, `<fstream>`, `<mutex>`, `<thread>`
- Système de fichiers C++17: `<filesystem>`
- ModularCppFramework Core: ConfigurationManager, Application

## License

Ce module fait partie du projet ModularCppFramework.

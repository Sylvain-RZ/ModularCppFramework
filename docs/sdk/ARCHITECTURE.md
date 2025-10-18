# Architecture du Framework ModularCppFramework

## Vue d'Ensemble

```
┌─────────────────────────────────────────────────────────────────────┐
│                          Application                                 │
│  ┌───────────────────────────────────────────────────────────────┐ │
│  │                      Core Services                             │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐   │ │
│  │  │  EventBus    │  │ServiceLocator│  │ResourceManager   │   │ │
│  │  │              │  │              │  │                  │   │ │
│  │  │ - Pub/Sub    │  │ - DI         │  │ - Cache          │   │ │
│  │  │ - Thread-safe│  │ - Lifetimes  │  │ - Ref counting   │   │ │
│  │  └──────────────┘  └──────────────┘  └──────────────────┘   │ │
│  └───────────────────────────────────────────────────────────────┘ │
│                                                                      │
│  ┌───────────────────────────────────────────────────────────────┐ │
│  │                    Module System                               │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐   │ │
│  │  │LoggerModule  │  │ConfigModule  │  │  Module    │   │ │
│  │  │(Priority:1000)│  │(Priority:900)│  │  (Priority:500)  │   │ │
│  │  └──────────────┘  └──────────────┘  └──────────────────┘   │ │
│  │         Modules statiques compilés dans l'application         │ │
│  └───────────────────────────────────────────────────────────────┘ │
│                                                                      │
│  ┌───────────────────────────────────────────────────────────────┐ │
│  │                   PluginManager                                │ │
│  │  ┌─────────────────────────────────────────────────────────┐ │ │
│  │  │           Dependency Resolution                          │ │ │
│  │  │  ┌────────────────────┐  ┌────────────────────┐        │ │ │
│  │  │  │DependencyResolver  │  │   PluginLoader     │        │ │ │
│  │  │  │ - DAG validation   │  │ - dlopen/dlsym     │        │ │ │
│  │  │  │ - Topological sort │  │ - Symbol resolution│        │ │ │
│  │  │  │ - Cycle detection  │  │ - Error handling   │        │ │ │
│  │  │  └────────────────────┘  └────────────────────┘        │ │ │
│  │  └─────────────────────────────────────────────────────────┘ │ │
│  │                                                                │ │
│  │  ┌───────────────────────────────────────────────────────┐  │ │
│  │  │                 Loaded Plugins                         │  │ │
│  │  │  ┌──────────┐  ┌──────────┐  ┌──────────┐           │  │ │
│  │  │  │ Plugin A │  │ Plugin B │  │ Plugin C │    ...    │  │ │
│  │  │  │(.so/.dll)│  │(.so/.dll)│  │(.so/.dll)│           │  │ │
│  │  │  └──────────┘  └──────────┘  └──────────┘           │  │ │
│  │  │         Dynamically loaded at runtime                 │  │ │
│  │  └───────────────────────────────────────────────────────┘  │ │
│  └───────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────┘
```

## Flux de Données

```
                      ┌─────────────┐
                      │   main()    │
                      └──────┬──────┘
                             │
                             ▼
                   ┌──────────────────┐
                   │  Application     │
                   │  - initialize()  │
                   └────────┬─────────┘
                            │
         ┌──────────────────┼──────────────────┐
         │                  │                  │
         ▼                  ▼                  ▼
  ┌──────────┐      ┌──────────────┐   ┌──────────────┐
  │EventBus  │      │ServiceLocator│   │ResourceMgr   │
  └──────────┘      └──────────────┘   └──────────────┘
         │                  │                  │
         └──────────────────┼──────────────────┘
                            │
                            ▼
                   ┌─────────────────┐
                   │ PluginManager   │
                   │ - initialize()  │
                   └────────┬────────┘
                            │
              ┌─────────────┼─────────────┐
              │             │             │
              ▼             ▼             ▼
         ┌────────┐   ┌────────┐   ┌────────┐
         │Plugin A│   │Plugin B│   │Plugin C│
         │.init() │   │.init() │   │.init() │
         └────────┘   └────────┘   └────────┘
              │             │             │
              └─────────────┼─────────────┘
                            │
                            ▼
                   ┌─────────────────┐
                   │   Application   │
                   │   - run()       │
                   │   Main Loop     │
                   └────────┬────────┘
                            │
                   ┌────────▼────────┐
                   │  Frame Update   │
                   │                 │
                   │ 1. EventBus     │
                   │ 2. Modules      │
                   │ 3. Plugins      │
                   │ 4. onUpdate()   │
                   │ 5. onRender()   │
                   └─────────────────┘
```

## Cycle de Vie d'un Plugin

```
┌────────────────────────────────────────────────────────────────┐
│                    Plugin Lifecycle                             │
└────────────────────────────────────────────────────────────────┘

1. DISCOVERY
   │
   ├─> PluginManager scans plugin directory
   │   └─> Finds plugin_name.so/.dll
   │
   ▼

2. LOADING
   │
   ├─> PluginLoader::loadPlugin(path)
   │   ├─> dlopen() / LoadLibrary()
   │   ├─> dlsym("createPlugin")
   │   ├─> dlsym("destroyPlugin")
   │   └─> createPlugin() → IPlugin*
   │
   ▼

3. DEPENDENCY RESOLUTION
   │
   ├─> DependencyResolver::addPlugin(metadata)
   │   ├─> Check dependencies exist
   │   ├─> Check version constraints
   │   ├─> Build dependency graph (DAG)
   │   ├─> Detect cycles
   │   └─> Topological sort
   │
   ▼

4. INITIALIZATION
   │
   ├─> PluginManager::initializeAll()
   │   └─> For each plugin (in dependency order):
   │       ├─> Create PluginContext
   │       │   ├─> EventBus*
   │       │   ├─> ServiceLocator*
   │       │   └─> Application*
   │       │
   │       ├─> plugin->initialize(context)
   │       │   ├─> Subscribe to events
   │       │   ├─> Resolve services
   │       │   ├─> Register services
   │       │   └─> Initialize resources
   │       │
   │       └─> Mark as initialized
   │
   ▼

5. RUNTIME (Main Loop)
   │
   ├─> Application::run()
   │   └─> While running:
   │       ├─> Calculate deltaTime
   │       ├─> EventBus::processQueue()
   │       ├─> Update modules
   │       ├─> PluginManager::updateAll(deltaTime)
   │       │   └─> plugin->onUpdate(deltaTime)
   │       ├─> onUpdate(deltaTime)
   │       └─> onRender()
   │
   ▼

6. SHUTDOWN
   │
   ├─> Application::shutdown()
   │   ├─> PluginManager::unloadAll()
   │   │   └─> For each plugin (REVERSE order):
   │   │       ├─> plugin->shutdown()
   │   │       │   ├─> Unsubscribe from events
   │   │       │   ├─> Release resources
   │   │       │   └─> Cleanup state
   │   │       │
   │   │       ├─> destroyPlugin(plugin)
   │   │       └─> dlclose() / FreeLibrary()
   │   │
   │   ├─> Shutdown modules (reverse order)
   │   ├─> Clear ResourceManager
   │   ├─> Clear EventBus
   │   └─> Clear ServiceLocator
   │
   ▼

7. CLEANUP
   │
   └─> Memory freed, handles closed
```

## Communication entre Composants

### 1. EventBus (Pub/Sub)

```
Plugin A                EventBus                 Plugin B
   │                       │                        │
   │  subscribe("event")   │                        │
   │──────────────────────>│                        │
   │                       │   subscribe("event")   │
   │                       │<───────────────────────│
   │                       │                        │
   │  publish("event")     │                        │
   │──────────────────────>│                        │
   │                       │  callback("event")     │
   │                       │───────────────────────>│
   │  callback("event")    │                        │
   │<──────────────────────│                        │
```

### 2. ServiceLocator (Dependency Injection)

```
Plugin                ServiceLocator           Service
   │                         │                     │
   │  registerSingleton()    │                     │
   │─────────────────────────>│                     │
   │                         │                     │
   │                         │  store shared_ptr   │
   │                         │────────────────────>│
   │                         │                     │
   │  resolve<IService>()    │                     │
   │─────────────────────────>│                     │
   │                         │  get shared_ptr     │
   │                         │<────────────────────│
   │  shared_ptr<IService>   │                     │
   │<─────────────────────────│                     │
```

### 3. ResourceManager (Caching)

```
Plugin A              ResourceManager           Disk/Memory
   │                         │                       │
   │  load<Texture>("a.png") │                       │
   │─────────────────────────>│                       │
   │                         │  Check cache          │
   │                         │  (not found)          │
   │                         │                       │
   │                         │  Load from disk       │
   │                         │──────────────────────>│
   │                         │  texture data         │
   │                         │<──────────────────────│
   │                         │  Store in cache       │
   │                         │  (ref_count = 1)      │
   │  shared_ptr<Texture>    │                       │
   │<─────────────────────────│                       │
   │                         │                       │
   │                         │                       │
Plugin B                     │                       │
   │                         │                       │
   │  load<Texture>("a.png") │                       │
   │─────────────────────────>│                       │
   │                         │  Check cache          │
   │                         │  (FOUND!)             │
   │                         │  (ref_count = 2)      │
   │  shared_ptr<Texture>    │                       │
   │<─────────────────────────│                       │
   │  (same instance as A)   │                       │
```

## Thread Safety

### Composants Thread-Safe

```
┌─────────────────────┐
│    EventBus         │  ← std::mutex sur subscribe/publish
├─────────────────────┤
│  ServiceLocator     │  ← std::mutex sur register/resolve
├─────────────────────┤
│  ResourceManager    │  ← std::mutex sur load/get
├─────────────────────┤
│  PluginManager      │  ← std::mutex sur load/unload
└─────────────────────┘
```

### Stratégie de Locking

```cpp
// Exemple: EventBus::publish()
void publish(const std::string& name, const Event& event) {
    std::vector<Subscriber> subscribersCopy;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        // Copy subscribers under lock
        subscribersCopy = m_subscribers[name];
    }
    // Release lock before callbacks

    // Invoke callbacks without holding lock
    for (const auto& sub : subscribersCopy) {
        sub.callback(event);
    }
}
```

## Patterns de Dépendances

### Exemple 1: Plugin Simple Sans Dépendances

```
┌──────────────┐
│  MyPlugin    │
│              │
│ deps: []     │
└──────────────┘
```

### Exemple 2: Plugin avec Dépendances Linéaires

```
┌────────────┐     ┌────────────┐     ┌────────────┐
│  CorePlugin│────>│  UtilPlugin│────>│   MyPlugin │
│            │     │            │     │            │
│  Priority: │     │  Priority: │     │  Priority: │
│    1000    │     │    500     │     │    100     │
└────────────┘     └────────────┘     └────────────┘

Load order: CorePlugin → UtilPlugin → MyPlugin
```

### Exemple 3: Graphe de Dépendances Complexe

```
            ┌────────────┐
            │  CorePlugin│
            └─────┬──────┘
                  │
         ┌────────┴────────┐
         │                 │
         ▼                 ▼
    ┌────────┐        ┌────────┐
    │LogPlugin│        │NetPlugin│
    └────┬───┘        └────┬───┘
         │                 │
         └────────┬────────┘
                  │
                  ▼
            ┌──────────┐
            │  MyPlugin│
            └──────────┘

Load order (topological sort):
1. CorePlugin
2. LogPlugin, NetPlugin (parallel)
3. MyPlugin
```

### Exemple 4: Dépendance Circulaire (INVALIDE)

```
    ┌────────────┐
    │  PluginA   │
    └─────┬──────┘
          │
          ▼
    ┌────────────┐
    │  PluginB   │
    └─────┬──────┘
          │
          ▼
    ┌────────────┐
    │  PluginC   │
    └─────┬──────┘
          │
          └────────┐
                   │
    ┌──────────────▼┐
    │    PluginA    │ ← CYCLE DETECTED!
    └───────────────┘

DependencyResolver détecte et rejette ce graphe.
```

## Extension Points

### 1. Créer un Nouveau Type de Service

```cpp
// Définir l'interface
class IMyService {
public:
    virtual ~IMyService() = default;
    virtual void doSomething() = 0;
};

// Implémenter
class MyService : public IMyService {
    void doSomething() override { /* ... */ }
};

// Enregistrer
serviceLocator->registerSingleton<IMyService>(
    std::make_shared<MyService>()
);

// Utiliser dans un plugin
auto service = context.getServiceLocator()->resolve<IMyService>();
service->doSomething();
```

### 2. Créer un Nouveau Type d'Événement

```cpp
// Définir la structure de données
struct PlayerScoreEvent {
    int playerId;
    int score;
    float timestamp;
};

// Publier
PlayerScoreEvent data{1, 100, 3.14f};
Event event("player.scored", data);
eventBus->publish("player.scored", event);

// S'abonner
eventBus->subscribe("player.scored",
    [](const Event& e) {
        auto data = std::any_cast<PlayerScoreEvent>(e.data);
        std::cout << "Player " << data.playerId
                  << " scored " << data.score << std::endl;
    }
);
```

### 3. Créer un Nouveau Type de Ressource

```cpp
// Définir la ressource
class Texture {
    // Vos données
};

// Enregistrer un loader
resourceManager->registerLoader<Texture>(
    [](const std::string& path) {
        // Charger depuis le disque
        return std::make_shared<Texture>(path);
    }
);

// Utiliser
auto texture = resourceManager->load<Texture>("textures/player.png");
```

## Performance Considerations

### 1. Overhead de dlopen
- First load: ~1-10ms par plugin
- Subsequent loads (si en cache système): ~0.1-1ms

### 2. EventBus Dispatch
- Subscribe: O(log n) - insertion triée
- Publish: O(n) - itération sur subscribers
- Lock contention: minimale (copy-then-release)

### 3. ServiceLocator Resolve
- Singleton: O(log n) - map lookup
- Transient: O(log n) + factory invocation
- Cache hit: ~10-100 nanoseconds

### 4. ResourceManager Load
- Cache hit: O(log n) - map lookup
- Cache miss: O(log n) + disk I/O

### 5. Dependency Resolution
- O(V + E) où V = plugins, E = dépendances
- Topological sort: O(V + E)
- Une seule fois à l'initialisation

## Sécurité

### Validation de Plugins

```
PluginLoader
    │
    ├─> Vérifier l'extension (.so/.dll)
    ├─> Vérifier les symboles requis
    ├─> Vérifier la signature (TODO)
    ├─> Vérifier les permissions (TODO)
    └─> Sandbox (TODO)
```

### Isolation

- Chaque plugin a son propre handle dlopen
- RTLD_LOCAL pour ne pas polluer le namespace global
- Pas d'accès direct entre plugins (via EventBus/ServiceLocator)
- TODO: Sandboxing avec seccomp/AppArmor

## Debugging

### Outils Disponibles

```bash
# Lister les symboles exportés
nm -D plugin_name.so | grep -E "createPlugin|destroyPlugin"

# Vérifier les dépendances
ldd plugin_name.so

# Tracer le chargement
strace -e trace=open,openat ./test_app

# Profiling
valgrind --tool=callgrind ./test_app
```

### Logs Intégrés

```
[PluginManager] Loading plugin: ./plugins/example_plugin.so
[PluginLoader] Resolving symbols for: ExamplePlugin
[DependencyResolver] Checking dependencies for: ExamplePlugin
[DependencyResolver] Load order: [ExamplePlugin]
[PluginManager] Initializing plugin: ExamplePlugin
[ExamplePlugin] Initializing plugin...
[ExamplePlugin] Initialization complete!
```

---

Pour plus de détails, voir:
- [README.md](../README.md) - Guide d'utilisation
- [IMPLEMENTATION.md](IMPLEMENTATION.md) - Détails d'implémentation
- [PLUGIN_GUIDE.md](PLUGIN_GUIDE.md) - Guide de création de plugins

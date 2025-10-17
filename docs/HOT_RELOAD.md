# Plugin Hot Reload

The ModularCppFramework framework supports hot reloading of plugins, allowing you to update plugin code without restarting the application. This significantly speeds up development and iteration.

## Features

- **Automatic File Monitoring**: Watches plugin files for changes and reloads automatically
- **Manual Reload**: Trigger plugin reloads programmatically
- **State Persistence**: Plugin state can be serialized and restored across reloads
- **Dependency Handling**: Automatically reloads dependent plugins in correct order
- **Safe Execution**: Application pauses during reload to prevent crashes
- **Rollback Support**: Attempts to restore previous version if reload fails
- **Plugin-Aware Cleanup**: Automatically cleans up EventBus, ServiceLocator, and ResourceManager registrations

## Quick Start

### 1. Enable Hot Reload

```cpp
#include "core/Application.hpp"

class MyApp : public mcf::Application {
protected:
    bool onInitialize() override {
        // Enable hot reload with 1-second polling interval
        m_pluginManager.enableHotReload(std::chrono::milliseconds(1000));
        return true;
    }
};
```

### 2. Implement State Serialization (Optional)

Override these methods in your plugin to preserve state across reloads:

```cpp
class MyPlugin : public mcf::IPlugin {
private:
    int m_counter = 0;
    std::string m_data;

public:
    // Serialize state to string (JSON, CSV, binary, etc.)
    std::string serializeState() override {
        std::ostringstream oss;
        oss << m_counter << "," << m_data;
        return oss.str();
    }

    // Deserialize state from string
    void deserializeState(const std::string& state) override {
        std::istringstream iss(state);
        std::string token;

        if (std::getline(iss, token, ',')) {
            m_counter = std::stoi(token);
        }
        if (std::getline(iss, token, ',')) {
            m_data = token;
        }
    }

    // Optional: Called before reload
    void onBeforeReload() override {
        std::cout << "Preparing for reload..." << std::endl;
    }

    // Optional: Called after reload
    void onAfterReload() override {
        std::cout << "Reload complete!" << std::endl;
    }
};
```

### 3. Test Hot Reload

Run the included hot reload demo:

```bash
cd build
./bin/hot_reload_demo
```

Then in another terminal:

```bash
# Edit the plugin
nano ../plugins/hot_reload_example/HotReloadExamplePlugin.cpp

# Change the m_message string to something new

# Rebuild just the plugin
make hot_reload_example
```

Watch the plugin reload automatically with preserved state!

## Manual Reload

You can also trigger reloads programmatically:

```cpp
// Reload a specific plugin by name
if (pluginManager.reloadPlugin("MyPlugin")) {
    std::cout << "Reload successful!" << std::endl;
}

// Check if hot reload is enabled
if (pluginManager.isHotReloadEnabled()) {
    // ...
}

// Disable hot reload
pluginManager.disableHotReload();
```

## How It Works

### Reload Process

1. **Pause Application**: Application updates are paused to prevent mid-update crashes
2. **Save State**: `serializeState()` and `onBeforeReload()` called on affected plugins
3. **Identify Dependents**: Find all plugins that depend on the reloading plugin
4. **Shutdown**: Plugins shut down in reverse dependency order
5. **Cleanup**: Remove EventBus subscriptions, ServiceLocator services, and ResourceManager resources
6. **Unload**: Dynamically unload plugin shared libraries
7. **Reload**: Load new plugin binaries from disk
8. **Reinitialize**: Initialize plugins in dependency order
9. **Restore State**: `deserializeState()` and `onAfterReload()` called
10. **Resume**: Application updates resume

### Dependency Handling

If Plugin B depends on Plugin A, and you reload Plugin A:
- Both Plugin A and Plugin B are reloaded
- Plugin B shuts down first (dependent)
- Plugin A shuts down second (dependency)
- Plugin A reloads and initializes first
- Plugin B reloads and initializes second

This ensures plugins always see consistent dependencies.

### Safety Features

- **Application Pause**: Updates stop during reload to prevent race conditions
- **Transaction Rollback**: If reload fails, attempts to restore previous version
- **Name Validation**: Prevents loading different plugin with same name
- **Exception Handling**: Catches errors during reload process

## Plugin-Aware Cleanup

The framework automatically cleans up plugin-specific resources:

### EventBus

Use `subscribeWithPlugin()` to track subscriptions:

```cpp
bool initialize(mcf::PluginContext& context) override {
    context.getEventBus()->subscribeWithPlugin(
        "my.event",
        [this](const mcf::Event& e) { /* handler */ },
        100,  // priority
        getName()  // plugin ID for cleanup
    );
    return true;
}
```

All subscriptions are automatically removed during reload.

### ServiceLocator

Use `registerSingletonWithPlugin()` to track services:

```cpp
bool initialize(mcf::PluginContext& context) override {
    auto service = std::make_shared<MyService>();
    context.getServiceLocator()->registerSingletonWithPlugin<IMyService>(
        service,
        getName()  // plugin ID for cleanup
    );
    return true;
}
```

All services are automatically unregistered during reload.

### ResourceManager

Use `addWithPlugin()` to track resources:

```cpp
bool initialize(mcf::PluginContext& context) override {
    auto resource = std::make_shared<MyResource>();
    context.getResourceManager()->addWithPlugin<MyResource>(
        "my.resource",
        resource,
        getName()  // plugin ID for cleanup
    );
    return true;
}
```

All resources are automatically released during reload.

## Best Practices

### 1. Design for Reloadability

- Keep plugin initialization idempotent
- Don't rely on static/global state
- Clean up external resources in `shutdown()`

### 2. Serialize Critical State

- Identify state that should persist (counters, user data, etc.)
- Use simple serialization formats (CSV, JSON)
- Handle deserialization errors gracefully

### 3. Handle Dependencies

- Minimize inter-plugin dependencies
- Design plugins to handle dependency reloads
- Test dependent plugin reloads

### 4. Test Thoroughly

- Test reload under load
- Test reload with invalid state
- Test reload with missing dependencies
- Test rollback on failure

### 5. Performance Considerations

- Reload is not instant (typically 10-100ms)
- File watching adds minimal overhead (configurable polling)
- Consider disabling in production if not needed

## Limitations

### Current Limitations

1. **No Live Patching**: Full shutdown/reinit required, not true "live" patching
2. **No Version Migration**: State format changes require manual handling
3. **No Undo/Redo**: Can't roll back to arbitrary previous versions
4. **File System Polling**: Uses polling instead of OS file events (cross-platform)
5. **Module Reload**: Static modules cannot be hot reloaded

### Known Issues

- **Large Plugins**: Reload time proportional to plugin size
- **Resource Handles**: Raw pointers to plugin objects become invalid after reload
- **Thread Safety**: Ensure no threads are using plugin during reload
- **Platform Differences**: Plugin binary compatibility varies by compiler

## Troubleshooting

### Reload Fails

**Symptom**: Plugin fails to reload, application continues with old version

**Solutions**:
- Check plugin compiles successfully
- Verify no syntax/link errors
- Check console for error messages
- Ensure plugin name hasn't changed

### State Lost

**Symptom**: Plugin state resets to defaults after reload

**Solutions**:
- Implement `serializeState()` and `deserializeState()`
- Check serialization format is correct
- Add logging to verify state preservation

### Crash During Reload

**Symptom**: Application crashes when plugin reloads

**Solutions**:
- Ensure no modules hold raw pointers to plugin objects
- Check plugin cleanup in `shutdown()`
- Verify no background threads accessing plugin
- Test with `pause()` / `resume()` manually first

### File Not Detected

**Symptom**: File changes not triggering reload

**Solutions**:
- Increase poll interval: `enableHotReload(std::chrono::seconds(2))`
- Check file watcher is started: `isHotReloadEnabled()`
- Verify plugin path is correct in `m_pluginPaths`
- Ensure file write is fully complete (some editors write in stages)

## Examples

See the complete working example in:
- [examples/hot_reload_demo.cpp](../examples/hot_reload_demo.cpp)
- [plugins/hot_reload_example/](../plugins/hot_reload_example/)

Run the demo:
```bash
cd build
./bin/hot_reload_demo
```

## Advanced Topics

### Custom State Serialization

For complex state, consider using a library like nlohmann/json:

```cpp
#include <nlohmann/json.hpp>

std::string serializeState() override {
    nlohmann::json j;
    j["counter"] = m_counter;
    j["data"] = m_data;
    j["values"] = m_values;  // vector<int>
    return j.dump();
}

void deserializeState(const std::string& state) override {
    auto j = nlohmann::json::parse(state);
    m_counter = j["counter"];
    m_data = j["data"];
    m_values = j["values"].get<std::vector<int>>();
}
```

### Reload Notifications

Subscribe to reload events in other plugins:

```cpp
bool initialize(mcf::PluginContext& context) override {
    context.getEventBus()->subscribe("plugin.reload",
        [this](const mcf::Event& e) {
            auto pluginName = std::any_cast<std::string>(e.data);
            onPluginReloaded(pluginName);
        }
    );
    return true;
}
```

### Conditional Reload

Only reload specific plugins based on environment:

```cpp
#ifdef DEBUG_BUILD
    pluginManager.enableHotReload();
#endif
```

## See Also

- [ARCHITECTURE.md](ARCHITECTURE.md) - System architecture overview
- [PLUGIN_GUIDE.md](PLUGIN_GUIDE.md) - Plugin development guide
- [IMPLEMENTATION.md](IMPLEMENTATION.md) - Technical implementation details

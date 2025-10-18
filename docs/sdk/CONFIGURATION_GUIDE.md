# Configuration System Guide

## Overview

The ModularCppFramework framework includes a complete JSON-based configuration system that allows applications and plugins to load, save, and manage configuration data at runtime.

## Features

- **JSON Parsing**: Built-in JSON parser with full JSON support
- **Hierarchical Configuration**: Access nested values using dot notation (`"section.subsection.key"`)
- **Type-Safe Access**: Type-safe getters for common types (string, int, float, bool, array, object)
- **Change Notifications**: Register callbacks to watch for configuration changes
- **Thread-Safe**: All operations are thread-safe using mutexes
- **Auto-Save**: Optional automatic saving on modifications
- **Plugin Integration**: Plugins can access configuration through PluginContext

## Core Components

### 1. JsonValue

Represents any JSON value type:
- Null
- Boolean
- Integer (int64_t)
- Float (double)
- String
- Array (vector of JsonValues)
- Object (map of string keys to JsonValues)

**Example:**
```cpp
#include "core/JsonValue.hpp"

using namespace mcf;

// Create values
JsonValue nullValue(nullptr);
JsonValue boolValue(true);
JsonValue intValue(42);
JsonValue floatValue(3.14);
JsonValue stringValue("hello");

// Create array
JsonArray arr;
arr.push_back(JsonValue(1));
arr.push_back(JsonValue(2));
JsonValue arrayValue(arr);

// Create object
JsonObject obj;
obj["name"] = JsonValue("MyApp");
obj["version"] = JsonValue("1.0.0");
JsonValue objectValue(obj);

// Access values
std::string str = stringValue.asString();
int num = intValue.asInt();
bool flag = boolValue.asBool();

// Check types
if (value.isObject()) {
    const JsonObject& obj = value.asObject();
}

// Navigate objects and arrays
JsonValue nested = objectValue["name"];
JsonValue element = arrayValue[0];
```

### 2. JsonParser

Parses JSON from strings and files:

**Example:**
```cpp
#include "core/JsonParser.hpp"

using namespace mcf;

// Parse from string
std::string jsonStr = R"({"name": "MyApp", "version": "1.0.0"})";
JsonValue value = JsonParser::parse(jsonStr);

// Parse from file
JsonValue config = JsonParser::parseFile("config.json");

// Write to file
JsonParser::writeFile("output.json", value);
```

### 3. ConfigurationManager

Main configuration management class:

**Example:**
```cpp
#include "core/ConfigurationManager.hpp"

using namespace mcf;

ConfigurationManager config;

// Load configuration from file
config.load("config.json");

// Get values with dot notation
std::string name = config.getString("application.name", "DefaultApp");
int fps = config.getInt("application.targetFPS", 60);
bool vsync = config.getBool("application.vsync", true);
double timeout = config.getFloat("network.timeout", 5.0);

// Get nested objects
JsonValue rendering = config.get("rendering.window");
if (rendering.isObject()) {
    int width = rendering["width"].asInt(1280);
    int height = rendering["height"].asInt(720);
}

// Set values
config.set("application.targetFPS", JsonValue(120));
config.set("logging.level", JsonValue("debug"));

// Check if key exists
if (config.has("custom.setting")) {
    // Key exists
}

// Remove keys
config.remove("temporary.value");

// Save configuration
config.save("config.json");

// Save to different file
config.save("config_backup.json");

// Reload from file
config.reload();
```

## Integration with Application

### Basic Setup

```cpp
#include "core/Application.hpp"

using namespace mcf;

int main() {
    // Create application config
    ApplicationConfig appConfig;
    appConfig.name = "MyApp";
    appConfig.configFile = "./config.json";  // Enable config file loading
    appConfig.autoLoadPlugins = true;
    appConfig.targetFPS = 60;

    Application app(appConfig);

    // Initialize (automatically loads config.json if specified)
    app.initialize();

    // Access configuration
    auto* config = app.getConfigurationManager();
    std::string customValue = config->getString("custom.setting", "default");

    app.run();
    app.shutdown();

    return 0;
}
```

### Custom Application with Configuration

```cpp
class MyApp : public Application {
public:
    MyApp() : Application() {
        m_config.configFile = "./config.json";
    }

protected:
    bool onInitialize() override {
        auto* config = getConfigurationManager();

        // Load custom settings
        m_windowWidth = config->getInt("window.width", 1280);
        m_windowHeight = config->getInt("window.height", 720);
        m_enableDebug = config->getBool("debug.enabled", false);

        // Watch for configuration changes
        config->watch("window.width", [this](const std::string& key, const JsonValue& value) {
            m_windowWidth = value.asInt();
            this->onWindowResize(m_windowWidth, m_windowHeight);
        });

        return true;
    }

    void onWindowResize(int width, int height) {
        // Handle window resize
    }

private:
    int m_windowWidth = 1280;
    int m_windowHeight = 720;
    bool m_enableDebug = false;
};
```

## Plugin Configuration

Plugins can access configuration through their `PluginContext`:

```cpp
class MyPlugin : public mcf::IPlugin {
private:
    mcf::PluginMetadata m_metadata;
    mcf::PluginContext m_context;
    bool m_initialized = false;

    // Plugin settings
    float m_updateInterval = 1.0f;
    bool m_debugMode = false;

public:
    MyPlugin() {
        m_metadata.name = "MyPlugin";
        m_metadata.version = "1.0.0";
    }

    bool initialize(const mcf::PluginContext& context) override {
        m_context = context;
        m_initialized = true;

        // Access configuration
        auto* config = m_context.getConfigurationManager();
        if (config) {
            // Load plugin-specific settings
            std::string pluginKey = "plugins." + m_metadata.name;

            m_updateInterval = config->getFloat(pluginKey + ".updateInterval", 1.0f);
            m_debugMode = config->getBool(pluginKey + ".debugMode", false);

            // Watch for changes
            config->watch(pluginKey + ".updateInterval",
                [this](const std::string& key, const JsonValue& value) {
                    m_updateInterval = value.asFloat(1.0f);
                });
        }

        return true;
    }

    // ... other IPlugin methods
};
```

## Example Configuration File

```json
{
  "application": {
    "name": "ModularApp",
    "version": "1.0.0",
    "pluginDirectory": "./plugins",
    "autoLoadPlugins": true,
    "autoInitPlugins": true,
    "targetFPS": 60,
    "vsync": true,
    "threadPoolSize": 0
  },
  "logging": {
    "enabled": true,
    "level": "info",
    "file": "app.log",
    "console": true
  },
  "plugins": {
    "ExamplePlugin": {
      "enabled": true,
      "priority": 100,
      "settings": {
        "updateInterval": 1.0,
        "debugMode": false
      }
    }
  },
  "rendering": {
    "window": {
      "width": 1280,
      "height": 720,
      "title": "Modular  IA Application",
      "fullscreen": false,
      "resizable": true
    },
    "graphics": {
      "validationLayers": true,
      "msaaSamples": 4,
      "maxFramesInFlight": 2
    }
  },
  "resources": {
    "texturesPath": "./assets/textures",
    "modelsPath": "./assets/models",
    "shadersPath": "./assets/shaders",
    "cacheSize": 512
  }
}
```

## Advanced Features

### Configuration Change Callbacks

Monitor specific configuration keys for changes:

```cpp
ConfigurationManager config;

// Register callback
config.watch("application.targetFPS", [](const std::string& key, const JsonValue& value) {
    std::cout << key << " changed to: " << value.asInt() << std::endl;
});

// Trigger callback
config.set("application.targetFPS", JsonValue(144));
```

### Working with Arrays

```cpp
// Set array
JsonArray colors;
colors.push_back(JsonValue("red"));
colors.push_back(JsonValue("green"));
colors.push_back(JsonValue("blue"));
config.set("ui.colors", JsonValue(colors));

// Get array
JsonArray retrievedColors = config.getArray("ui.colors");
for (const auto& color : retrievedColors) {
    std::cout << color.asString() << std::endl;
}
```

### Working with Objects

```cpp
// Set nested object
JsonObject window;
window["width"] = JsonValue(1920);
window["height"] = JsonValue(1080);
window["fullscreen"] = JsonValue(true);

JsonObject rendering;
rendering["window"] = JsonValue(window);

config.set("rendering", JsonValue(rendering));

// Get nested value
int width = config.getInt("rendering.window.width", 1280);
```

### Default Values

Always provide default values to handle missing configuration:

```cpp
// Good practice - always provide defaults
int fps = config.getInt("application.targetFPS", 60);
std::string name = config.getString("application.name", "DefaultApp");
bool enabled = config.getBool("feature.enabled", false);

// Check existence first
if (config.has("custom.feature")) {
    // Feature is configured
    auto value = config.get("custom.feature");
} else {
    // Use default behavior
}
```

## Thread Safety

All `ConfigurationManager` operations are thread-safe:

```cpp
ConfigurationManager config;

// Thread 1
std::thread t1([&config]() {
    config.set("counter", JsonValue(42));
});

// Thread 2
std::thread t2([&config]() {
    int value = config.getInt("counter", 0);
});

t1.join();
t2.join();
```

## Performance Tips

1. **Cache Frequently Used Values**: Don't query the same configuration value repeatedly in loops
   ```cpp
   // Bad
   for (int i = 0; i < 1000; i++) {
       int fps = config->getInt("application.targetFPS", 60);
   }

   // Good
   int fps = config->getInt("application.targetFPS", 60);
   for (int i = 0; i < 1000; i++) {
       // Use cached fps
   }
   ```

2. **Use Watch Callbacks**: Instead of polling for changes, use watch callbacks

3. **Minimize Lock Contention**: Batch configuration reads/writes when possible

## Error Handling

```cpp
try {
    // Parse JSON
    JsonValue value = JsonParser::parse(jsonString);
} catch (const std::exception& e) {
    std::cerr << "JSON parse error: " << e.what() << std::endl;
}

// File operations return bool
if (!config.load("config.json")) {
    // File doesn't exist or is invalid - use defaults
}

if (!config.save("config.json")) {
    std::cerr << "Failed to save configuration" << std::endl;
}
```

## Best Practices

1. **Use Dot Notation**: Access nested values with `"section.subsection.key"`
2. **Always Provide Defaults**: Never assume a configuration key exists
3. **Document Your Configuration**: Add comments in your code explaining what each setting does
4. **Version Your Config**: Include a version field to handle schema changes
5. **Validate Values**: Check ranges and validity after loading configuration
6. **Watch for Critical Changes**: Use callbacks for settings that require immediate action
7. **Separate Concerns**: Group related settings in sections (rendering, logging, network, etc.)

## Files Reference

- **JsonValue.hpp**: JSON value representation
- **JsonParser.hpp**: JSON parsing and serialization
- **ConfigurationManager.hpp**: Configuration management
- **Application.hpp**: Application integration
- **PluginContext.hpp**: Plugin access to configuration

## See Also

- [Architecture Guide](ARCHITECTURE.md)
- [Plugin Development Guide](PLUGIN_GUIDE.md)
- [API Documentation](API.md)

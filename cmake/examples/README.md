# CMake Examples - MCF Plugin Generator

This directory contains example scripts demonstrating how to use the MCF Plugin Generator.

## Examples

### generate-plugin-example.cmake

Demonstrates how to generate plugins directly from CMake scripts.

**Usage:**
```bash
cmake -P cmake/examples/generate-plugin-example.cmake
```

This will generate 4 example plugins:
1. **BasicPlugin** - Basic plugin with IPlugin only
2. **PhysicsPlugin** - Realtime plugin with IRealtimeUpdatable
3. **NotificationPlugin** - Event-driven plugin with IEventDriven
4. **GameLogicPlugin** - Full plugin with both interfaces and dependencies

## Shell Script Examples

For most use cases, the shell script is easier:

```bash
# Basic plugin
./cmake/create-plugin.sh -n MyPlugin

# Realtime plugin
./cmake/create-plugin.sh -n PhysicsPlugin -r

# Event-driven plugin
./cmake/create-plugin.sh -n NotificationPlugin -e

# Full-featured plugin
./cmake/create-plugin.sh \
    -n GameLogicPlugin \
    -v 3.0.0 \
    -a "Game Team" \
    -d "Main game logic" \
    -p 500 \
    -r -e
```

## CMake Integration

You can also integrate plugin generation into your CMakeLists.txt:

```cmake
include(cmake/MCFPluginGenerator.cmake)

# Generate plugin during configure step
mcf_generate_plugin(
    NAME MyPlugin
    VERSION 1.0.0
    REALTIME
)
```

**Note:** This approach generates the plugin every time you run `cmake ..`, which might not be desirable. The shell script approach is recommended for one-time plugin creation.

## Custom Templates

To customize the generated plugin templates, edit the files in `cmake/templates/`:
- `Plugin.cpp.in` - Plugin source code template
- `PluginCMakeLists.txt.in` - CMakeLists.txt template
- `PluginREADME.md.in` - README.md template

Template variables available:
- `@PLUGIN_NAME@` - Plugin class name
- `@PLUGIN_NAME_LOWER@` - Plugin name in lowercase
- `@PLUGIN_NAME_UPPER@` - Plugin name in uppercase
- `@PLUGIN_VERSION@` - Plugin version
- `@PLUGIN_AUTHOR@` - Plugin author
- `@PLUGIN_DESCRIPTION@` - Plugin description
- `@PLUGIN_PRIORITY@` - Load priority
- `@PLUGIN_INTERFACES@` - Interface inheritance string
- `@PLUGIN_INTERFACE_INCLUDES@` - Include directives for interfaces
- `@PLUGIN_INTERFACE_METHODS@` - Interface method implementations
- `@PLUGIN_DEPS_JSON@` - Dependencies as JSON array

# Application Packaging Guide

This guide explains how to package your MCF applications for distribution to end-users.

## Overview

ModularCppFramework provides CMake utilities to package your applications into distributable archives. This works whether you're:
- **Developing within the MCF repository** (cloned framework)
- **Using MCF as an external dependency** (installed SDK)

The packaging system creates `.tar.gz` archives containing:
- Compiled executables
- Plugin libraries
- Configuration files
- Resource directories
- README documentation

## Packaging System Components

MCF provides two packaging functions:

1. **`mcf_package_application()`** - Package a single application
2. **`mcf_package_application_bundle()`** - Package multiple applications together

Both functions are available via `cmake/MCFPackaging.cmake`, which is automatically included when you use `find_package(ModularCppFramework)`.

---

## Packaging a Single Application

Use `mcf_package_application()` to create a distributable package for one application.

### Basic Usage

```cmake
include(cmake/MCFPackaging.cmake)  # Only needed if not using find_package()

mcf_package_application(
    TARGET my_app
    VERSION 1.0.0
    OUTPUT_NAME "MyApplication"
)
```

This creates:
- A CMake target: `package-my_app`
- An archive: `MyApplication-1.0.0-linux-x86_64.tar.gz`

### Build Command

```bash
cmake --build . --target package-my_app
```

### Package Structure

```
MyApplication-1.0.0/
├── bin/
│   └── my_app
├── plugins/
├── config/
├── resources/
└── README.txt
```

### Full Example with Plugins and Resources

```cmake
mcf_package_application(
    TARGET game_server
    VERSION 2.3.1
    OUTPUT_NAME "GameServer"
    PLUGINS
        auth_plugin
        database_plugin
        game_logic_plugin
    CONFIG_FILES
        config/server.json
        config/database.json
    RESOURCES
        assets/maps/
        assets/textures/
)
```

This creates a complete distributable package:

```bash
cmake --build . --target package-game_server
# Creates: GameServer-2.3.1-Linux-x86_64.tar.gz
```

### Parameters

| Parameter | Required | Description |
|-----------|----------|-------------|
| `TARGET` | Yes | Name of the executable target |
| `VERSION` | Yes | Application version (e.g., "1.0.0") |
| `OUTPUT_NAME` | No | Package name (defaults to TARGET) |
| `PLUGINS` | No | List of plugin targets to include |
| `CONFIG_FILES` | No | Configuration files to copy (relative to source dir) |
| `RESOURCES` | No | Resource directories to copy (relative to source dir) |

---

## Packaging Multiple Applications (Bundle)

Use `mcf_package_application_bundle()` to package multiple related applications together.

### Basic Usage

```cmake
mcf_package_application_bundle(
    BUNDLE_NAME "MyApplicationSuite"
    VERSION 1.0.0
    TARGETS
        server_app
        client_app
        admin_tool
)
```

This creates:
- A CMake target: `package-myapplicationsuite` (lowercase)
- An archive: `MyApplicationSuite-1.0.0-linux-x86_64.tar.gz`

### Build Command

```bash
cmake --build . --target package-myapplicationsuite
```

### Package Structure

```
MyApplicationSuite-1.0.0/
├── bin/
│   ├── server_app
│   ├── client_app
│   └── admin_tool
├── plugins/
├── config/
├── resources/
└── README.txt
```

### Full Example (MCF Examples)

This is how MCF packages its own example applications:

```cmake
mcf_package_application_bundle(
    BUNDLE_NAME "MCF-Examples"
    VERSION ${PROJECT_VERSION}
    TARGETS
        logger_example
        hot_reload_demo
        realtime_app_example
        event_driven_app_example
        profiling_example
        filesystem_example
        networking_server_example
        networking_client_example
    PLUGINS
        example_plugin
        hot_reload_example
    DESCRIPTION "ModularCppFramework Examples - Demonstrating MCF features"
    APPLICATIONS_INFO
        "logger_example=Logger module with JSON config"
        "hot_reload_demo=Plugin hot-reload demonstration"
        "realtime_app_example=Fixed timestep realtime loop"
        "event_driven_app_example=Event-driven architecture"
        "profiling_example=Performance profiling"
        "filesystem_example=Filesystem utilities"
        "networking_server_example=TCP server"
        "networking_client_example=TCP client"
)
```

Build and extract:

```bash
cmake --build . --target package-mcf-examples
tar -xzf MCF-Examples-1.0.2-Linux-x86_64.tar.gz
cd MCF-Examples-1.0.2
./bin/logger_example
```

### Parameters

| Parameter | Required | Description |
|-----------|----------|-------------|
| `BUNDLE_NAME` | Yes | Bundle name (e.g., "MyAppSuite") |
| `VERSION` | Yes | Bundle version (e.g., "1.0.0") |
| `TARGETS` | Yes | List of application targets to bundle |
| `PLUGINS` | No | List of plugin targets to include |
| `CONFIG_FILES` | No | Configuration files to copy |
| `RESOURCES` | No | Resource directories to copy |
| `DESCRIPTION` | No | Bundle description for README |
| `APPLICATIONS_INFO` | No | Per-app descriptions: `"app=Description"` |

---

## Using Packaging in External Projects

When you install MCF as a dependency (via `make install`, Conan, vcpkg, etc.), the packaging utilities are automatically available.

### Project Structure

```
my-project/
├── CMakeLists.txt
├── src/
│   └── main.cpp
├── plugins/
│   └── my_plugin/
│       ├── CMakeLists.txt
│       └── plugin.cpp
└── config/
    └── app.json
```

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyProject VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

# Find MCF (automatically includes MCFPackaging.cmake)
find_package(ModularCppFramework 1.0 REQUIRED)

# Build your application
add_executable(my_app src/main.cpp)
target_link_libraries(my_app PRIVATE mcf::core mcf::logger mcf::networking)

# Build your plugins
add_subdirectory(plugins)

# Package your application
mcf_package_application(
    TARGET my_app
    VERSION ${PROJECT_VERSION}
    OUTPUT_NAME "MyApplication"
    PLUGINS my_plugin
    CONFIG_FILES config/app.json
)
```

### Build and Package

```bash
mkdir build && cd build
cmake ..
cmake --build .
cmake --build . --target package-my_app

# Creates: MyApplication-1.0.0-Linux-x86_64.tar.gz
```

### Verify Package

```bash
tar -tzf MyApplication-1.0.0-Linux-x86_64.tar.gz

MyApplication-1.0.0/
MyApplication-1.0.0/bin/
MyApplication-1.0.0/bin/my_app
MyApplication-1.0.0/plugins/
MyApplication-1.0.0/plugins/my_plugin.so
MyApplication-1.0.0/config/
MyApplication-1.0.0/config/app.json
MyApplication-1.0.0/README.txt
```

---

## Platform-Specific Packaging

The packaging system automatically adapts to your platform:

### Linux
```
MyApp-1.0.0-Linux-x86_64.tar.gz
```
- Plugins: `.so` extension
- Executables: no extension

### macOS
```
MyApp-1.0.0-Darwin-arm64.tar.gz
```
- Plugins: `.dylib` extension
- Executables: no extension

### Windows
```
MyApp-1.0.0-Windows-x86_64.tar.gz
```
- Plugins: `.dll` extension
- Executables: `.exe` extension

---

## README Generation

Each package includes an auto-generated `README.txt` file.

### Single Application README

```
MyApplication 1.0.0
==================================================

Installation:
1. Extract this archive to your desired location
2. Run: ./bin/my_app

Directory Structure:
- bin/        - Executable files
- plugins/    - Plugin libraries
- config/     - Configuration files
- resources/  - Application resources

For more information, visit the project documentation.
```

### Bundle README (with descriptions)

```
MCF-Examples v1.0.2
====================

ModularCppFramework Examples - Demonstrating MCF features

Applications:
  ./bin/logger_example  - Logger module with JSON config
  ./bin/hot_reload_demo  - Plugin hot-reload demonstration
  ./bin/realtime_app_example  - Fixed timestep realtime loop
  ./bin/event_driven_app_example  - Event-driven architecture
  ./bin/profiling_example  - Performance profiling
  ./bin/filesystem_example  - Filesystem utilities
  ./bin/networking_server_example  - TCP server
  ./bin/networking_client_example  - TCP client

Usage:
  Run from this directory: ./bin/<application_name>
```

---

## Advanced Use Cases

### Packaging for Multiple Platforms

Use CI/CD to build packages for different platforms:

```yaml
# .github/workflows/package.yml
name: Package Applications

on: [push, release]

jobs:
  package-linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Build and Package
        run: |
          mkdir build && cd build
          cmake -DCMAKE_BUILD_TYPE=Release ..
          cmake --build .
          cmake --build . --target package-my_app
      - name: Upload Package
        uses: actions/upload-artifact@v3
        with:
          name: linux-package
          path: build/*.tar.gz

  package-windows:
    runs-on: windows-latest
    # Similar steps for Windows...

  package-macos:
    runs-on: macos-latest
    # Similar steps for macOS...
```

### Conditional Plugin Inclusion

```cmake
# Only include debug plugins in debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(APP_PLUGINS core_plugin debug_plugin profiler_plugin)
else()
    set(APP_PLUGINS core_plugin)
endif()

mcf_package_application(
    TARGET my_app
    VERSION ${PROJECT_VERSION}
    PLUGINS ${APP_PLUGINS}
)
```

### Version from Git Tags

```cmake
# Get version from git tag
execute_process(
    COMMAND git describe --tags --abbrev=0
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

mcf_package_application(
    TARGET my_app
    VERSION ${GIT_VERSION}
    OUTPUT_NAME "MyApp"
)
```

---

## Testing Packages

Always test your packages before distribution:

```bash
# Build package
cmake --build . --target package-my_app

# Extract to temporary directory
mkdir /tmp/test-package
tar -xzf MyApp-1.0.0-Linux-x86_64.tar.gz -C /tmp/test-package

# Test execution
cd /tmp/test-package/MyApp-1.0.0
./bin/my_app

# Verify plugins load
ldd bin/my_app  # Check dependencies
ls -lh plugins/ # Verify plugins included
```

---

## Troubleshooting

### Package Target Not Found

**Problem:** `make: *** No rule to make target 'package-my_app'`

**Solution:**
1. Make sure you called `mcf_package_application()` with `TARGET my_app`
2. Check that MCFPackaging.cmake is included
3. Reconfigure: `cmake ..`

### Missing Plugins in Package

**Problem:** Plugins not copied to package

**Solution:**
1. Verify plugin targets are built: `make my_plugin`
2. Check `PLUGINS` parameter lists valid targets
3. Ensure plugins have dependencies: `add_dependencies(package-my_app my_plugin)`

### Wrong Package Directory

**Problem:** Package structure is incorrect

**Solution:**
- Config files are relative to `CMAKE_SOURCE_DIR`
- Use `config/app.json`, not `/absolute/path/config/app.json`
- Resources must be directories, not individual files

### Platform Name Issues

**Problem:** Package has wrong platform name

**Solution:**
- Platform is auto-detected from `CMAKE_SYSTEM_NAME` and `CMAKE_SYSTEM_PROCESSOR`
- Override if needed: Cross-compilation scenarios may need manual platform strings

---

## Best Practices

1. **Version Everything**: Use semantic versioning (MAJOR.MINOR.PATCH)
2. **Test Packages**: Always extract and test before distribution
3. **Minimal Packages**: Only include necessary plugins and resources
4. **Document Usage**: Add custom README or documentation in resources
5. **Automate CI/CD**: Use GitHub Actions / GitLab CI to build multi-platform packages
6. **Sign Releases**: Consider signing packages for security (gpg, codesign)

---

## Reference: Complete Example

Here's a complete real-world example packaging a multiplayer game server:

```cmake
cmake_minimum_required(VERSION 3.16)
project(GameServer VERSION 2.5.1)

set(CMAKE_CXX_STANDARD 17)

# Find dependencies
find_package(ModularCppFramework 1.0 REQUIRED)

# Build server application
add_executable(game_server
    src/main.cpp
    src/server.cpp
    src/game_logic.cpp
)

target_link_libraries(game_server PRIVATE
    mcf::core
    mcf::networking
    mcf::profiling
)

# Build plugins
add_subdirectory(plugins/auth)
add_subdirectory(plugins/database)
add_subdirectory(plugins/game_modes)

# Package for distribution
mcf_package_application(
    TARGET game_server
    VERSION ${PROJECT_VERSION}
    OUTPUT_NAME "GameServer"
    PLUGINS
        auth_plugin
        database_plugin
        deathmatch_mode
        capture_flag_mode
    CONFIG_FILES
        config/server.json
        config/database.json
        config/game_modes.json
    RESOURCES
        assets/maps/
        assets/scripts/
)
```

Build and distribute:

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j4
cmake --build . --target package-game_server

# Distribute:
scp GameServer-2.5.1-Linux-x86_64.tar.gz user@server:/opt/
```

---

## See Also

- [Quick Start Guide](QUICK_START.md) - Getting started with MCF
- [Plugin Guide](PLUGIN_GUIDE.md) - Creating plugins
- [Packaging Guide](PACKAGING.md) - Distributing MCF SDK
- [Examples](../README.md#examples) - Example applications

---

**Note:** The packaging system is designed to work seamlessly whether you're developing within the MCF repository or using it as an external dependency. All examples shown work in both scenarios.

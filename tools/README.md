# MCF Tools - Cross-Platform Scripts

This directory contains **Python-based tools** for generating plugins, applications, and packaging for ModularCppFramework.

## Platform Support

All tools are **cross-platform Python scripts** that work on:
- ✅ **Windows** - Use `python tools/script.py`
- ✅ **Linux** - Use `python3 tools/script.py`
- ✅ **macOS** - Use `python3 tools/script.py`

## Requirements

- **Python 3.6 or higher** (required)
- **CMake** (must be in PATH)
- No additional dependencies needed

## Available Tools

### 1. Plugin Generator

Create new MCF plugins with all necessary boilerplate code.

**Usage:**
```bash
# Linux/macOS
python3 create-plugin.py -n MyPlugin [options]

# Windows
python create-plugin.py -n MyPlugin [options]
```

**Options:**
- `-n, --name` - Plugin name (required)
- `-v, --version` - Plugin version (default: 1.0.0)
- `-a, --author` - Plugin author
- `-d, --description` - Plugin description
- `-p, --priority` - Load priority (default: 100)
- `-r, --realtime` - Add IRealtimeUpdatable interface
- `-e, --event-driven` - Add IEventDriven interface
- `-o, --output` - Output directory
- `-h, --help` - Show help

**Examples:**
```bash
# Basic plugin
python3 create-plugin.py -n MyPlugin

# Realtime plugin
python3 create-plugin.py -n PhysicsPlugin -r

# Full plugin with all features
python3 create-plugin.py -n NetworkPlugin -r -e -p 200

# Windows examples
python create-plugin.py -n MyPlugin
python create-plugin.py -n PhysicsPlugin -r
```

### 2. Application Generator

Create new MCF applications with optional modules and features.

**Usage:**
```bash
# Linux/macOS
python3 create-application.py -n MyApp [options]

# Windows
python create-application.py -n MyApp [options]
```

**Options:**
- `-n, --name` - Application name (required)
- `-v, --version` - Application version (default: 1.0.0)
- `-a, --author` - Application author
- `-d, --description` - Application description
- `-m, --modules` - Comma-separated modules (logger,networking,profiling,realtime)
- `-p, --plugins` - Comma-separated plugins to load
- `-r, --realtime` - Add realtime update loop
- `-e, --event-driven` - Add event-driven architecture
- `-c, --config` - Generate config.json file
- `-o, --output` - Output directory
- `-h, --help` - Show help

**Examples:**
```bash
# Basic application
python3 create-application.py -n MyApp

# Application with modules
python3 create-application.py -n MyGame -r -c -m logger,profiling

# Full application
python3 create-application.py -n NetworkApp -r -e -c -m logger,networking,profiling

# Windows examples
python create-application.py -n MyApp
python create-application.py -n MyGame -r -c -m logger,profiling
```

### 3. Application Packaging

Package MCF applications into distributable archives.

**Usage:**
```bash
# Linux/macOS
python3 package-application.py -t package-my_app [options]

# Windows
python package-application.py -t package-my_app [options]
```

**Options:**
- `-t, --target` - Package target name (required)
- `-b, --build-dir` - Build directory (default: build)
- `-c, --config` - Build configuration (Debug, Release, RelWithDebInfo, MinSizeRel)
- `-o, --output` - Copy package to output directory
- `-j, --jobs` - Number of parallel build jobs
- `--clean` - Clean build directory before building
- `--extract` - Extract package after building for verification
- `--test` - Run basic tests on the extracted package
- `-v, --verbose` - Enable verbose output
- `-h, --help` - Show help

**Examples:**
```bash
# Package application
python3 package-application.py -t package-my_app

# Package with clean build
python3 package-application.py -t package-my_app --clean

# Package, extract, and test
python3 package-application.py -t package-my_app --extract --test

# Package and copy to distribution directory
python3 package-application.py -t package-my_app -o /tmp/dist

# Windows examples
python package-application.py -t package-my_app
python package-application.py -t package-my_app --extract --test
```

## Platform-Specific Notes

### Windows

When using Python scripts on Windows:
- Use `python` instead of `python3`
- Path separators are automatically handled
- Packages are created as `.zip` files
- Ensure CMake is in your PATH

**Example:**
```cmd
python create-plugin.py -n MyPlugin
python create-application.py -n MyApp
python package-application.py -t package-my_app
```

### Linux/macOS

Use `python3` command:

```bash
python3 create-plugin.py -n MyPlugin
python3 create-application.py -n MyApp
python3 package-application.py -t package-my_app
```

**Note:** Scripts can also be made executable with `chmod +x *.py` and run directly:
```bash
./create-plugin.py -n MyPlugin
```

## Troubleshooting

### "CMake not found"
Ensure CMake is installed and in your PATH:
```bash
cmake --version
```

### "Python not found"
Install Python 3.6 or higher:
- **Windows**: Download from [python.org](https://www.python.org/downloads/)
- **Linux**: `sudo apt install python3` (Ubuntu/Debian) or `sudo dnf install python3` (Fedora)
- **macOS**: `brew install python3`

### Permission denied (Linux/macOS)
Make scripts executable:
```bash
chmod +x tools/*.py
```

### Windows-specific issues
- Use `python` instead of `python3`
- Ensure Python and CMake are in PATH
- Check PATH with: `echo %PATH%` (CMD) or `$env:Path` (PowerShell)

## See Also

- [Plugin Generator Guide](../docs/sdk/generators/PLUGIN_GENERATOR.md)
- [Application Generator Guide](../docs/sdk/generators/APPLICATION_GENERATOR.md)
- [Application Packaging Guide](../docs/sdk/APPLICATION_PACKAGING.md)
- [Quick Start Guide](../docs/sdk/generators/QUICKSTART.md)

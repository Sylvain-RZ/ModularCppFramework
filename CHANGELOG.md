# Changelog

All notable changes to ModularCppFramework will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned
- Conan package support
- vcpkg port
- Additional modules: InputModule, ScriptingModule, DatabaseModule
- Plugin security and sandboxing features
- Advanced optimization features (parallel plugin loading, lazy loading)

---

## [1.0.0] - 2025-10-18

### Initial Release

ModularCppFramework v1.0.0 is a C++17 header-only framework for building modular applications with dynamic plugin systems.

### Added

#### Core Components
- **EventBus**: Thread-safe publish/subscribe event system with priority-based handlers
- **ServiceLocator**: Dependency injection with Singleton, Transient, and Scoped lifetimes
- **ResourceManager**: Cached resource loading with reference counting
- **PluginManager**: Dynamic plugin loading with dependency resolution and hot reload
- **ConfigurationManager**: JSON-based configuration with dot notation access and hot-reload
- **DependencyResolver**: DAG validation, cycle detection, and topological sorting
- **PluginLoader**: Cross-platform dynamic library loading (dlopen/LoadLibrary)
- **FileWatcher**: File change monitoring for hot reload functionality
- **FileSystem**: Cross-platform file system utilities
- **Logger**: Flexible logging with console, file, and rotating file sinks
- **ThreadPool**: Thread pool for asynchronous task execution
- **JsonParser**: Built-in JSON parser with full JSON value support

#### Modules
- **LoggerModule**: JSON-based logger configuration with hot-reload support
- **NetworkingModule**: TCP client and server with async I/O
- **ProfilingModule**: Performance metrics collection and frame time tracking
- **RealtimeModule**: Fixed timestep updates for physics and simulation

#### Plugin System
- Dynamic plugin loading at runtime (.so/.dll)
- Dependency resolution with semantic versioning support
- Hot reload with optional state serialization
- Priority-based initialization order
- `MCF_PLUGIN_EXPORT` macro for easy plugin creation

#### Application Framework
- Application base class with lifecycle management
- Module system for static components
- Plugin context providing access to all core services
- Thread-safe architecture using mutex-based synchronization

#### Interfaces
- **IPlugin**: Plugin interface with metadata and lifecycle hooks
- **IModule**: Module interface with ModuleBase helper class
- **IRealtimeUpdatable**: Interface for frame-based updates
- **IEventDriven**: Interface for event-driven components

#### Examples
- Logger example with JSON configuration
- Hot reload demonstration
- Realtime application with fixed timestep
- Event-driven application architecture
- Performance profiling demonstration
- File system utilities showcase
- TCP server and client examples

#### Documentation
- Complete user guide (README.md)
- Quick start guide (QUICK_START.md)
- Architecture documentation with diagrams (docs/ARCHITECTURE.md)
- Plugin creation guide (docs/PLUGIN_GUIDE.md)
- Configuration system guide (docs/CONFIGURATION_GUIDE.md)
- Hot reload feature guide (docs/HOT_RELOAD.md)
- Implementation details (docs/IMPLEMENTATION.md)
- Test coverage documentation (docs/TEST_COVERAGE.md)
- 100% Doxygen API documentation coverage

#### Testing
- Unit tests for all core components
- Integration tests for plugin system and configuration
- CTest integration for automated testing
- All tests passing (10/10)

#### Build System
- CMake 3.10+ support
- Header-only core library
- Optional modules (can be enabled/disabled)
- Cross-platform support (Linux, Windows, macOS)
- CI/CD pipeline with GitHub Actions

### Features
- Smart pointer-based memory management (no raw pointers)
- RAII principles throughout the framework
- Thread-safe core services
- Copy-under-lock pattern to prevent deadlocks
- Cross-platform file system abstraction
- Flexible logging system with multiple sinks
- JSON configuration with type-safe access

### Requirements
- C++17 compiler
- CMake 3.10 or higher
- Platform: Linux, Windows, or macOS

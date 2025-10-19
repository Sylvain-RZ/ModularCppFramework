# Cross-Platform Support

ModularCppFramework is fully cross-platform, supporting Linux, Windows, and macOS.

## Platform Support Matrix

| Platform | Compilers | Status | Notes |
|----------|-----------|--------|-------|
| **Linux** | GCC 7+, Clang 6+ | ✅ Fully Supported | Primary development platform |
| **Windows** | MSVC 2019+, MinGW, Clang | ✅ Fully Supported | Static linking for MinGW |
| **macOS** | Clang (Apple), GCC | ✅ Fully Supported | Intel and Apple Silicon |

## Tools Cross-Platform Support

All development tools are **Python-based** for guaranteed cross-platform compatibility:

### Python Scripts (`.py`) - Primary Tools
- **Platforms:** ✅ Windows, ✅ Linux, ✅ macOS
- **Requirements:** Python 3.6 or higher
- **Executable:**
  - Linux/macOS: `python3 tools/create-plugin.py`
  - Windows: `python tools/create-plugin.py`

The Python-based tools work identically on all platforms with no platform-specific workarounds needed.

## CMake Cross-Platform Features

### 1. Platform Detection

The build system automatically detects the platform and adjusts:

```cmake
# Automatic platform identifier
set(PLATFORM_ID "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

# Archive format selection
if(WIN32)
    set(ARCHIVE_EXT "zip")
else()
    set(ARCHIVE_EXT "tar.gz")
endif()
```

### 2. Plugin Extensions

Plugin file extensions are handled automatically:
- **Linux:** `.so`
- **Windows:** `.dll`
- **macOS:** `.dylib`

CMake uses `$<TARGET_FILE:plugin>` which resolves correctly on all platforms.

### 3. Static Linking (Windows MinGW)

For MinGW builds, runtime libraries are statically linked:

```cmake
if(WIN32 AND MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()
```

### 4. Archive Creation

The packaging system creates platform-appropriate archives:
- **Linux/macOS:** `.tar.gz` using `cmake -E tar czf`
- **Windows:** `.zip` using `cmake -E tar cfv`

## Platform-Specific Code

### Dynamic Library Loading

The framework abstracts platform differences:

```cpp
// Linux
#include <dlfcn.h>
void* handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
void* symbol = dlsym(handle, "createPlugin");
dlclose(handle);

// Windows
#include <windows.h>
HMODULE handle = LoadLibraryA(path.c_str());
void* symbol = (void*)GetProcAddress(handle, "createPlugin");
FreeLibrary(handle);
```

All platform-specific code is contained in [PluginLoader.hpp](../../core/PluginLoader.hpp).

### Symbol Visibility

Plugin symbols are exported using macros:

```cpp
// Linux/macOS
#define MCF_PLUGIN_API __attribute__((visibility("default")))

// Windows
#define MCF_PLUGIN_API __declspec(dllexport)
```

All non-exported symbols are hidden:

```cmake
set_target_properties(plugin PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
```

## Platform-Specific Tools

### Linux

All tools work natively with Python 3:

```bash
python3 tools/create-plugin.py -n MyPlugin
python3 tools/create-application.py -n MyApp
python3 tools/package-application.py -t package-my_app
```

Debugging tools:
```bash
# Check plugin symbols
nm -D plugins/my_plugin.so

# Check dependencies
ldd plugins/my_plugin.so

# Trace library loading
strace -e trace=open,openat ./bin/my_app
```

### Windows

Use Python scripts:

```cmd
python tools\create-plugin.py -n MyPlugin
python tools\create-application.py -n MyApp
python tools\package-application.py -t package-my_app
```

Debugging tools:
```cmd
# Check DLL dependencies (using Dependency Walker or dumpbin)
dumpbin /DEPENDENTS plugins\my_plugin.dll

# List exported symbols
dumpbin /EXPORTS plugins\my_plugin.dll
```

### macOS

Use Python 3 (works on both Intel and Apple Silicon):

```bash
python3 tools/create-plugin.py -n MyPlugin
python3 tools/create-application.py -n MyApp
python3 tools/package-application.py -t package-my_app
```

Debugging tools:
```bash
# Check plugin symbols
nm -g plugins/my_plugin.dylib

# Check dependencies
otool -L plugins/my_plugin.dylib

# Trace library loading
DYLD_PRINT_LIBRARIES=1 ./bin/my_app
```

## CI/CD Integration

The framework is tested on multiple platforms via GitHub Actions:

```yaml
strategy:
  matrix:
    os: [ubuntu-20.04, ubuntu-22.04, windows-latest, macos-latest]
    build_type: [Debug, Release]
```

See [.github/workflows/ci.yml](../../.github/workflows/ci.yml) for details.

## Common Issues and Solutions

### Issue: Exit code detection in tests (Windows)

**Problem:** The `system()` function has platform-specific behavior:
- **Windows:** Returns the exit code directly
- **Unix/Linux/macOS:** Returns a wait status that must be decoded with `WIFEXITED()` and `WEXITSTATUS()`

**Solution:** Use conditional compilation to handle both cases:

```cpp
int rawExitCode = system(command.c_str());

#ifdef _WIN32
    // On Windows, system() returns the exit code directly
    int exitCode = rawExitCode;
#else
    // On Unix, system() returns a wait status that needs to be decoded
    int exitCode = -1;
    if (WIFEXITED(rawExitCode)) {
        exitCode = WEXITSTATUS(rawExitCode);
    }
#endif
```

See [tests/unit/test_tools_scripts.cpp:94-105](../../tests/unit/test_tools_scripts.cpp#L94-L105) for the full implementation.

### Issue: Python script not found (Windows)

**Solution:** Ensure Python is installed and in PATH:
```cmd
python --version
```

### Issue: "Permission denied" on bash scripts (Linux/macOS)

**Solution:** Make scripts executable:
```bash
chmod +x tools/*.sh
```

### Issue: Plugin not loading on Windows

**Solution:** Check DLL dependencies using `dumpbin`:
```cmd
dumpbin /DEPENDENTS plugins\my_plugin.dll
```

Ensure all required DLLs are in PATH or the same directory.

### Issue: MinGW missing runtime DLLs

**Solution:** The framework uses static linking by default for MinGW. If you need dynamic linking, remove the static flags from [CMakeLists.txt:12-14](../../CMakeLists.txt#L12-L14).

### Issue: macOS "library not loaded" error

**Solution:** Use install_name_tool to fix library paths:
```bash
install_name_tool -change old_path new_path plugins/my_plugin.dylib
```

Or use `@rpath` in your CMake configuration.

## Testing Cross-Platform Support

Run the test suite on each platform:

```bash
# Configure
cmake -B build -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON

# Build
cmake --build build

# Test
ctest --test-dir build -V
```

All 27 tests should pass on all platforms.

## Packaging for Distribution

### Linux

```bash
python3 tools/package-application.py -t package-my_app
# Creates: my_app-1.0.0-linux-x86_64.tar.gz
```

### Windows

```cmd
python tools\package-application.py -t package-my_app
REM Creates: my_app-1.0.0-windows-amd64.zip
```

### macOS

```bash
python3 tools/package-application.py -t package-my_app
# Creates: my_app-1.0.0-darwin-x86_64.tar.gz (or arm64)
```

## Best Practices

1. **Use CMake for all file operations** - Avoid platform-specific commands in CMakeLists.txt
2. **Test on all platforms** - Use CI/CD to catch platform-specific issues early
3. **Use `$<TARGET_FILE:>` generator expressions** - Let CMake handle file extensions
4. **Use Python for cross-platform tools** - Python 3.6+ works everywhere
5. **Document platform differences** - Note any platform-specific behavior

## References

- [CMake Cross-Platform Guide](https://cmake.org/cmake/help/latest/manual/cmake-language.7.html)
- [Plugin Generator Guide](../sdk/generators/PLUGIN_GENERATOR.md)
- [Application Generator Guide](../sdk/generators/APPLICATION_GENERATOR.md)
- [Tools README](../../tools/README.md)

# ModularCppFramework - Packaging Guide

This guide explains how to package and distribute ModularCppFramework using Conan and vcpkg.

> **For Users**: Looking to install the package? See the [Installation section in README.md](../README.md#installation)

## Table of Contents

- [Quick Reference](#quick-reference)
- [Conan Package](#conan-package)
  - [Creating the Package](#creating-the-conan-package)
  - [Testing Locally](#testing-conan-package-locally)
  - [Publishing](#publishing-to-conan-center)
  - [Using the Package](#using-the-conan-package)
- [vcpkg Port](#vcpkg-port)
  - [Creating the Port](#creating-the-vcpkg-port)
  - [Testing Locally](#testing-vcpkg-port-locally)
  - [Publishing](#publishing-to-vcpkg-registry)
  - [Using the Port](#using-the-vcpkg-port)
- [CMake Package](#cmake-package)
- [Best Practices](#best-practices)

---

## Quick Reference

### For Package Maintainers

**Create and test Conan package:**
```bash
conan create . --build=missing
./test_conan_package.sh
```

**Create and test vcpkg port:**
```bash
mkdir -p vcpkg-overlay-ports/modular-cpp-framework
cp vcpkg.json portfile.cmake usage vcpkg-overlay-ports/modular-cpp-framework/
vcpkg install modular-cpp-framework --overlay-ports=vcpkg-overlay-ports
```

**Release workflow:**
```bash
# 1. Update versions in: conanfile.py, vcpkg.json, CMakeLists.txt, CHANGELOG.md
# 2. Commit and tag
git add -A && git commit -m "Release v1.0.0"
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin main --tags
# 3. Create GitHub release
# 4. Publish packages (optional)
```

### For Package Users

See [Installation section in README.md](../README.md#installation) for:
- Installing via Conan
- Installing via vcpkg
- Manual installation

---

## Conan Package

### Creating the Conan Package

The `conanfile.py` at the root of the repository defines the Conan package.

**Key Features:**
- **Header-only core** with platform-specific system libraries (dl, pthread, ws2_32)
- **Optional modules** via Conan options
- **CMake integration** with components
- **Multi-platform support** (Linux, Windows, macOS)

**Available Options:**
```python
options = {
    "shared": [True, False],              # Build modules as shared libraries
    "fPIC": [True, False],                # Position Independent Code (Linux/macOS)
    "build_tests": [True, False],         # Build test suite
    "build_examples": [True, False],      # Build example applications
    "with_logger_module": [True, False],  # Include LoggerModule
    "with_networking_module": [True, False],  # Include NetworkingModule
    "with_profiling_module": [True, False],   # Include ProfilingModule
    "with_realtime_module": [True, False],    # Include RealtimeModule
}
```

### Testing Conan Package Locally

#### Step 1: Create the Package

```bash
# From the project root
conan create . --build=missing

# Or with specific options
conan create . --build=missing \
    -o build_tests=True \
    -o build_examples=True
```

#### Step 2: Test Installation

Use the provided test script:

```bash
./test_conan_package.sh
```

Or manually:

```bash
# Create test directory
mkdir test_conan && cd test_conan

# Create conanfile.txt
cat > conanfile.txt << EOF
[requires]
modular-cpp-framework/1.0.0

[generators]
CMakeDeps
CMakeToolchain

[layout]
cmake_layout
EOF

# Install dependencies
conan install . --build=missing

# Build your application
cmake --preset conan-default
cmake --build --preset conan-release
```

### Publishing to Conan Center

#### Prerequisites

1. **Conan account**: Create at https://conan.io
2. **Conan remote**: Add JFrog Artifactory or Conan Center

```bash
conan remote add conancenter https://center.conan.io
```

#### Publishing Steps

```bash
# 1. Export the package
conan export . modular-cpp-framework/1.0.0@

# 2. Upload to remote
conan upload modular-cpp-framework/1.0.0 -r conancenter --all

# Or for testing with your own remote
conan remote add myremote https://your-artifactory-url
conan upload modular-cpp-framework/1.0.0 -r myremote --all
```

#### Conan Center Submission

For official Conan Center submission:

1. Fork https://github.com/conan-io/conan-center-index
2. Create recipe under `recipes/modular-cpp-framework/`
3. Follow https://github.com/conan-io/conan-center-index/blob/master/docs/adding_packages/README.md
4. Submit Pull Request

### Using the Conan Package

#### Option A: conanfile.txt

```ini
[requires]
modular-cpp-framework/1.0.0

[generators]
CMakeDeps
CMakeToolchain

[options]
modular-cpp-framework:with_logger_module=True
modular-cpp-framework:with_networking_module=True

[layout]
cmake_layout
```

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(MyApp CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(modular-cpp-framework REQUIRED)

add_executable(my_app main.cpp)

# Link with core (header-only)
target_link_libraries(my_app PRIVATE mcf::core)

# Optionally link with modules
target_link_libraries(my_app PRIVATE
    mcf::logger
    mcf::networking
    mcf::profiling
    mcf::realtime
)
```

#### Option B: conanfile.py

```python
from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout

class MyAppConan(ConanFile):
    name = "myapp"
    version = "1.0.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("modular-cpp-framework/1.0.0")

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
```

---

## vcpkg Port

### Creating the vcpkg Port

The repository includes:
- `vcpkg.json` - Port manifest
- `portfile.cmake` - Build instructions
- `usage` - Usage documentation

**Available Features:**
```json
{
  "logger": "Build Logger module",
  "networking": "Build Networking module",
  "profiling": "Build Profiling module",
  "realtime": "Build Realtime module",
  "examples": "Build example applications",
  "tests": "Build test suite"
}
```

### Testing vcpkg Port Locally

#### Step 1: Create Local Registry

```bash
# Create vcpkg overlay ports directory
mkdir -p vcpkg-overlay-ports/modular-cpp-framework

# Copy port files
cp vcpkg.json vcpkg-overlay-ports/modular-cpp-framework/
cp portfile.cmake vcpkg-overlay-ports/modular-cpp-framework/
cp usage vcpkg-overlay-ports/modular-cpp-framework/

# Update portfile.cmake SHA512
# (Generate with: sha512sum of source tarball)
```

#### Step 2: Install Locally

```bash
# Install with overlay
vcpkg install modular-cpp-framework \
    --overlay-ports=vcpkg-overlay-ports

# Or with specific features
vcpkg install modular-cpp-framework[logger,networking,profiling,realtime] \
    --overlay-ports=vcpkg-overlay-ports
```

#### Step 3: Test Integration

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(MyApp CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(modular-cpp-framework CONFIG REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE mcf::core)
```

```bash
# Build with vcpkg toolchain
cmake -B build \
    -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

### Publishing to vcpkg Registry

#### Official vcpkg Submission

1. Fork https://github.com/microsoft/vcpkg
2. Create port directory: `ports/modular-cpp-framework/`
3. Add files:
   - `vcpkg.json`
   - `portfile.cmake`
   - `usage`
4. Update `versions/m-/modular-cpp-framework.json`:

```json
{
  "versions": [
    {
      "version-semver": "1.0.0",
      "git-tree": "<git tree hash>"
    }
  ]
}
```

5. Update baseline in `versions/baseline.json`:

```json
{
  "modular-cpp-framework": {
    "baseline": "1.0.0",
    "port-version": 0
  }
}
```

6. Submit Pull Request following https://github.com/microsoft/vcpkg/blob/master/docs/maintainers/maintainer-guide.md

### Using the vcpkg Port

#### Installation

```bash
# Install with default features (all modules)
vcpkg install modular-cpp-framework

# Install core only
vcpkg install modular-cpp-framework[core]

# Install with specific features
vcpkg install modular-cpp-framework[logger,networking]

# Install with examples
vcpkg install modular-cpp-framework[examples]
```

#### CMake Integration

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyApp CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(modular-cpp-framework CONFIG REQUIRED)

add_executable(my_app main.cpp)

# Link core (always available)
target_link_libraries(my_app PRIVATE mcf::core)

# Link modules (if features enabled)
if(TARGET mcf::logger)
    target_link_libraries(my_app PRIVATE mcf::logger)
endif()

if(TARGET mcf::networking)
    target_link_libraries(my_app PRIVATE mcf::networking)
endif()
```

---

## CMake Package

ModularCppFramework provides CMake package configuration for direct use.

### Installation

```bash
# Build and install
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
make -j$(nproc)
sudo make install
```

### Usage

```cmake
find_package(modular-cpp-framework 1.0.0 REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE mcf::core)
```

---

## Best Practices

### Versioning

Follow [Semantic Versioning](https://semver.org/):
- **MAJOR**: Incompatible API changes
- **MINOR**: New features, backward compatible
- **PATCH**: Bug fixes, backward compatible

Update version in:
- `conanfile.py` → `version = "X.Y.Z"`
- `vcpkg.json` → `"version-semver": "X.Y.Z"`
- `CMakeLists.txt` → `project(ModularCppFramework VERSION X.Y.Z)`

### Testing Checklist

Before publishing:

- [ ] All tests pass: `ctest -V`
- [ ] Examples build: `cmake -DBUILD_EXAMPLES=ON`
- [ ] Conan package builds: `conan create . --build=missing`
- [ ] vcpkg port builds: `vcpkg install modular-cpp-framework --overlay-ports=...`
- [ ] Documentation updated
- [ ] CHANGELOG.md updated
- [ ] Git tag created: `git tag v1.0.0`

### Platform Support

Test on all platforms before release:

- **Linux**: Ubuntu 20.04+, Fedora, Arch
- **Windows**: Windows 10+, MSVC 2019+
- **macOS**: macOS 11+, Xcode 12+

Use GitHub Actions CI/CD (`.github/workflows/ci.yml`) for automated testing.

### Documentation

Update documentation when packaging:

- [ ] README.md installation instructions
- [ ] CHANGELOG.md release notes
- [ ] docs/PACKAGING.md (this file)
- [ ] Doxygen documentation
- [ ] vcpkg `usage` file
- [ ] Conan recipe README

### Common Issues

#### Issue: Conan can't find package

```bash
# Solution: Export package locally first
conan create . --build=missing
```

#### Issue: vcpkg SHA512 mismatch

```bash
# Solution: Update portfile.cmake with correct hash
# Generate from GitHub release tarball:
curl -L https://github.com/user/repo/archive/v1.0.0.tar.gz | sha512sum
```

#### Issue: CMake can't find package

```bash
# Solution: Set CMAKE_PREFIX_PATH or CMAKE_MODULE_PATH
cmake -DCMAKE_PREFIX_PATH=/path/to/install ..

# Or for vcpkg
cmake -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg.cmake ..
```

#### Issue: Missing system libraries

```bash
# Linux
sudo apt-get install libdl-dev  # For dlopen
# pthread is built-in with GCC

# Windows
# ws2_32.lib is built-in with MSVC
```

---

## Example: Publishing Workflow

### 1. Prepare Release

```bash
# Update version in all files
# Update CHANGELOG.md
# Commit changes
git add -A
git commit -m "Release v1.0.0"

# Create annotated tag
git tag -a v1.0.0 -m "Release version 1.0.0"

# Push to GitHub
git push origin main --tags
```

### 2. Create GitHub Release

- Go to https://github.com/Sylvain-RZ/ModularCppFramework/releases
- Click "Draft a new release"
- Select tag v1.0.0
- Add release notes from CHANGELOG.md
- Upload assets (optional)
- Publish release

### 3. Publish Conan Package

```bash
# Create package
conan create . --build=missing

# Upload to Artifactory or Conan Center
conan upload modular-cpp-framework/1.0.0 -r myremote --all
```

### 4. Submit vcpkg PR

- Fork microsoft/vcpkg
- Create port
- Update versions
- Submit PR

### 5. Announce

- Reddit: r/cpp, r/programming
- Twitter/X with #cpp #opensource
- cpp-lang Slack
- C++ Discord servers

---

## References

- [Conan Documentation](https://docs.conan.io/)
- [vcpkg Documentation](https://vcpkg.io/)
- [CMake Documentation](https://cmake.org/documentation/)
- [Semantic Versioning](https://semver.org/)
- [GitHub Releases](https://docs.github.com/en/repositories/releasing-projects-on-github)

---

**Last Updated**: 2025-10-18
**Framework Version**: 1.0.0

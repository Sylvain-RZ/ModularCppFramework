# Contributing to ModularCppFramework

Thank you for your interest in contributing to ModularCppFramework! We welcome contributions from the community and are grateful for your support.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [How Can I Contribute?](#how-can-i-contribute)
- [Contributor License Agreement (CLA)](#contributor-license-agreement-cla)
- [Development Setup](#development-setup)
- [Coding Standards](#coding-standards)
- [Commit Guidelines](#commit-guidelines)
- [Pull Request Process](#pull-request-process)
- [Testing Requirements](#testing-requirements)
- [Documentation](#documentation)

## Code of Conduct

By participating in this project, you agree to abide by our Code of Conduct:

- Be respectful and inclusive
- Welcome newcomers and help them get started
- Focus on constructive feedback
- Accept criticism gracefully
- Put the community's interests first

## How Can I Contribute?

### Reporting Bugs

Before creating a bug report:
- Check the existing issues to avoid duplicates
- Collect relevant information (OS, compiler version, build configuration)
- Create a minimal reproducible example if possible

Include in your bug report:
- Clear, descriptive title
- Steps to reproduce
- Expected vs actual behavior
- Code samples and error messages
- Environment details

### Suggesting Enhancements

Enhancement suggestions are welcome! Please:
- Use a clear, descriptive title
- Provide detailed description of the proposed feature
- Explain why this would be useful to most users
- Include code examples if applicable

### Code Contributions

We accept contributions in these areas:
- Bug fixes
- New features (discuss in an issue first for major features)
- Performance improvements
- Documentation improvements
- Test coverage improvements
- Example applications

## Contributor License Agreement (CLA)

**IMPORTANT**: Before we can accept your contribution, you must sign our Contributor License Agreement (CLA).

### Why Do We Require a CLA?

The CLA ensures that:
1. The project can legally accept your contribution
2. You retain copyright to your contribution
3. You grant us rights to use and distribute your contribution
4. The project can offer commercial licenses alongside AGPL-3.0
5. The project is protected from potential legal issues

### How to Sign the CLA

1. Read the full CLA in [CLA.md](CLA.md)
2. Add your name and details to the CLA signature section
3. Include the following statement in your first pull request:

```
I have read and agree to the Contributor License Agreement (CLA.md).
```

4. For corporate contributors, your employer may need to sign a Corporate CLA

### CLA Requirements Summary

By signing the CLA, you confirm that:
- You have the right to submit the contribution
- You grant us a perpetual, worldwide license to use your contribution
- You retain copyright to your contribution
- Your contribution is submitted under the project's license (AGPL-3.0)

## Development Setup

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.12 or later
- Git

### Building from Source

```bash
# Clone the repository
git clone https://github.com/Sylvain-RZ/ModularCppFramework.git
cd ModularCppFramework

# Create build directory
mkdir build && cd build

# Configure with tests and examples
cmake -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON ..

# Build
make -j$(nproc)

# Run tests
ctest -V
```

### Development Workflow

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Make your changes
4. Write/update tests
5. Update documentation
6. Sign the CLA (first-time contributors)
7. Submit a pull request

## Coding Standards

### C++ Style Guide

We follow modern C++ best practices:

#### Naming Conventions
- Classes: `PascalCase` (e.g., `PluginManager`)
- Functions/methods: `camelCase` (e.g., `loadPlugin()`)
- Variables: `camelCase` (e.g., `pluginName`)
- Member variables: `m_` prefix (e.g., `m_plugins`)
- Constants: `UPPER_CASE` or `kPascalCase` (e.g., `MAX_PLUGINS` or `kDefaultTimeout`)
- Namespaces: lowercase (e.g., `mcf`)

#### Code Style
- Indentation: 4 spaces (no tabs)
- Brace style: Allman (opening brace on new line)
- Line length: 100 characters maximum (flexible for readability)
- Use `auto` when type is obvious
- Prefer `nullptr` over `NULL`
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- Avoid raw pointers in public APIs

#### Example

```cpp
namespace mcf
{

class MyClass
{
private:
    std::string m_name;
    int m_value;

public:
    MyClass(const std::string& name, int value)
        : m_name(name), m_value(value)
    {
    }

    void doSomething()
    {
        if (m_value > 0)
        {
            // Implementation
        }
    }
};

} // namespace mcf
```

### Design Principles

- **RAII**: Resource Acquisition Is Initialization
- **SOLID**: Follow SOLID principles
- **DRY**: Don't Repeat Yourself
- **Thread-Safety**: All public APIs must be thread-safe
- **Header-Only**: Core library remains header-only
- **No Exceptions in Headers**: Use error codes or optional returns
- **Smart Pointers**: No raw pointer ownership

### Platform Compatibility

- Support Linux, Windows, and macOS
- Use preprocessor guards for platform-specific code
- Test on multiple platforms before submitting

```cpp
#ifdef _WIN32
    // Windows-specific code
#elif __linux__
    // Linux-specific code
#elif __APPLE__
    // macOS-specific code
#endif
```

## Commit Guidelines

### Commit Message Format

```
<type>(<scope>): <subject>

<body>

<footer>
```

#### Types
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, no logic change)
- `refactor`: Code refactoring
- `perf`: Performance improvements
- `test`: Adding or updating tests
- `build`: Build system changes
- `ci`: CI/CD changes
- `chore`: Other changes (dependencies, etc.)

#### Examples

```
feat(plugin): Add hot reload support for plugins

Implemented FileWatcher integration to detect plugin changes
and automatically reload them without application restart.

Closes #123
```

```
fix(service-locator): Fix race condition in service resolution

Added mutex protection around service map access to prevent
concurrent modification issues.

Fixes #456
```

## Pull Request Process

### Before Submitting

- [ ] Code follows the style guidelines
- [ ] Tests pass locally (`ctest -V`)
- [ ] New tests added for new features
- [ ] Documentation updated
- [ ] CLA signed (first-time contributors)
- [ ] Commit messages follow guidelines
- [ ] Branch is up-to-date with main

### PR Description Template

```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update

## Testing
- Describe how you tested your changes
- List any new tests added

## Checklist
- [ ] Code follows style guidelines
- [ ] Self-review completed
- [ ] Comments added for complex code
- [ ] Documentation updated
- [ ] Tests added/updated
- [ ] All tests pass
- [ ] CLA signed (if first contribution)

## Related Issues
Closes #(issue number)
```

### Review Process

1. Automated checks run (build, tests, formatting)
2. Maintainers review code
3. Address feedback and update PR
4. Once approved, maintainers will merge

### Merging

- PRs require at least one approval from a maintainer
- All CI checks must pass
- Branch must be up-to-date with main
- Squash and merge is preferred for feature branches

## Testing Requirements

### Unit Tests

- Use Catch2 framework
- Place tests in `tests/unit/`
- Aim for >80% code coverage
- Test edge cases and error conditions

```cpp
#include <catch2/catch_test_macros.hpp>
#include "core/ServiceLocator.hpp"

TEST_CASE("ServiceLocator resolves singleton correctly", "[service-locator]")
{
    mcf::ServiceLocator locator;

    auto service = std::make_shared<MyService>();
    locator.registerSingleton<IMyService>(service);

    auto resolved = locator.resolve<IMyService>();
    REQUIRE(resolved == service);
}
```

### Integration Tests

- Test interactions between components
- Place tests in `tests/integration/`
- Test realistic usage scenarios

### Running Tests

```bash
# Run all tests
ctest -V

# Run specific test
ctest -R ServiceLocatorTest -V

# Run with verbose output
./bin/test_app --success
```

## Documentation

### Code Documentation

- Use Doxygen-style comments for public APIs
- Document parameters, return values, and exceptions
- Include usage examples in complex APIs

```cpp
/**
 * @brief Registers a service with the service locator
 *
 * @tparam TInterface The interface type
 * @tparam TImplementation The implementation type
 * @param instance Shared pointer to the service instance
 * @throws std::runtime_error if service already registered
 *
 * @example
 * locator.registerSingleton<ILogger>(std::make_shared<ConsoleLogger>());
 */
template<typename TInterface, typename TImplementation>
void registerSingleton(std::shared_ptr<TImplementation> instance);
```

### Documentation Files

Update relevant documentation:
- `README.md` - User-facing features
- `docs/ARCHITECTURE.md` - Architecture changes
- `docs/PLUGIN_GUIDE.md` - Plugin-related changes
- `.claude/CLAUDE.md` - Build/development changes

### Examples

Add examples for new features:
- Create example in `examples/`
- Add to CMakeLists.txt with `BUILD_EXAMPLES`
- Document in README.md

## Getting Help

- Open an issue for questions
- Join our discussions (if applicable)
- Tag maintainers for urgent matters

## License

By contributing, you agree that your contributions will be licensed under the AGPL-3.0 license,
with the additional rights granted in the CLA allowing dual-licensing for commercial use.

---

Thank you for contributing to ModularCppFramework!

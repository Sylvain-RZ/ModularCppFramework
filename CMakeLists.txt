cmake_minimum_required(VERSION 3.16)
project(ModularCppFramework VERSION 1.0.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# On Windows with MinGW, link runtime libraries statically to avoid DLL issues
if(WIN32 AND MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

# Build options
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build example plugins" ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Plugin directory
set(PLUGIN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins)

# External dependencies
add_subdirectory(external)

# Core library
add_subdirectory(core)

# Modules
add_subdirectory(modules)

# Plugins
if(BUILD_EXAMPLES)
    add_subdirectory(plugins)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples
if(BUILD_EXAMPLES)
    # Hot Reload Demo
    add_executable(hot_reload_demo examples/hot_reload_demo.cpp)
    target_link_libraries(hot_reload_demo PRIVATE mcf_core mcf_realtime_module)
    set_target_properties(hot_reload_demo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

    # Logger Module Demo
    add_executable(logger_example examples/logger_example.cpp)
    target_link_libraries(logger_example PRIVATE mcf_core mcf_logger_module)
    set_target_properties(logger_example PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

    # Real-time Application Example
    add_executable(realtime_app_example examples/realtime_app_example.cpp)
    target_link_libraries(realtime_app_example PRIVATE mcf_core mcf_realtime_module)
    set_target_properties(realtime_app_example PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

    # Event-Driven Application Example
    add_executable(event_driven_app_example examples/event_driven_app_example.cpp)
    target_link_libraries(event_driven_app_example PRIVATE mcf_core)
    set_target_properties(event_driven_app_example PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

    # Profiling & Metrics Example
    add_executable(profiling_example examples/profiling_example.cpp)
    target_link_libraries(profiling_example PRIVATE mcf_core mcf_realtime_module mcf_profiling_module)
    set_target_properties(profiling_example PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

    # FileSystem Example
    add_executable(filesystem_example examples/filesystem_example.cpp)
    target_link_libraries(filesystem_example PRIVATE mcf_core)
    set_target_properties(filesystem_example PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

    # Networking Examples
    add_subdirectory(examples/networking)

    # ========================================================================
    # Package Examples Applications (Binary Distribution)
    # ========================================================================
    # This demonstrates how to use mcf_package_application_bundle() to create
    # a binary distribution bundle containing multiple applications (for end-users)

    include(cmake/MCFPackaging.cmake)

    # Create a bundle target that packages all examples together
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

endif()

# ============================================================================
# Installation Rules
# ============================================================================

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install core headers (header-only library)
install(DIRECTORY core/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mcf/core
        FILES_MATCHING PATTERN "*.hpp"
        PATTERN "*.cpp" EXCLUDE)

# Install module headers
install(DIRECTORY modules/logger/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mcf/modules/logger
        FILES_MATCHING PATTERN "*.hpp"
        PATTERN "*.cpp" EXCLUDE)

install(DIRECTORY modules/networking/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mcf/modules/networking
        FILES_MATCHING PATTERN "*.hpp"
        PATTERN "*.cpp" EXCLUDE)

install(DIRECTORY modules/profiling/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mcf/modules/profiling
        FILES_MATCHING PATTERN "*.hpp"
        PATTERN "*.cpp" EXCLUDE)

install(DIRECTORY modules/realtime/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mcf/modules/realtime
        FILES_MATCHING PATTERN "*.hpp"
        PATTERN "*.cpp" EXCLUDE)

# Install module libraries (if built)
install(TARGETS mcf_logger_module mcf_networking_module mcf_profiling_module mcf_realtime_module
        EXPORT ModularCppFrameworkTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Install documentation
install(FILES README.md LICENSE CHANGELOG.md
        DESTINATION ${CMAKE_INSTALL_DOCDIR})

install(DIRECTORY docs/
        DESTINATION ${CMAKE_INSTALL_DOCDIR}
        FILES_MATCHING PATTERN "*.md")

# Generate and install CMake config files
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ModularCppFrameworkConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ModularCppFrameworkConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ModularCppFramework
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/ModularCppFrameworkConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/ModularCppFrameworkConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/ModularCppFrameworkConfigVersion.cmake
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/MCFPackaging.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ModularCppFramework)

# Install export targets
install(EXPORT ModularCppFrameworkTargets
        FILE ModularCppFrameworkTargets.cmake
        NAMESPACE mcf::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ModularCppFramework)

# ============================================================================
# Packaging Target
# ============================================================================

# Custom target to create a release package
add_custom_target(package-release
    COMMAND ${CMAKE_COMMAND} -E echo "Creating release package..."
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/package
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/package/modular-cpp-framework-${PROJECT_VERSION}

    # Copy headers (only .hpp files, exclude build artifacts)
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/package/modular-cpp-framework-${PROJECT_VERSION}/include/mcf
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/core
            ${CMAKE_BINARY_DIR}/package/modular-cpp-framework-${PROJECT_VERSION}/include/mcf/core
            PATTERN "*.cpp" EXCLUDE
            PATTERN "*.o" EXCLUDE
            PATTERN "build" EXCLUDE
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/modules
            ${CMAKE_BINARY_DIR}/package/modular-cpp-framework-${PROJECT_VERSION}/include/mcf/modules
            PATTERN "*.cpp" EXCLUDE
            PATTERN "*.o" EXCLUDE
            PATTERN "build" EXCLUDE

    # Copy documentation
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/README.md
            ${CMAKE_BINARY_DIR}/package/modular-cpp-framework-${PROJECT_VERSION}/
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/LICENSE
            ${CMAKE_BINARY_DIR}/package/modular-cpp-framework-${PROJECT_VERSION}/
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/CHANGELOG.md
            ${CMAKE_BINARY_DIR}/package/modular-cpp-framework-${PROJECT_VERSION}/
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/docs
            ${CMAKE_BINARY_DIR}/package/modular-cpp-framework-${PROJECT_VERSION}/docs

    # Copy CMake config files
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/package/modular-cpp-framework-${PROJECT_VERSION}/cmake
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/ModularCppFrameworkConfig.cmake
            ${CMAKE_BINARY_DIR}/package/modular-cpp-framework-${PROJECT_VERSION}/cmake/
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/ModularCppFrameworkConfigVersion.cmake
            ${CMAKE_BINARY_DIR}/package/modular-cpp-framework-${PROJECT_VERSION}/cmake/

    # Copy compiled libraries (only STATIC modules, not INTERFACE)
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/package/modular-cpp-framework-${PROJECT_VERSION}/lib
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/lib/libmcf_networking_module.a
            ${CMAKE_BINARY_DIR}/package/modular-cpp-framework-${PROJECT_VERSION}/lib/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/lib/libmcf_profiling_module.a
            ${CMAKE_BINARY_DIR}/package/modular-cpp-framework-${PROJECT_VERSION}/lib/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/lib/libmcf_realtime_module.a
            ${CMAKE_BINARY_DIR}/package/modular-cpp-framework-${PROJECT_VERSION}/lib/

    # Create tarball
    COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/package
            ${CMAKE_COMMAND} -E tar czf
            ../modular-cpp-framework-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}.tar.gz
            modular-cpp-framework-${PROJECT_VERSION}

    COMMENT "Building release package: modular-cpp-framework-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}.tar.gz"
    VERBATIM
)

# Make sure modules are built before packaging
add_dependencies(package-release mcf_logger_module mcf_networking_module mcf_profiling_module mcf_realtime_module)

cmake_minimum_required(VERSION 3.16)
project(ModularCppFramework VERSION 1.0.0 LANGUAGES CXX)

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
endif()

# Install rules
install(DIRECTORY ${CMAKE_BINARY_DIR}/plugins/
        DESTINATION plugins
        FILES_MATCHING PATTERN "*.so" PATTERN "*.dll")

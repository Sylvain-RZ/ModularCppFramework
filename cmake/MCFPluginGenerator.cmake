# MCFPluginGenerator.cmake
# CMake functions to generate new plugin templates for ModularCppFramework

#[[
Function: mcf_generate_plugin
Description: Generates a new plugin template with all necessary files

Usage:
  mcf_generate_plugin(
      NAME <plugin_name>
      [VERSION <version>]
      [AUTHOR <author>]
      [DESCRIPTION <description>]
      [PRIORITY <priority>]
      [DEPENDENCIES <dep1> <dep2> ...]
      [REALTIME]
      [EVENT_DRIVEN]
      [OUTPUT_DIR <directory>]
  )

Arguments:
  NAME          - Plugin name (required, e.g., "MyPlugin")
  VERSION       - Plugin version (optional, default: "1.0.0")
  AUTHOR        - Plugin author (optional, default: "MCF Developer")
  DESCRIPTION   - Plugin description (optional)
  PRIORITY      - Load priority (optional, default: 100)
  DEPENDENCIES  - List of plugin dependencies (optional)
  REALTIME      - Add IRealtimeUpdatable interface (optional)
  EVENT_DRIVEN  - Add IEventDriven interface (optional)
  OUTPUT_DIR    - Output directory (optional, default: plugins/<plugin_name>)

Example:
  mcf_generate_plugin(
      NAME AudioPlugin
      VERSION 1.0.0
      AUTHOR "Audio Team"
      DESCRIPTION "Audio processing plugin"
      PRIORITY 200
      REALTIME
  )
]]
function(mcf_generate_plugin)
    set(options REALTIME EVENT_DRIVEN)
    set(oneValueArgs NAME VERSION AUTHOR DESCRIPTION PRIORITY OUTPUT_DIR)
    set(multiValueArgs DEPENDENCIES)
    cmake_parse_arguments(PLUGIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required arguments
    if(NOT PLUGIN_NAME)
        message(FATAL_ERROR "mcf_generate_plugin: NAME is required")
    endif()

    # Set defaults
    if(NOT PLUGIN_VERSION)
        set(PLUGIN_VERSION "1.0.0")
    endif()
    if(NOT PLUGIN_AUTHOR)
        set(PLUGIN_AUTHOR "MCF Developer")
    endif()
    if(NOT PLUGIN_DESCRIPTION)
        set(PLUGIN_DESCRIPTION "Plugin for ModularCppFramework")
    endif()
    if(NOT PLUGIN_PRIORITY)
        set(PLUGIN_PRIORITY 100)
    endif()
    if(NOT PLUGIN_OUTPUT_DIR)
        set(PLUGIN_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/plugins/${PLUGIN_NAME}")
    endif()

    # Create output directory
    file(MAKE_DIRECTORY "${PLUGIN_OUTPUT_DIR}")

    # Convert plugin name to various formats
    string(TOLOWER "${PLUGIN_NAME}" PLUGIN_NAME_LOWER)
    string(TOUPPER "${PLUGIN_NAME}" PLUGIN_NAME_UPPER)

    # Determine interfaces to implement
    set(PLUGIN_INTERFACES "")
    set(PLUGIN_INTERFACE_INCLUDES "")
    set(PLUGIN_INTERFACE_METHODS "")

    if(PLUGIN_REALTIME)
        set(PLUGIN_INTERFACES "${PLUGIN_INTERFACES}, public mcf::IRealtimeUpdatable")
        set(PLUGIN_INTERFACE_INCLUDES "${PLUGIN_INTERFACE_INCLUDES}#include \"../../core/IRealtimeUpdatable.hpp\"\n")
        set(PLUGIN_INTERFACE_METHODS "${PLUGIN_INTERFACE_METHODS}
    // IRealtimeUpdatable implementation
    void onRealtimeUpdate(float deltaTime) override {
        // TODO: Implement realtime update logic
        // This method is called every frame with deltaTime in seconds
    }
")
    endif()

    if(PLUGIN_EVENT_DRIVEN)
        set(PLUGIN_INTERFACES "${PLUGIN_INTERFACES}, public mcf::IEventDriven")
        set(PLUGIN_INTERFACE_INCLUDES "${PLUGIN_INTERFACE_INCLUDES}#include \"../../core/IEventDriven.hpp\"\n")
        set(PLUGIN_INTERFACE_METHODS "${PLUGIN_INTERFACE_METHODS}
    // IEventDriven implementation
    void onEvent(const mcf::Event& event) override {
        // TODO: Implement event handling logic
    }
")
    endif()

    # Build dependencies JSON array
    set(PLUGIN_DEPS_JSON "[]")
    if(PLUGIN_DEPENDENCIES)
        set(PLUGIN_DEPS_JSON "[\n")
        foreach(dep ${PLUGIN_DEPENDENCIES})
            set(PLUGIN_DEPS_JSON "${PLUGIN_DEPS_JSON}            {\"name\": \"${dep}\", \"minVersion\": \"1.0.0\"},\n")
        endforeach()
        string(REGEX REPLACE ",\n$" "\n" PLUGIN_DEPS_JSON "${PLUGIN_DEPS_JSON}")
        set(PLUGIN_DEPS_JSON "${PLUGIN_DEPS_JSON}        ]")
    endif()

    # Generate plugin source file
    configure_file(
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/Plugin.cpp.in"
        "${PLUGIN_OUTPUT_DIR}/${PLUGIN_NAME}.cpp"
        @ONLY
    )

    # Generate CMakeLists.txt
    configure_file(
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/PluginCMakeLists.txt.in"
        "${PLUGIN_OUTPUT_DIR}/CMakeLists.txt"
        @ONLY
    )

    # Generate README.md
    configure_file(
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/PluginREADME.md.in"
        "${PLUGIN_OUTPUT_DIR}/README.md"
        @ONLY
    )

    message(STATUS "Generated plugin '${PLUGIN_NAME}' in ${PLUGIN_OUTPUT_DIR}")
    message(STATUS "  - ${PLUGIN_NAME}.cpp")
    message(STATUS "  - CMakeLists.txt")
    message(STATUS "  - README.md")
    message(STATUS "")
    message(STATUS "Next steps:")
    message(STATUS "  1. Add 'add_subdirectory(${PLUGIN_NAME})' to plugins/CMakeLists.txt")
    message(STATUS "  2. Implement plugin logic in ${PLUGIN_OUTPUT_DIR}/${PLUGIN_NAME}.cpp")
    message(STATUS "  3. Build: cmake --build build")
    message(STATUS "")
endfunction()

#[[
Function: mcf_list_plugin_templates
Description: Lists available plugin templates and options

Usage:
  mcf_list_plugin_templates()
]]
function(mcf_list_plugin_templates)
    message(STATUS "=== MCF Plugin Generator ===")
    message(STATUS "")
    message(STATUS "Available options:")
    message(STATUS "  - Basic plugin (IPlugin only)")
    message(STATUS "  - Realtime plugin (IPlugin + IRealtimeUpdatable)")
    message(STATUS "  - Event-driven plugin (IPlugin + IEventDriven)")
    message(STATUS "  - Full plugin (IPlugin + IRealtimeUpdatable + IEventDriven)")
    message(STATUS "")
    message(STATUS "Example usage:")
    message(STATUS "  mcf_generate_plugin(NAME MyPlugin REALTIME)")
    message(STATUS "")
endfunction()

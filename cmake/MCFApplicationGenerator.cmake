# MCFApplicationGenerator.cmake
# CMake functions to generate new application templates for ModularCppFramework

#[[
Function: mcf_generate_application
Description: Generates a new MCF application template with all necessary files

Usage:
  mcf_generate_application(
      NAME <app_name>
      [VERSION <version>]
      [AUTHOR <author>]
      [DESCRIPTION <description>]
      [MODULES <module1> <module2> ...]
      [PLUGINS <plugin1> <plugin2> ...]
      [REALTIME]
      [EVENT_DRIVEN]
      [CONFIG]
      [OUTPUT_DIR <directory>]
  )

Arguments:
  NAME          - Application name (required, e.g., "MyGame")
  VERSION       - Application version (optional, default: "1.0.0")
  AUTHOR        - Application author (optional, default: "MCF Developer")
  DESCRIPTION   - Application description (optional)
  MODULES       - List of modules to include (logger, networking, profiling, realtime)
  PLUGINS       - List of plugins to load (optional)
  REALTIME      - Add realtime update loop (optional)
  EVENT_DRIVEN  - Add event-driven architecture (optional)
  CONFIG        - Generate config.json file (optional)
  OUTPUT_DIR    - Output directory (optional, default: <app_name>)

Example:
  mcf_generate_application(
      NAME MyGame
      VERSION 1.0.0
      AUTHOR "Game Team"
      DESCRIPTION "A game built with MCF"
      MODULES logger realtime profiling
      PLUGINS physics_plugin audio_plugin
      REALTIME
      CONFIG
  )
]]
function(mcf_generate_application)
    set(options REALTIME EVENT_DRIVEN CONFIG)
    set(oneValueArgs NAME VERSION AUTHOR DESCRIPTION OUTPUT_DIR)
    set(multiValueArgs MODULES PLUGINS)
    cmake_parse_arguments(APP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required arguments
    if(NOT APP_NAME)
        message(FATAL_ERROR "mcf_generate_application: NAME is required")
    endif()

    # Set defaults
    if(NOT APP_VERSION)
        set(APP_VERSION "1.0.0")
    endif()
    if(NOT APP_AUTHOR)
        set(APP_AUTHOR "MCF Developer")
    endif()
    if(NOT APP_DESCRIPTION)
        set(APP_DESCRIPTION "Application built with ModularCppFramework")
    endif()
    if(NOT APP_OUTPUT_DIR)
        set(APP_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/${APP_NAME}")
    endif()

    # Convert app name to various formats
    string(TOLOWER "${APP_NAME}" APP_NAME_LOWER)
    string(TOUPPER "${APP_NAME}" APP_NAME_UPPER)

    # Determine application type
    set(APP_BASE_CLASS "mcf::Application")
    set(APP_INCLUDES "")
    set(APP_UPDATE_METHOD "")
    set(APP_EVENT_METHOD "")

    # Add realtime support
    if(APP_REALTIME)
        set(APP_UPDATE_METHOD "
    void onUpdate(float deltaTime) override {
        // TODO: Implement application update logic
        // Called every frame with deltaTime in seconds

        // Example: Update game state, process input, etc.
    }
")
    endif()

    # Add event-driven support
    if(APP_EVENT_DRIVEN)
        set(APP_EVENT_METHOD "
    void setupEvents() {
        // Subscribe to application events
        auto* eventBus = getEventBus();
        if (eventBus) {
            eventBus->subscribe(\"app.started\",
                [this](const mcf::Event& e) {
                    std::cout << \"[${APP_NAME}] Application started!\" << std::endl;
                }
            );

            eventBus->subscribe(\"app.shutdown\",
                [this](const mcf::Event& e) {
                    std::cout << \"[${APP_NAME}] Application shutting down...\" << std::endl;
                }
            );

            // TODO: Add more event subscriptions here
        }
    }
")
    endif()

    # Build modules include list
    set(APP_MODULE_INCLUDES "")
    set(APP_MODULE_REGISTRATIONS "")
    if(APP_MODULES)
        foreach(module ${APP_MODULES})
            string(TOLOWER "${module}" module_lower)
            if(module_lower STREQUAL "logger")
                set(APP_MODULE_INCLUDES "${APP_MODULE_INCLUDES}#include \"modules/logger/LoggerModule.hpp\"\n")
                set(APP_MODULE_REGISTRATIONS "${APP_MODULE_REGISTRATIONS}        addModule<mcf::LoggerModule>();\n")
            elseif(module_lower STREQUAL "networking")
                set(APP_MODULE_INCLUDES "${APP_MODULE_INCLUDES}#include \"modules/networking/NetworkingModule.hpp\"\n")
                set(APP_MODULE_REGISTRATIONS "${APP_MODULE_REGISTRATIONS}        addModule<mcf::NetworkingModule>();\n")
            elseif(module_lower STREQUAL "profiling")
                set(APP_MODULE_INCLUDES "${APP_MODULE_INCLUDES}#include \"modules/profiling/ProfilingModule.hpp\"\n")
                set(APP_MODULE_REGISTRATIONS "${APP_MODULE_REGISTRATIONS}        addModule<mcf::ProfilingModule>();\n")
            elseif(module_lower STREQUAL "realtime")
                set(APP_MODULE_INCLUDES "${APP_MODULE_INCLUDES}#include \"modules/realtime/RealtimeModule.hpp\"\n")
                set(APP_MODULE_REGISTRATIONS "${APP_MODULE_REGISTRATIONS}        addModule<mcf::RealtimeModule>();\n")
            endif()
        endforeach()
    endif()

    # Build modules CMake link list
    set(APP_MODULE_LINKS "")
    if(APP_MODULES)
        foreach(module ${APP_MODULES})
            string(TOLOWER "${module}" module_lower)
            if(module_lower STREQUAL "logger")
                # Logger is header-only, no linking needed
            elseif(module_lower STREQUAL "networking")
                set(APP_MODULE_LINKS "${APP_MODULE_LINKS}    mcf_networking_module\n")
            elseif(module_lower STREQUAL "profiling")
                set(APP_MODULE_LINKS "${APP_MODULE_LINKS}    mcf_profiling_module\n")
            elseif(module_lower STREQUAL "realtime")
                set(APP_MODULE_LINKS "${APP_MODULE_LINKS}    mcf_realtime_module\n")
            endif()
        endforeach()
    endif()

    # Build plugins list
    set(APP_PLUGIN_PATHS "")
    if(APP_PLUGINS)
        set(APP_PLUGIN_PATHS "        // Load plugins\n")
        foreach(plugin ${APP_PLUGINS})
            set(APP_PLUGIN_PATHS "${APP_PLUGIN_PATHS}        // m_pluginManager.loadPlugin(\"plugins/${plugin}.so\");\n")
        endforeach()
    endif()

    # Create output directory structure
    file(MAKE_DIRECTORY "${APP_OUTPUT_DIR}")
    file(MAKE_DIRECTORY "${APP_OUTPUT_DIR}/src")
    file(MAKE_DIRECTORY "${APP_OUTPUT_DIR}/include")
    if(APP_CONFIG)
        file(MAKE_DIRECTORY "${APP_OUTPUT_DIR}/config")
    endif()

    # Generate main.cpp
    configure_file(
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/Application_main.cpp.in"
        "${APP_OUTPUT_DIR}/src/main.cpp"
        @ONLY
    )

    # Generate CMakeLists.txt
    configure_file(
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/ApplicationCMakeLists.txt.in"
        "${APP_OUTPUT_DIR}/CMakeLists.txt"
        @ONLY
    )

    # Generate README.md
    configure_file(
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/ApplicationREADME.md.in"
        "${APP_OUTPUT_DIR}/README.md"
        @ONLY
    )

    # Generate .gitignore
    configure_file(
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/Application.gitignore.in"
        "${APP_OUTPUT_DIR}/.gitignore"
        @ONLY
    )

    # Generate config.json if requested
    if(APP_CONFIG)
        configure_file(
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/Application_config.json.in"
            "${APP_OUTPUT_DIR}/config/config.json"
            @ONLY
        )
    endif()

    message(STATUS "Generated application '${APP_NAME}' in ${APP_OUTPUT_DIR}")
    message(STATUS "  - src/main.cpp")
    message(STATUS "  - CMakeLists.txt")
    message(STATUS "  - README.md")
    message(STATUS "  - .gitignore")
    if(APP_CONFIG)
        message(STATUS "  - config/config.json")
    endif()
    message(STATUS "")
    message(STATUS "Next steps:")
    message(STATUS "  1. cd ${APP_OUTPUT_DIR}")
    message(STATUS "  2. mkdir build && cd build")
    message(STATUS "  3. cmake ..")
    message(STATUS "  4. make -j$(nproc)")
    message(STATUS "  5. ./bin/${APP_NAME_LOWER}")
    message(STATUS "")
endfunction()

#[[
Function: mcf_list_application_templates
Description: Lists available application templates and options

Usage:
  mcf_list_application_templates()
]]
function(mcf_list_application_templates)
    message(STATUS "=== MCF Application Generator ===")
    message(STATUS "")
    message(STATUS "Available options:")
    message(STATUS "  - Basic application (minimal setup)")
    message(STATUS "  - Realtime application (with update loop)")
    message(STATUS "  - Event-driven application (with event handling)")
    message(STATUS "  - Full application (realtime + events + modules)")
    message(STATUS "")
    message(STATUS "Available modules:")
    message(STATUS "  - logger      : Logging system")
    message(STATUS "  - networking  : TCP networking")
    message(STATUS "  - profiling   : Performance profiling")
    message(STATUS "  - realtime    : Fixed timestep updates")
    message(STATUS "")
    message(STATUS "Example usage:")
    message(STATUS "  mcf_generate_application(NAME MyGame REALTIME MODULES logger profiling)")
    message(STATUS "")
endfunction()

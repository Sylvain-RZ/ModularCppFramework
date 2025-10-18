# MCFPackaging.cmake
# Helper functions to package applications built with ModularCppFramework
# Works for both cloned repo and external dependency scenarios

# ============================================================================
# mcf_package_application
# ============================================================================
# Creates a packaging target for your application
#
# Usage:
#   mcf_package_application(
#       TARGET my_app                      # Your application target
#       VERSION 1.0.0                      # Application version
#       OUTPUT_NAME "MyApplication"        # Package name
#       PLUGINS plugin1 plugin2            # Plugin targets to include (optional)
#       CONFIG_FILES config.json           # Config files to include (optional)
#       RESOURCES textures/ models/        # Resource directories (optional)
#   )
#
# Creates a target called "package-${TARGET}" that produces:
#   ${OUTPUT_NAME}-${VERSION}-${PLATFORM}.tar.gz
#
# Package structure:
#   MyApplication-1.0.0/
#   ├── bin/
#   │   └── my_app
#   ├── plugins/
#   │   ├── plugin1.so
#   │   └── plugin2.so
#   ├── config/
#   │   └── config.json
#   ├── resources/
#   │   ├── textures/
#   │   └── models/
#   └── README.txt
#
function(mcf_package_application)
    set(options "")
    set(oneValueArgs TARGET VERSION OUTPUT_NAME)
    set(multiValueArgs PLUGINS CONFIG_FILES RESOURCES)

    cmake_parse_arguments(PKG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required arguments
    if(NOT PKG_TARGET)
        message(FATAL_ERROR "mcf_package_application: TARGET is required")
    endif()

    if(NOT PKG_VERSION)
        message(FATAL_ERROR "mcf_package_application: VERSION is required")
    endif()

    if(NOT PKG_OUTPUT_NAME)
        set(PKG_OUTPUT_NAME ${PKG_TARGET})
    endif()

    # Platform identifier
    set(PLATFORM_ID "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
    string(TOLOWER ${PLATFORM_ID} PLATFORM_ID)

    # Package directory structure
    set(PACKAGE_DIR "${CMAKE_BINARY_DIR}/package")
    set(PACKAGE_ROOT "${PACKAGE_DIR}/${PKG_OUTPUT_NAME}-${PKG_VERSION}")
    set(PACKAGE_BIN "${PACKAGE_ROOT}/bin")
    set(PACKAGE_PLUGINS "${PACKAGE_ROOT}/plugins")
    set(PACKAGE_CONFIG "${PACKAGE_ROOT}/config")
    set(PACKAGE_RESOURCES "${PACKAGE_ROOT}/resources")

    # Create custom target
    add_custom_target(package-${PKG_TARGET}
        COMMAND ${CMAKE_COMMAND} -E echo "Packaging ${PKG_OUTPUT_NAME} ${PKG_VERSION}..."

        # Create directory structure
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PACKAGE_BIN}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PACKAGE_PLUGINS}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PACKAGE_CONFIG}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PACKAGE_RESOURCES}

        # Copy executable
        COMMAND ${CMAKE_COMMAND} -E copy
                $<TARGET_FILE:${PKG_TARGET}>
                ${PACKAGE_BIN}/

        COMMENT "Creating package for ${PKG_OUTPUT_NAME} ${PKG_VERSION}..."
        VERBATIM
    )

    # Make sure the target is built before packaging
    add_dependencies(package-${PKG_TARGET} ${PKG_TARGET})

    # Copy plugins if specified
    if(PKG_PLUGINS)
        foreach(plugin ${PKG_PLUGINS})
            add_custom_command(TARGET package-${PKG_TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                        $<TARGET_FILE:${plugin}>
                        ${PACKAGE_PLUGINS}/
                COMMENT "Copying plugin: ${plugin}"
            )
            add_dependencies(package-${PKG_TARGET} ${plugin})
        endforeach()
    endif()

    # Copy config files if specified
    if(PKG_CONFIG_FILES)
        foreach(config ${PKG_CONFIG_FILES})
            add_custom_command(TARGET package-${PKG_TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                        ${CMAKE_SOURCE_DIR}/${config}
                        ${PACKAGE_CONFIG}/
                COMMENT "Copying config: ${config}"
            )
        endforeach()
    endif()

    # Copy resource directories if specified
    if(PKG_RESOURCES)
        foreach(resource ${PKG_RESOURCES})
            add_custom_command(TARGET package-${PKG_TARGET} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                        ${CMAKE_SOURCE_DIR}/${resource}
                        ${PACKAGE_RESOURCES}/${resource}
                COMMENT "Copying resources: ${resource}"
            )
        endforeach()
    endif()

    # Create README.txt
    set(README_CONTENT
"${PKG_OUTPUT_NAME} ${PKG_VERSION}
==================================================

Installation:
1. Extract this archive to your desired location
2. Run: ./bin/${PKG_TARGET}

Directory Structure:
- bin/        - Executable files
- plugins/    - Plugin libraries
- config/     - Configuration files
- resources/  - Application resources

For more information, visit the project documentation.
"
    )

    file(WRITE ${CMAKE_BINARY_DIR}/package_readme.txt ${README_CONTENT})

    add_custom_command(TARGET package-${PKG_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_BINARY_DIR}/package_readme.txt
                ${PACKAGE_ROOT}/README.txt
    )

    # Create tarball
    add_custom_command(TARGET package-${PKG_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E tar czf
                ${CMAKE_BINARY_DIR}/${PKG_OUTPUT_NAME}-${PKG_VERSION}-${PLATFORM_ID}.tar.gz
                ${PKG_OUTPUT_NAME}-${PKG_VERSION}
        WORKING_DIRECTORY ${PACKAGE_DIR}
        COMMENT "Creating archive: ${PKG_OUTPUT_NAME}-${PKG_VERSION}-${PLATFORM_ID}.tar.gz"
    )

    # Print summary
    add_custom_command(TARGET package-${PKG_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
        COMMAND ${CMAKE_COMMAND} -E echo "Package created successfully!"
        COMMAND ${CMAKE_COMMAND} -E echo "Location: ${CMAKE_BINARY_DIR}/${PKG_OUTPUT_NAME}-${PKG_VERSION}-${PLATFORM_ID}.tar.gz"
        COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
        COMMAND ${CMAKE_COMMAND} -E echo ""
    )

endfunction()

# ============================================================================
# mcf_install_application
# ============================================================================
# Creates install rules for your application (for system-wide installation)
#
# Usage:
#   mcf_install_application(
#       TARGET my_app
#       PLUGINS plugin1 plugin2
#       CONFIG_FILES config.json
#   )
#
function(mcf_install_application)
    set(options "")
    set(oneValueArgs TARGET)
    set(multiValueArgs PLUGINS CONFIG_FILES)

    cmake_parse_arguments(INST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT INST_TARGET)
        message(FATAL_ERROR "mcf_install_application: TARGET is required")
    endif()

    include(GNUInstallDirs)

    # Install executable
    install(TARGETS ${INST_TARGET}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    # Install plugins
    if(INST_PLUGINS)
        foreach(plugin ${INST_PLUGINS})
            install(TARGETS ${plugin}
                    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${INST_TARGET}/plugins)
        endforeach()
    endif()

    # Install config files
    if(INST_CONFIG_FILES)
        install(FILES ${INST_CONFIG_FILES}
                DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${INST_TARGET})
    endif()

endfunction()

# ============================================================================
# mcf_package_application_bundle
# ============================================================================
# Creates a packaging target for multiple applications bundled together
#
# Usage:
#   mcf_package_application_bundle(
#       BUNDLE_NAME "MyApplicationSuite"  # Bundle name
#       VERSION 1.0.0                     # Bundle version
#       TARGETS app1 app2 app3            # Application targets to bundle
#       PLUGINS plugin1 plugin2           # Plugin targets to include (optional)
#       CONFIG_FILES config.json          # Config files to include (optional)
#       RESOURCES textures/ models/       # Resource directories (optional)
#       DESCRIPTION "Description"         # Bundle description (optional)
#       APPLICATIONS_INFO "app1=Description" "app2=Description"  # Per-app descriptions (optional)
#   )
#
# Creates a target called "package-${BUNDLE_NAME}" (lowercase) that produces:
#   ${BUNDLE_NAME}-${VERSION}-${PLATFORM}.tar.gz
#
function(mcf_package_application_bundle)
    set(options "")
    set(oneValueArgs BUNDLE_NAME VERSION DESCRIPTION)
    set(multiValueArgs TARGETS PLUGINS CONFIG_FILES RESOURCES APPLICATIONS_INFO)

    cmake_parse_arguments(BUNDLE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required arguments
    if(NOT BUNDLE_BUNDLE_NAME)
        message(FATAL_ERROR "mcf_package_application_bundle: BUNDLE_NAME is required")
    endif()

    if(NOT BUNDLE_VERSION)
        message(FATAL_ERROR "mcf_package_application_bundle: VERSION is required")
    endif()

    if(NOT BUNDLE_TARGETS)
        message(FATAL_ERROR "mcf_package_application_bundle: TARGETS is required")
    endif()

    # Platform identifier
    set(PLATFORM_ID "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

    # Package directory structure
    set(PACKAGE_DIR "${CMAKE_BINARY_DIR}/package-apps")
    set(PACKAGE_ROOT "${PACKAGE_DIR}/${BUNDLE_BUNDLE_NAME}-${BUNDLE_VERSION}")
    set(PACKAGE_BIN "${PACKAGE_ROOT}/bin")
    set(PACKAGE_PLUGINS "${PACKAGE_ROOT}/plugins")
    set(PACKAGE_CONFIG "${PACKAGE_ROOT}/config")
    set(PACKAGE_RESOURCES "${PACKAGE_ROOT}/resources")

    # Create target name (lowercase)
    string(TOLOWER ${BUNDLE_BUNDLE_NAME} BUNDLE_TARGET_NAME)

    # Create custom target
    add_custom_target(package-${BUNDLE_TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} -E echo "Creating ${BUNDLE_BUNDLE_NAME} application bundle..."

        # Create directory structure
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PACKAGE_BIN}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PACKAGE_PLUGINS}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PACKAGE_CONFIG}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PACKAGE_RESOURCES}

        COMMENT "Creating bundle: ${BUNDLE_BUNDLE_NAME} ${BUNDLE_VERSION}..."
        VERBATIM
    )

    # Copy all application executables
    foreach(target ${BUNDLE_TARGETS})
        add_custom_command(TARGET package-${BUNDLE_TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    $<TARGET_FILE:${target}>
                    ${PACKAGE_BIN}/
            COMMENT "Bundling application: ${target}"
        )
        add_dependencies(package-${BUNDLE_TARGET_NAME} ${target})
    endforeach()

    # Copy plugins if specified
    if(BUNDLE_PLUGINS)
        foreach(plugin ${BUNDLE_PLUGINS})
            add_custom_command(TARGET package-${BUNDLE_TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        $<TARGET_FILE:${plugin}>
                        ${PACKAGE_PLUGINS}/
                COMMENT "Bundling plugin: ${plugin}"
            )
            add_dependencies(package-${BUNDLE_TARGET_NAME} ${plugin})
        endforeach()
    endif()

    # Copy config files if specified
    if(BUNDLE_CONFIG_FILES)
        foreach(config ${BUNDLE_CONFIG_FILES})
            add_custom_command(TARGET package-${BUNDLE_TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                        ${CMAKE_SOURCE_DIR}/${config}
                        ${PACKAGE_CONFIG}/
                COMMENT "Bundling config: ${config}"
            )
        endforeach()
    endif()

    # Copy resource directories if specified
    if(BUNDLE_RESOURCES)
        foreach(resource ${BUNDLE_RESOURCES})
            add_custom_command(TARGET package-${BUNDLE_TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                        ${CMAKE_SOURCE_DIR}/${resource}
                        ${PACKAGE_RESOURCES}/${resource}
                COMMENT "Bundling resources: ${resource}"
            )
        endforeach()
    endif()

    # Create README.txt
    set(README_HEADER "${BUNDLE_BUNDLE_NAME} v${BUNDLE_VERSION}\n")
    string(LENGTH "${README_HEADER}" HEADER_LENGTH)
    string(REPEAT "=" ${HEADER_LENGTH} SEPARATOR)

    set(README_CONTENT "${README_HEADER}${SEPARATOR}\n\n")

    if(BUNDLE_DESCRIPTION)
        set(README_CONTENT "${README_CONTENT}${BUNDLE_DESCRIPTION}\n\n")
    endif()

    set(README_CONTENT "${README_CONTENT}Applications:\n")

    # Add application info if provided
    if(BUNDLE_APPLICATIONS_INFO)
        foreach(app_info ${BUNDLE_APPLICATIONS_INFO})
            string(REPLACE "=" ";" app_parts ${app_info})
            list(GET app_parts 0 app_name)
            list(GET app_parts 1 app_desc)
            set(README_CONTENT "${README_CONTENT}  ./bin/${app_name}  - ${app_desc}\n")
        endforeach()
    else()
        # Just list applications without descriptions
        foreach(target ${BUNDLE_TARGETS})
            set(README_CONTENT "${README_CONTENT}  ./bin/${target}\n")
        endforeach()
    endif()

    set(README_CONTENT "${README_CONTENT}\nUsage:\n")
    set(README_CONTENT "${README_CONTENT}  Run from this directory: ./bin/<application_name>\n")

    file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/bundle_readme_${BUNDLE_TARGET_NAME}.txt
         CONTENT ${README_CONTENT})

    add_custom_command(TARGET package-${BUNDLE_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_BINARY_DIR}/bundle_readme_${BUNDLE_TARGET_NAME}.txt
                ${PACKAGE_ROOT}/README.txt
    )

    # Create tarball
    add_custom_command(TARGET package-${BUNDLE_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E tar czf
                ${CMAKE_BINARY_DIR}/${BUNDLE_BUNDLE_NAME}-${BUNDLE_VERSION}-${PLATFORM_ID}.tar.gz
                ${BUNDLE_BUNDLE_NAME}-${BUNDLE_VERSION}
        WORKING_DIRECTORY ${PACKAGE_DIR}
        COMMENT "Creating bundle archive: ${BUNDLE_BUNDLE_NAME}-${BUNDLE_VERSION}-${PLATFORM_ID}.tar.gz"
    )

    # Print summary
    add_custom_command(TARGET package-${BUNDLE_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo ""
        COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
        COMMAND ${CMAKE_COMMAND} -E echo "Bundle created successfully!"
        COMMAND ${CMAKE_COMMAND} -E echo "Location: ${CMAKE_BINARY_DIR}/${BUNDLE_BUNDLE_NAME}-${BUNDLE_VERSION}-${PLATFORM_ID}.tar.gz"
        COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
        COMMAND ${CMAKE_COMMAND} -E echo ""
    )

endfunction()

# ============================================================================
# Helper: Print package info
# ============================================================================
function(mcf_print_package_info)
    message(STATUS "")
    message(STATUS "========================================")
    message(STATUS "MCF Packaging System")
    message(STATUS "========================================")
    message(STATUS "Platform: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
    message(STATUS "Binary dir: ${CMAKE_BINARY_DIR}")
    message(STATUS "")
    message(STATUS "Available packaging targets:")
    message(STATUS "  - package-release      : Package MCF framework (SDK)")
    message(STATUS "  - package-<app>        : Package single application")
    message(STATUS "  - package-<bundle>     : Package application bundle")
    message(STATUS "")
    message(STATUS "Usage:")
    message(STATUS "  cmake --build . --target package-<target>")
    message(STATUS "========================================")
    message(STATUS "")
endfunction()

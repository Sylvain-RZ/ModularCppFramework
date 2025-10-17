cmake_minimum_required(VERSION 3.16)

# Example plugin
add_library(example_plugin SHARED
    ExamplePlugin.cpp
)

target_link_libraries(example_plugin PRIVATE
    mcf_core
)

# Set output directory
set_target_properties(example_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_DIRECTORY}
    RUNTIME_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_DIRECTORY}
    PREFIX ""  # Don't add "lib" prefix on Unix
)

# Set symbol visibility
set_target_properties(example_plugin PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# Install
install(TARGETS example_plugin
        LIBRARY DESTINATION plugins
        RUNTIME DESTINATION plugins)

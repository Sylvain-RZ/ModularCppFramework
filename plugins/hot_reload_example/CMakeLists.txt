# Hot Reload Example Plugin

add_library(hot_reload_example SHARED
    HotReloadExamplePlugin.cpp
)

target_link_libraries(hot_reload_example PRIVATE
    mcf_core
)

# Set output directory to plugins folder
set_target_properties(hot_reload_example PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_DIRECTORY}
    RUNTIME_OUTPUT_DIRECTORY ${PLUGIN_OUTPUT_DIRECTORY}
    PREFIX ""  # No "lib" prefix
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# Install plugin
install(TARGETS hot_reload_example
    LIBRARY DESTINATION plugins
    RUNTIME DESTINATION plugins
)

cmake_minimum_required(VERSION 3.16)

# ProfilingModule - Optional module for performance profiling and metrics
add_library(mcf_profiling_module STATIC
    ProfilingTypes.cpp
    MetricsCollector.cpp
    ProfilingModule.cpp
)

target_include_directories(mcf_profiling_module PUBLIC
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(mcf_profiling_module PUBLIC
    mcf_core
)

# C++17 required
target_compile_features(mcf_profiling_module PUBLIC cxx_std_17)

# Set module output properties
set_target_properties(mcf_profiling_module PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# Option to enable profiling at compile time
# For debug builds, enable profiling by default
if(CMAKE_BUILD_TYPE MATCHES Debug)
    option(MCF_ENABLE_PROFILING "Enable profiling macros (zero overhead when disabled)" ON)
else()
    option(MCF_ENABLE_PROFILING "Enable profiling macros (zero overhead when disabled)" OFF)
endif()

if(MCF_ENABLE_PROFILING)
    target_compile_definitions(mcf_profiling_module PUBLIC MCF_PROFILING_ENABLED)
    message(STATUS "MCF Profiling: ENABLED (profiling macros active)")
else()
    message(STATUS "MCF Profiling: DISABLED (zero overhead - macros compile to nothing)")
endif()

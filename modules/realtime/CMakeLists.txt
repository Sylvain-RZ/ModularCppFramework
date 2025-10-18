cmake_minimum_required(VERSION 3.16)

# RealtimeModule - Optional module for real-time applications
add_library(mcf_realtime_module STATIC
    RealtimeModule.cpp
)

target_include_directories(mcf_realtime_module PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(mcf_realtime_module PUBLIC
    mcf_core
)

# C++17 required
target_compile_features(mcf_realtime_module PUBLIC cxx_std_17)

# Set module output properties
set_target_properties(mcf_realtime_module PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

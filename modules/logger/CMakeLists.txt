# Logger Module CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# The logger module is header-only, no compilation needed
# It's included via the core library

# However, we can add an interface library for organization
add_library(mcf_logger_module INTERFACE)

# Use generator expressions for build vs install paths
target_include_directories(mcf_logger_module INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(mcf_logger_module INTERFACE mcf_core)

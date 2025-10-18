cmake_minimum_required(VERSION 3.16)

# Core library as header-only
add_library(mcf_core INTERFACE)

# Different include directories for build vs install
target_include_directories(mcf_core INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(mcf_core INTERFACE cxx_std_17)

# Platform-specific linking
if(UNIX)
    target_link_libraries(mcf_core INTERFACE dl pthread)
endif()

# Install target for export
install(TARGETS mcf_core
        EXPORT ModularCppFrameworkTargets)

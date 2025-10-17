cmake_minimum_required(VERSION 3.16)

# Core library as header-only
add_library(mcf_core INTERFACE)

target_include_directories(mcf_core INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(mcf_core INTERFACE cxx_std_17)

# Platform-specific linking
if(UNIX)
    target_link_libraries(mcf_core INTERFACE dl pthread)
endif()

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION include/mcf
        FILES_MATCHING PATTERN "*.hpp")

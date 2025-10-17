cmake_minimum_required(VERSION 3.16)

# Create networking module as STATIC library
add_library(mcf_networking_module STATIC
    NetworkingModule.cpp
    TcpServer.cpp
    TcpClient.cpp
)

# Include directories
target_include_directories(mcf_networking_module PUBLIC
    ${CMAKE_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(mcf_networking_module PUBLIC
    mcf_core
)

# Platform-specific networking libraries
if(WIN32)
    # Windows: Link Winsock2
    target_link_libraries(mcf_networking_module PUBLIC ws2_32)
else()
    # Linux/Unix: No additional libraries needed (sockets are part of libc)
    # pthread is already linked via mcf_core
endif()

# C++17 standard required
target_compile_features(mcf_networking_module PUBLIC cxx_std_17)

# Set properties for symbol visibility
set_target_properties(mcf_networking_module PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# Compiler warnings
if(MSVC)
    target_compile_options(mcf_networking_module PRIVATE /W4)
else()
    target_compile_options(mcf_networking_module PRIVATE -Wall -Wextra -Wpedantic)
endif()

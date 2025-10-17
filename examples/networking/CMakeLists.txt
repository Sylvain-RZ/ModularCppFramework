cmake_minimum_required(VERSION 3.16)

# TCP Server Example
add_executable(networking_server_example
    server_example.cpp
)

target_link_libraries(networking_server_example PRIVATE
    mcf_core
    mcf_networking_module
    mcf_realtime_module
)

target_compile_features(networking_server_example PRIVATE cxx_std_17)

set_target_properties(networking_server_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# TCP Client Example
add_executable(networking_client_example
    client_example.cpp
)

target_link_libraries(networking_client_example PRIVATE
    mcf_core
    mcf_networking_module
    mcf_realtime_module
)

target_compile_features(networking_client_example PRIVATE cxx_std_17)

set_target_properties(networking_client_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

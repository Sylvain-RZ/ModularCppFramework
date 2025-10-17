cmake_minimum_required(VERSION 3.16)

# Catch2 v3 library
add_library(Catch2 STATIC
    catch_amalgamated.cpp
)

target_include_directories(Catch2 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(Catch2 PUBLIC cxx_std_17)

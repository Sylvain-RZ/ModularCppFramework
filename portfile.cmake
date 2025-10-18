# vcpkg portfile for modular-cpp-framework

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO Sylvain-RZ/ModularCppFramework
    REF v1.0.0
    SHA512 0
    HEAD_REF main
)

# Determine which features to build
vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        logger      BUILD_LOGGER_MODULE
        networking  BUILD_NETWORKING_MODULE
        profiling   BUILD_PROFILING_MODULE
        realtime    BUILD_REALTIME_MODULE
        examples    BUILD_EXAMPLES
        tests       BUILD_TESTS
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        ${FEATURE_OPTIONS}
        -DBUILD_TESTS=OFF
        -DBUILD_EXAMPLES=OFF
)

vcpkg_cmake_install()

# Remove duplicate headers from debug
vcpkg_cmake_config_fixup(PACKAGE_NAME modular-cpp-framework CONFIG_PATH lib/cmake/modular-cpp-framework)

# Copy header files
file(INSTALL "${SOURCE_PATH}/core/" DESTINATION "${CURRENT_PACKAGES_DIR}/include/mcf/core" FILES_MATCHING PATTERN "*.hpp")
file(INSTALL "${SOURCE_PATH}/modules/" DESTINATION "${CURRENT_PACKAGES_DIR}/include/mcf/modules" FILES_MATCHING PATTERN "*.hpp")

# Remove unnecessary files
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

# Handle license
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

# Copy usage file
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")

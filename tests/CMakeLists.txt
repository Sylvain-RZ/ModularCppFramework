cmake_minimum_required(VERSION 3.16)

# ============================================
# Unit Tests (Catch2)
# ============================================

# EventBus Unit Tests
add_executable(test_eventbus
    unit/test_eventbus.cpp
)
target_link_libraries(test_eventbus PRIVATE mcf_core Catch2)
add_test(NAME EventBus COMMAND test_eventbus)

# ServiceLocator Unit Tests
add_executable(test_service_locator
    unit/test_service_locator.cpp
)
target_link_libraries(test_service_locator PRIVATE mcf_core Catch2)
add_test(NAME ServiceLocator COMMAND test_service_locator)

# ResourceManager Unit Tests
add_executable(test_resource_manager
    unit/test_resource_manager.cpp
)
target_link_libraries(test_resource_manager PRIVATE mcf_core Catch2)
add_test(NAME ResourceManager COMMAND test_resource_manager)

# DependencyResolver Unit Tests
add_executable(test_dependency_resolver
    unit/test_dependency_resolver.cpp
)
target_link_libraries(test_dependency_resolver PRIVATE mcf_core Catch2)
add_test(NAME DependencyResolver COMMAND test_dependency_resolver)

# FileWatcher Unit Tests
add_executable(test_file_watcher
    unit/test_file_watcher.cpp
)
target_link_libraries(test_file_watcher PRIVATE mcf_core Catch2)
add_test(NAME FileWatcher COMMAND test_file_watcher)

# ThreadPool Unit Tests
add_executable(test_thread_pool
    unit/test_thread_pool.cpp
)
target_link_libraries(test_thread_pool PRIVATE mcf_core Catch2)
add_test(NAME ThreadPool COMMAND test_thread_pool)

# FileSystem Unit Tests
add_executable(test_filesystem
    unit/test_filesystem.cpp
)
target_link_libraries(test_filesystem PRIVATE mcf_core Catch2)
add_test(NAME FileSystem COMMAND test_filesystem)

# ============================================
# Integration Tests
# ============================================

# Basic Integration Test
add_executable(test_app
    integration/test_app.cpp
)
target_link_libraries(test_app PRIVATE mcf_core)
add_test(NAME IntegrationTest_Basic COMMAND test_app)

# # Hot Reload Integration Test
# add_executable(test_hot_reload
#     integration/test_hot_reload.cpp
# )
# target_link_libraries(test_hot_reload PRIVATE mcf_core Catch2)
# add_test(NAME IntegrationTest_HotReload COMMAND test_hot_reload)

# Configuration System Test
add_executable(test_config
    test_config.cpp
)
target_link_libraries(test_config PRIVATE mcf_core)
add_test(NAME IntegrationTest_Config COMMAND test_config)

# Logger System Test
add_executable(test_logger
    test_logger.cpp
)
target_link_libraries(test_logger PRIVATE mcf_core mcf_logger_module)
add_test(NAME IntegrationTest_Logger COMMAND test_logger)

# ============================================
# Test Output Configuration
# ============================================

# Set output directory for test executables
set_target_properties(
    test_eventbus
    test_service_locator
    test_resource_manager
    test_dependency_resolver
    test_file_watcher
    test_thread_pool
    test_filesystem
    test_app
    # test_hot_reload
    test_config
    test_logger
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests
)

# ============================================
# Custom Test Targets
# ============================================

# Run all unit tests
add_custom_target(unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -R "EventBus|ServiceLocator|ResourceManager|DependencyResolver|FileWatcher|ThreadPool|FileSystem" --exclude-regex Integration
    DEPENDS test_eventbus
            test_service_locator
            test_resource_manager
            test_dependency_resolver
            test_file_watcher
            test_thread_pool
            test_filesystem
    COMMENT "Running unit tests..."
)

# Run benchmarks
add_custom_target(benchmarks
    COMMAND test_eventbus "[.benchmark]"
    COMMAND test_service_locator "[.benchmark]"
    COMMAND test_resource_manager "[.benchmark]"
    COMMAND test_dependency_resolver "[.benchmark]"
    COMMAND test_file_watcher "[.benchmark]"
    COMMAND test_thread_pool "[.benchmark]"
    COMMAND test_filesystem "[.benchmark]"
    # COMMAND test_hot_reload "[.benchmark]"
    DEPENDS test_eventbus
            test_service_locator
            test_resource_manager
            test_dependency_resolver
            test_file_watcher
            test_thread_pool
            test_filesystem
            # test_hot_reload
    COMMENT "Running benchmarks..."
)

# Run hot-reload specific tests
add_custom_target(hot_reload_tests
    COMMAND test_eventbus "[hot-reload]"
    COMMAND test_service_locator "[hot-reload]"
    COMMAND test_resource_manager "[hot-reload]"
    COMMAND test_dependency_resolver "[hot-reload]"
    COMMAND test_file_watcher "[hot-reload]"
    # COMMAND test_hot_reload "[hot-reload]"
    DEPENDS test_eventbus
            test_service_locator
            test_resource_manager
            test_dependency_resolver
            test_file_watcher
            # test_hot_reload
    COMMENT "Running hot-reload tests..."
)

# Run integration tests
add_custom_target(integration_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -R IntegrationTest
    DEPENDS test_app test_config test_logger  # test_hot_reload
    COMMENT "Running integration tests..."
)

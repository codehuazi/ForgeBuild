cmake_minimum_required(VERSION 3.20)

project(
    ForgeBuild
    VERSION 0.1.0
    LANGUAGES CXX
)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(
    Threads REQUIRED
)

add_library(
    forge_core
    STATIC
        src/rule.cpp
        src/node.cpp
        src/edge.cpp
        src/build_graph.cpp
        src/build_plan.cpp
        src/manifest.cpp
        src/string_utils.cpp
        src/parser.cpp
        src/builder.cpp
        src/build_planner.cpp
        src/executor.cpp
        src/file_system.cpp
        src/hash.cpp
        src/cache_key.cpp
        src/compiler_identity.cpp
        src/build_log.cpp
        src/scheduler.cpp
        src/depfile.cpp
        src/deps_log.cpp
        src/local_cache.cpp
)

target_include_directories(
    forge_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_options(
    forge_core
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

add_executable(
    forge
    src/main.cpp
)


target_link_libraries(
    forge
    PRIVATE
        forge_core
)

add_executable(
    graph_demo
    tests/graph_demo.cpp
)

target_link_libraries(
    graph_demo
    PRIVATE
        forge_core
)

target_compile_options(
    graph_demo
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

add_executable(string_demo
    tests/string_demo.cpp
)

target_link_libraries(
    string_demo
    forge_core
)

add_executable(
    parser_demo
    tests/parser_demo.cpp
)
target_link_libraries(
    parser_demo
    forge_core
)

add_executable(builder_demo
    tests/builder_demo.cpp
)


target_link_libraries(
    builder_demo
    forge_core
)

add_executable(
    executor_demo
    tests/executor_demo.cpp
)


target_link_libraries(
    executor_demo
    forge_core
)

add_executable(
    file_system_demo
    tests/file_system_demo.cpp
)

target_link_libraries(
    file_system_demo
    PRIVATE
    forge_core
)

add_executable(
    incremental_chain_demo
    tests/incremental_chain_demo.cpp
)

target_link_libraries(
    incremental_chain_demo
    PRIVATE
    forge_core
)

add_executable(
    hash_demo
    tests/hash_demo.cpp
)

target_link_libraries(
    hash_demo
    PRIVATE
    forge_core
)

add_executable(
    compiler_identity_demo
    tests/compiler_identity_demo.cpp
)

target_link_libraries(
    compiler_identity_demo
    PRIVATE
        forge_core
)

add_executable(
    build_log_demo
    tests/build_log_demo.cpp
)

target_link_libraries(
    build_log_demo
    PRIVATE
    forge_core
)

add_executable(
    command_change_demo
    tests/command_change_demo.cpp
)

target_link_libraries(
    command_change_demo
    PRIVATE
    forge_core
)

add_executable(
    executor_build_log_demo
    tests/executor_build_log_demo.cpp
)

target_link_libraries(
    executor_build_log_demo
    PRIVATE
    forge_core
)

add_executable(
    scheduler_demo
    tests/scheduler_demo.cpp
)


target_link_libraries(
    scheduler_demo
    PRIVATE
    forge_core
)

add_executable(
    mutex_demo
    tests/mutex_demo.cpp
)

add_executable(
    condition_variable_demo
    tests/condition_variable_demo.cpp
)

target_link_libraries(
    condition_variable_demo
    PRIVATE
    Threads::Threads
)

add_executable(
    task_queue_demo
    tests/task_queue_demo.cpp
)

target_link_libraries(
    task_queue_demo
    PRIVATE
    Threads::Threads
)

add_executable(
    two_workers_demo
    tests/two_workers_demo.cpp
)

target_link_libraries(
    two_workers_demo
    PRIVATE
    Threads::Threads
)

add_executable(
    dependency_queue_demo
    tests/dependency_queue_demo.cpp
)

target_link_libraries(
    dependency_queue_demo
    PRIVATE
    Threads::Threads
)

add_executable(
    depfile_demo
    tests/depfile_demo.cpp
)


target_link_libraries(
    depfile_demo
    forge_core
)

add_executable(
    deps_log_demo
    tests/deps_log_demo.cpp
)


target_link_libraries(
    deps_log_demo
    forge_core
)


add_executable(
    dynamic_dependency_demo
    tests/dynamic_dependency_demo.cpp
)


target_link_libraries(
    dynamic_dependency_demo
    PRIVATE
        forge_core
)

add_executable(
    real_depfile_build_demo
    tests/real_depfile_build_demo.cpp
)


target_link_libraries(
    real_depfile_build_demo
    PRIVATE
        forge_core
)

add_executable(
    scheduler_guard_demo
    tests/scheduler_guard_demo.cpp
)


target_link_libraries(
    scheduler_guard_demo
    PRIVATE
        forge_core
)

add_executable(
    scheduler_multi_output_demo
    tests/scheduler_multi_output_demo.cpp
)


target_link_libraries(
    scheduler_multi_output_demo
    PRIVATE
        forge_core
)

add_executable(
    scheduler_failure_demo
    tests/scheduler_failure_demo.cpp
)

add_executable(
    build_planner_multi_output_demo
    tests/build_planner_multi_output_demo.cpp
)

target_link_libraries(
    build_planner_multi_output_demo
    PRIVATE
        forge_core
)

add_executable(
    local_cache_demo
    tests/local_cache_demo.cpp
)

target_link_libraries(
    local_cache_demo
    PRIVATE
    forge_core
)


target_link_libraries(
    scheduler_failure_demo
    PRIVATE
        forge_core
)

add_executable(
    log_atomicity_demo
    tests/log_atomicity_demo.cpp
)

target_link_libraries(
    log_atomicity_demo
    PRIVATE
        forge_core
)

add_executable(
    persisted_state_format_demo
    tests/persisted_state_format_demo.cpp
)


target_link_libraries(
    persisted_state_format_demo
    PRIVATE
        forge_core
)

if(BUILD_TESTING)

    add_test(
        NAME scheduler_guard
        COMMAND scheduler_guard_demo
    )

    add_test(
        NAME scheduler_multi_output
        COMMAND scheduler_multi_output_demo
    )

    add_test(
        NAME build_planner_multi_output
        COMMAND build_planner_multi_output_demo
    )

    add_test(
        NAME scheduler_failure
        COMMAND scheduler_failure_demo
    )

    add_test(
        NAME hash
        COMMAND hash_demo
    )

    add_test(
        NAME local_cache
        COMMAND local_cache_demo
    )

    add_test(
        NAME compiler_identity
        COMMAND compiler_identity_demo
    )

    add_test(
        NAME incremental_source
        COMMAND
            bash
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/incremental_source_test.sh"
            "$<TARGET_FILE:forge>"
            "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    add_test(
        NAME incremental_header
        COMMAND
            bash
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/incremental_header_test.sh"
            "$<TARGET_FILE:forge>"
            "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    add_test(
        NAME incremental_command
        COMMAND
            bash
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/incremental_command_test.sh"
            "$<TARGET_FILE:forge>"
            "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    add_test(
        NAME incremental_missing_input
        COMMAND
            bash
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/incremental_missing_input_test.sh"
            "$<TARGET_FILE:forge>"
            "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    add_test(
        NAME missing_output
        COMMAND
            bash
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/missing_output_test.sh"
            "$<TARGET_FILE:forge>"
    )

    add_test(
        NAME log_atomicity
        COMMAND log_atomicity_demo
    )

        add_test(
        NAME persisted_state_format
        COMMAND persisted_state_format_demo
    )


    add_test(
        NAME corrupted_persisted_state
        COMMAND
            bash
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/corrupted_persisted_state_test.sh"
            "$<TARGET_FILE:forge>"
            "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    add_test(
        NAME interrupted_state
        COMMAND
            bash
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/interrupted_state_test.sh"
            "$<TARGET_FILE:forge>"
            "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    set_tests_properties(
        scheduler_guard
        scheduler_multi_output
        scheduler_failure
        build_planner_multi_output
        hash
        local_cache
        compiler_identity
        log_atomicity
        persisted_state_format
        PROPERTIES
            TIMEOUT 5
    )

    set_tests_properties(
        scheduler_guard
        scheduler_multi_output
        scheduler_failure
        build_planner_multi_output
        hash
        local_cache
        compiler_identity
        log_atomicity
        persisted_state_format
        PROPERTIES
            TIMEOUT 5
    )

endif()
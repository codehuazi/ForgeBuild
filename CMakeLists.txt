cmake_minimum_required(VERSION 3.20)

project(
    ForgeBuild
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(
    forge_core
    STATIC
        src/rule.cpp
        src/node.cpp
        src/edge.cpp
        src/build_graph.cpp
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
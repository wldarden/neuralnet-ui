cmake_minimum_required(VERSION 3.24)
project(neuralnet-ui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/CompilerWarnings.cmake)

add_library(neuralnet-ui STATIC
    src/render_neural_net.cpp
    src/render_net_topology.cpp
    src/net_background.cpp
    src/tiny_text.cpp
)
target_include_directories(neuralnet-ui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(neuralnet-ui PUBLIC neuralnet SDL2::SDL2 PRIVATE project_warnings)

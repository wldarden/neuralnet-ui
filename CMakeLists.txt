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

#pragma once

#include <neuralnet/network.h>
#include <neuralnet-ui/render_neural_net.h>

#include <cstddef>
#include <vector>

struct SDL_Renderer;

namespace neuralnet_ui {

struct TopologyRenderConfig {
    SDL_Renderer* renderer = nullptr;
    const neuralnet::NetworkTopology* topology = nullptr;
    int x = 0, y = 0, w = 200, h = 200;

    std::vector<NodeColor> input_colors;
};

/// Render a static network topology diagram (no activations, no hover).
void render_net_topology(const TopologyRenderConfig& config);

} // namespace neuralnet_ui

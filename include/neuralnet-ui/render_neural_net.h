#pragma once

#include <neuralnet/network.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct SDL_Renderer;

namespace neuralnet_ui {

struct NodeColor {
    uint8_t r, g, b;
};

struct NetRenderConfig {
    SDL_Renderer* renderer = nullptr;
    const neuralnet::Network* network = nullptr;
    int x = 0, y = 0, w = 400, h = 600;

    std::vector<float> input_values;
    std::vector<std::string> input_labels;
    std::vector<NodeColor> input_colors;
    std::vector<std::size_t> display_order;

    std::vector<std::string> output_labels;
    std::vector<NodeColor> output_colors;

    int mouse_x = -1;
    int mouse_y = -1;

    int text_scale = 1;  // bitmap font scale (1 = default, 2 = 2x, etc.)
};

/// Result of a node interaction (hover or click).
struct NodeHit {
    int column = -1;   // -1 = no hit. 0 = input, 1..N = layers
    int node = -1;     // node index within column
    bool is_input() const { return column == 0 && node >= 0; }
    bool is_hidden() const { return column > 0 && node >= 0; }
    bool is_output(const neuralnet::Network& net) const {
        return column == static_cast<int>(net.topology().layers.size()) && node >= 0;
    }
    bool valid() const { return column >= 0 && node >= 0; }
};

/// Result filled by render_neural_net.
struct NetRenderResult {
    NodeHit hovered;   // node under the mouse this frame
    NodeHit clicked;   // node that was clicked this frame (mouse down → up)
    int column_clicked = -1;  // column index clicked but NOT on a specific node (-1 = none)
};

/// Render an interactive neural network diagram using SDL2.
/// Returns hover/click info for the caller to act on.
[[nodiscard]] NetRenderResult render_neural_net(const NetRenderConfig& config);

} // namespace neuralnet_ui

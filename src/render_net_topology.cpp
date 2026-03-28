#include <neuralnet-ui/render_net_topology.h>
#include <neuralnet-ui/tiny_text.h>

#include <SDL.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace neuralnet_ui {

void render_net_topology(const TopologyRenderConfig& config) {
    const auto& topology = *config.topology;

    if (topology.layers.empty()) return;

    // Compute layout: 1 input column + N layer columns
    std::size_t num_columns = 1 + topology.layers.size();
    float col_spacing = static_cast<float>(config.w) / static_cast<float>(num_columns);

    struct NodePos { float px, py; };
    std::vector<std::vector<NodePos>> columns;

    // Input column
    {
        std::vector<NodePos> col;
        auto n = topology.input_size;
        float spacing = static_cast<float>(config.h) / static_cast<float>(n + 1);
        for (std::size_t i = 0; i < n; ++i) {
            col.push_back({
                static_cast<float>(config.x) + col_spacing * 0.5f,
                static_cast<float>(config.y) + spacing * static_cast<float>(i + 1)
            });
        }
        columns.push_back(std::move(col));
    }

    // Hidden + output layers
    for (std::size_t l = 0; l < topology.layers.size(); ++l) {
        std::vector<NodePos> col;
        auto n = topology.layers[l].output_size;
        float spacing = static_cast<float>(config.h) / static_cast<float>(n + 1);
        for (std::size_t i = 0; i < n; ++i) {
            col.push_back({
                static_cast<float>(config.x) + col_spacing * (static_cast<float>(l) + 1.5f),
                static_cast<float>(config.y) + spacing * static_cast<float>(i + 1)
            });
        }
        columns.push_back(std::move(col));
    }

    // Draw connections between adjacent columns (neutral gray, alpha ~50)
    SDL_SetRenderDrawColor(config.renderer, 100, 100, 100, 50);
    for (std::size_t l = 0; l + 1 < columns.size(); ++l) {
        for (const auto& from : columns[l]) {
            for (const auto& to : columns[l + 1]) {
                SDL_RenderDrawLine(config.renderer,
                    static_cast<int>(from.px), static_cast<int>(from.py),
                    static_cast<int>(to.px), static_cast<int>(to.py));
            }
        }
    }

    // Draw node circles with type-based coloring
    for (std::size_t c = 0; c < columns.size(); ++c) {
        bool is_input = (c == 0);
        bool is_output = (c == columns.size() - 1);
        float radius = is_output ? 5.0f : 3.5f;

        for (std::size_t i = 0; i < columns[c].size(); ++i) {
            uint8_t nr, ng, nb;
            if (is_input && !config.input_colors.empty()) {
                // Use caller-provided input colors
                if (i < config.input_colors.size()) {
                    nr = config.input_colors[i].r;
                    ng = config.input_colors[i].g;
                    nb = config.input_colors[i].b;
                } else {
                    // Default blue for inputs beyond the color list
                    nr = 68; ng = 136; nb = 255;
                }
            } else if (is_input) {
                // No colors provided: default blue
                nr = 68; ng = 136; nb = 255;
            } else if (is_output) {
                // Output: green
                nr = 0; ng = 200; nb = 136;
            } else {
                // Hidden: warm gray
                nr = 200; ng = 160; nb = 120;
            }
            render_filled_circle(config.renderer, columns[c][i].px, columns[c][i].py, radius, 0.2f, nr, ng, nb);
        }
    }
}

} // namespace neuralnet_ui

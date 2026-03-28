#include <neuralnet-ui/render_neural_net.h>
#include <neuralnet-ui/tiny_text.h>

#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace neuralnet_ui {

NetRenderResult render_neural_net(const NetRenderConfig& config) {
    NetRenderResult result;
    int panel_x = config.x;
    int panel_w = config.w;
    int panel_h = config.h;

    auto all_activations = config.network->get_all_activations();
    const auto& topo = config.network->topology();

    // Compute layout: columns for input + each layer
    std::size_t num_columns = 1 + topo.layers.size();  // input + layers
    float col_spacing = static_cast<float>(panel_w) / static_cast<float>(num_columns) * 0.5f;

    // Build node positions: [column][node_idx] -> (x, y)
    struct NodePos { float x, y; };
    std::vector<std::vector<NodePos>> columns;

    // Input column
    {
        std::vector<NodePos> col;
        auto n = topo.input_size;
        float spacing = static_cast<float>(panel_h) / static_cast<float>(n + 1);
        for (std::size_t i = 0; i < n; ++i) {
            col.push_back({
                static_cast<float>(panel_x) + col_spacing * 0.5f,
                10.0f + spacing * static_cast<float>(i + 1)
            });
        }
        columns.push_back(std::move(col));
    }

    // Hidden + output layers
    for (std::size_t l = 0; l < topo.layers.size(); ++l) {
        std::vector<NodePos> col;
        auto n = topo.layers[l].output_size;
        float spacing = static_cast<float>(panel_h) / static_cast<float>(n + 1);
        for (std::size_t i = 0; i < n; ++i) {
            col.push_back({
                static_cast<float>(panel_x) + col_spacing * (static_cast<float>(l) + 1.5f),
                10.0f + spacing * static_cast<float>(i + 1)
            });
        }
        columns.push_back(std::move(col));
    }

    // Detect hovered node: find closest node to mouse within radius
    int hover_col = -1;
    int hover_node = -1;
    constexpr float HOVER_RADIUS = 18.0f;
    {
        float best_dist = HOVER_RADIUS;
        for (std::size_t c = 0; c < columns.size(); ++c) {
            for (std::size_t n = 0; n < columns[c].size(); ++n) {
                float dx = static_cast<float>(config.mouse_x) - columns[c][n].x;
                float dy = static_cast<float>(config.mouse_y) - columns[c][n].y;
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist < best_dist) {
                    best_dist = dist;
                    hover_col = static_cast<int>(c);
                    hover_node = static_cast<int>(n);
                }
            }
        }
    }
    result.hovered = {hover_col, hover_node};

    // Detect click: mouse button released
    // Left-click = node click (if near a node)
    // Right-click = layer/column click (if in a column's x-range)
    {
        static bool was_left_down = false;
        static bool was_right_down = false;
        uint32_t mouse_btn = SDL_GetMouseState(nullptr, nullptr);
        bool left_down = (mouse_btn & SDL_BUTTON_LMASK) != 0;
        bool right_down = (mouse_btn & SDL_BUTTON_RMASK) != 0;

        // Left-click release: node click
        if (!left_down && was_left_down && hover_col >= 0 && hover_node >= 0) {
            result.clicked = {hover_col, hover_node};
        }

        // Right-click release: column/layer click
        if (!right_down && was_right_down) {
            float mx = static_cast<float>(config.mouse_x);
            float my = static_cast<float>(config.mouse_y);
            if (my >= static_cast<float>(config.y) &&
                my <= static_cast<float>(config.y + config.h)) {
                for (std::size_t c = 0; c < columns.size(); ++c) {
                    if (columns[c].empty()) continue;
                    float col_x = columns[c][0].x;
                    constexpr float COL_CLICK_RADIUS = 40.0f;
                    if (std::abs(mx - col_x) < COL_CLICK_RADIUS) {
                        result.column_clicked = static_cast<int>(c);
                        break;
                    }
                }
            }
        }

        was_left_down = left_down;
        was_right_down = right_down;
    }

    // Draw connections (between adjacent columns)
    auto all_weights = config.network->get_all_weights();

    // First pass: draw non-hovered connections (dim)
    std::size_t wo = 0;
    for (std::size_t l = 0; l < topo.layers.size(); ++l) {
        auto prev_size = (l == 0) ? topo.input_size : topo.layers[l - 1].output_size;
        auto curr_size = topo.layers[l].output_size;
        auto weight_count = prev_size * curr_size;

        for (std::size_t to = 0; to < curr_size; ++to) {
            for (std::size_t from = 0; from < prev_size; ++from) {
                float w = all_weights[wo + to * prev_size + from];
                float abs_w = std::min(std::abs(w), 2.0f) / 2.0f;

                if (abs_w < 0.05f) continue;

                // Dim connections when hovering (unless connected to hovered node)
                bool connected_to_hover = false;
                if (hover_col >= 0) {
                    bool from_match = (static_cast<int>(l) == hover_col && static_cast<int>(from) == hover_node);
                    bool to_match = (static_cast<int>(l + 1) == hover_col && static_cast<int>(to) == hover_node);
                    connected_to_hover = from_match || to_match;
                }

                if (hover_col >= 0 && !connected_to_hover) {
                    // Dim non-connected lines when hovering
                    SDL_SetRenderDrawColor(config.renderer, 30, 30, 30, 30);
                } else {
                    uint8_t r = w < 0 ? static_cast<uint8_t>(abs_w * 200) : 0;
                    uint8_t b = w > 0 ? static_cast<uint8_t>(abs_w * 200) : 0;
                    uint8_t alpha = static_cast<uint8_t>(40 + abs_w * 150);
                    if (connected_to_hover) {
                        // Bright highlight for hovered connections
                        r = w < 0 ? 255 : 50;
                        b = w > 0 ? 255 : 50;
                        alpha = 255;
                    }
                    SDL_SetRenderDrawColor(config.renderer, r, 0, b, alpha);
                }

                SDL_RenderDrawLine(config.renderer,
                    static_cast<int>(columns[l][from].x),
                    static_cast<int>(columns[l][from].y),
                    static_cast<int>(columns[l + 1][to].x),
                    static_cast<int>(columns[l + 1][to].y));
            }
        }

        wo += weight_count + curr_size;
    }

    // Second pass: draw weight labels on hovered node's connections
    if (hover_col >= 0) {
        std::size_t weight_offset = 0;
        for (std::size_t l = 0; l < topo.layers.size(); ++l) {
            auto prev_size = (l == 0) ? topo.input_size : topo.layers[l - 1].output_size;
            auto curr_size = topo.layers[l].output_size;
            auto wc = prev_size * curr_size;

            for (std::size_t to = 0; to < curr_size; ++to) {
                for (std::size_t from = 0; from < prev_size; ++from) {
                    bool from_match = (static_cast<int>(l) == hover_col && static_cast<int>(from) == hover_node);
                    bool to_match = (static_cast<int>(l + 1) == hover_col && static_cast<int>(to) == hover_node);

                    if (from_match || to_match) {
                        float w = all_weights[weight_offset + to * prev_size + from];
                        if (std::abs(w) < 0.01f) continue;

                        // Draw weight label at the OTHER end of the connection
                        float label_x, label_y;
                        if (from_match) {
                            label_x = columns[l + 1][to].x;
                            label_y = columns[l + 1][to].y;
                        } else {
                            label_x = columns[l][from].x;
                            label_y = columns[l][from].y;
                        }

                        char buf[16];
                        std::snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(w));

                        uint8_t tr = w < 0 ? 255 : 100;
                        uint8_t tb = w > 0 ? 255 : 100;
                        draw_tiny_text(config.renderer,
                                       static_cast<int>(label_x) + 8,
                                       static_cast<int>(label_y) - 8,
                                       buf, config.text_scale, tr, 200, tb);
                    }
                }
            }

            weight_offset += wc + curr_size;
        }
    }

    // Use caller-provided labels, or generate generic defaults.
    std::vector<std::string> label_strings;

    if (!config.input_labels.empty()) {
        label_strings = config.input_labels;
    } else {
        for (std::size_t i = 0; i < topo.input_size; ++i) {
            char buf[16];
            std::snprintf(buf, sizeof(buf), "IN %zu", i);
            label_strings.push_back(buf);
        }
    }

    // Draw input nodes with labels and activation bars
    // display_order maps visual position -> data index.
    // If empty, identity mapping (visual pos == data index).
    bool has_reorder = !config.display_order.empty();

    for (std::size_t i = 0; i < columns[0].size(); ++i) {
        // di = data index for this visual position
        std::size_t di = has_reorder && i < config.display_order.size() ? config.display_order[i] : i;

        // Color by input type: use caller-provided colors or default neutral blue
        uint8_t lr = 68, lg = 136, lb = 255;
        if (!config.input_colors.empty() && di < config.input_colors.size()) {
            lr = config.input_colors[di].r;
            lg = config.input_colors[di].g;
            lb = config.input_colors[di].b;
        }

        // Input activation: use actual input value (data index di)
        float input_act = 0.5f;
        if (di < config.input_values.size()) {
            input_act = std::clamp(std::abs(config.input_values[di]), 0.0f, 1.0f);
        }

        // Visual position i, data from di
        render_filled_circle(config.renderer, columns[0][i].x, columns[0][i].y, 6.0f, input_act, 68, 136, 255);

        if (di < label_strings.size()) {
            draw_tiny_text(config.renderer,
                static_cast<int>(columns[0][i].x) - 55 * config.text_scale,
                static_cast<int>(columns[0][i].y) - 3,
                label_strings[di].c_str(), config.text_scale, lr, lg, lb);
        }

        // Activation bar
        if (di < config.input_values.size()) {
            float val = config.input_values[di];
            float bar_act = std::clamp(std::abs(val), 0.0f, 1.0f);
            int bar_x = static_cast<int>(columns[0][i].x) + 6;
            int bar_y = static_cast<int>(columns[0][i].y) - 2;
            int bar_w = static_cast<int>(bar_act * 30.0f);
            if (bar_w > 0) {
                // Green for positive, red for negative
                uint8_t br = (val < 0) ? static_cast<uint8_t>(200) : static_cast<uint8_t>(0);
                uint8_t bg = (val >= 0) ? static_cast<uint8_t>(200) : static_cast<uint8_t>(0);
                SDL_SetRenderDrawColor(config.renderer, br, bg, 100, 180);
                SDL_Rect bar = {bar_x, bar_y, bar_w, 4};
                SDL_RenderFillRect(config.renderer, &bar);
            }
        }
    }

    // Draw hidden + output layer nodes with activations
    for (std::size_t l = 0; l < topo.layers.size(); ++l) {
        bool is_output = (l == topo.layers.size() - 1);
        for (std::size_t i = 0; i < columns[l + 1].size(); ++i) {
            float act = (l < all_activations.size() && i < all_activations[l].size())
                            ? (all_activations[l][i] + 1.0f) / 2.0f  // tanh: -1..1 -> 0..1
                            : 0.5f;

            if (is_output) {
                render_filled_circle(config.renderer, columns[l + 1][i].x, columns[l + 1][i].y, 9.0f, act, 0, 255, 136);

                // Output activation bar
                int bar_x = static_cast<int>(columns[l + 1][i].x) + 10;
                int bar_y = static_cast<int>(columns[l + 1][i].y) - 3;
                int bar_w = static_cast<int>(act * 40.0f);
                SDL_SetRenderDrawColor(config.renderer, 0, 255, 136, 200);
                SDL_Rect bar = {bar_x, bar_y, bar_w, 6};
                SDL_RenderFillRect(config.renderer, &bar);

                // Output label
                if (i < config.output_labels.size()) {
                    // Use caller-provided output color or default green
                    uint8_t olr = 0, olg = 200, olb = 110;
                    if (i < config.output_colors.size()) {
                        olr = config.output_colors[i].r;
                        olg = config.output_colors[i].g;
                        olb = config.output_colors[i].b;
                    }
                    draw_tiny_text(config.renderer,
                        bar_x + bar_w + 4, bar_y - 1,
                        config.output_labels[i].c_str(), config.text_scale, olr, olg, olb);
                }
            } else {
                render_filled_circle(config.renderer, columns[l + 1][i].x, columns[l + 1][i].y, 6.0f, act, 255, 136, 68);
            }
        }
    }

    return result;
}

} // namespace neuralnet_ui

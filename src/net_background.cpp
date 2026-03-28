#include "neuralnet-ui/net_background.h"

#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

namespace neuralnet_ui {

namespace {

struct Node {
    float x;
    float y;
};

void draw_filled_circle(SDL_Renderer* renderer, int cx, int cy, float radius,
                        uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
    int ir = static_cast<int>(std::ceil(radius));
    for (int dy = -ir; dy <= ir; ++dy) {
        float half_w = std::sqrt(radius * radius - static_cast<float>(dy * dy));
        int x0 = cx - static_cast<int>(half_w);
        int x1 = cx + static_cast<int>(half_w);
        SDL_RenderDrawLine(renderer, x0, cy + dy, x1, cy + dy);
    }
}

} // namespace

SDL_Texture* generate_net_background(SDL_Renderer* renderer,
                                     const NetBackgroundConfig& config) {
    SDL_Texture* texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
        config.width, config.height);
    if (!texture) {
        return nullptr;
    }

    SDL_SetRenderTarget(renderer, texture);

    // 1. Fill with dark base
    SDL_SetRenderDrawColor(renderer, config.base_r, config.base_g, config.base_b, 255);
    SDL_RenderClear(renderer);

    // 2. Overlay accent color as edge flair
    if (config.accent_alpha > 0) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, config.accent_r, config.accent_g,
                               config.accent_b, config.accent_alpha);
        SDL_Rect full = {0, 0, config.width, config.height};
        SDL_RenderFillRect(renderer, &full);
    }

    // Generate nodes biased toward edges
    std::mt19937 rng(config.seed);
    std::uniform_real_distribution<float> dist_x(0.0f,
                                                 static_cast<float>(config.width));
    std::uniform_real_distribution<float> dist_y(0.0f,
                                                 static_cast<float>(config.height));
    std::uniform_real_distribution<float> dist_01(0.0f, 1.0f);

    std::vector<Node> nodes;
    nodes.reserve(static_cast<size_t>(config.num_nodes));

    for (int i = 0; i < config.num_nodes; ++i) {
        float x = dist_x(rng);
        float y = dist_y(rng);
        float nx = x / static_cast<float>(config.width);
        float ny = y / static_cast<float>(config.height);
        float radial =
            std::sqrt((nx - 0.5f) * (nx - 0.5f) + (ny - 0.5f) * (ny - 0.5f)) *
            2.0f;

        if (radial < config.clear_radius) {
            continue;
        }

        float prob = (radial - config.clear_radius) / (1.0f - config.clear_radius);
        prob = std::clamp(prob, 0.0f, 1.0f);

        if (dist_01(rng) < prob) {
            nodes.push_back({x, y});
        }
    }

    // Draw connections between nearby nodes
    float max_conn_dist =
        config.connection_radius *
        static_cast<float>(std::max(config.width, config.height));

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (size_t i = 0; i < nodes.size(); ++i) {
        for (size_t j = i + 1; j < nodes.size(); ++j) {
            float dx = nodes[i].x - nodes[j].x;
            float dy = nodes[i].y - nodes[j].y;
            float d = std::sqrt(dx * dx + dy * dy);

            if (d < max_conn_dist) {
                float dist_factor = 1.0f - d / max_conn_dist;

                auto alpha = static_cast<uint8_t>(
                    static_cast<float>(config.line_max_alpha) * dist_factor);

                SDL_SetRenderDrawColor(renderer, config.r, config.g, config.b,
                                       alpha);
                SDL_RenderDrawLine(renderer, static_cast<int>(nodes[i].x),
                                   static_cast<int>(nodes[i].y),
                                   static_cast<int>(nodes[j].x),
                                   static_cast<int>(nodes[j].y));
            }
        }
    }

    // Draw nodes with glow effect — layered circles from outer glow to bright core
    for (const auto& node : nodes) {
        int cx = static_cast<int>(node.x);
        int cy = static_cast<int>(node.y);

        // 5 glow layers from glow_radius down to core_radius
        constexpr int GLOW_LAYERS = 5;
        for (int layer = 0; layer < GLOW_LAYERS; ++layer) {
            float t = static_cast<float>(layer) / static_cast<float>(GLOW_LAYERS - 1);
            float radius = config.glow_radius * (1.0f - t) + config.core_radius * t;
            // Alpha ramps up quadratically toward center
            auto a = static_cast<uint8_t>(
                static_cast<float>(config.core_alpha) * t * t);
            draw_filled_circle(renderer, cx, cy, radius,
                               config.r, config.g, config.b, a);
        }
        // Bright core
        draw_filled_circle(renderer, cx, cy, config.core_radius,
                           config.r, config.g, config.b, config.core_alpha);
        // White hotspot
        if (config.white_hotspot) {
            draw_filled_circle(renderer, cx, cy, 1.0f, 255, 255, 255, 200);
        }
    }

    // Radial vignette — draw a dark gradient from center outward to erase
    // any nodes/lines that landed near the center, creating a clean clear zone.
    {
        float center_x = static_cast<float>(config.width) * 0.5f;
        float center_y = static_cast<float>(config.height) * 0.5f;
        float min_dim = static_cast<float>(std::min(config.width, config.height));
        float inner_r = config.clear_radius * min_dim * 0.5f;
        float outer_r = inner_r * config.vignette_falloff;
        for (int step = config.vignette_steps; step >= 0; --step) {
            float t = static_cast<float>(step) /
                      static_cast<float>(config.vignette_steps);
            float radius = inner_r + (outer_r - inner_r) * t;
            auto alpha = static_cast<uint8_t>(255.0f * (1.0f - t) * (1.0f - t));
            draw_filled_circle(renderer, static_cast<int>(center_x),
                               static_cast<int>(center_y), radius,
                               config.base_r, config.base_g, config.base_b, alpha);
        }
    }

    SDL_SetRenderTarget(renderer, nullptr);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    return texture;
}

void render_net_background(SDL_Renderer* renderer, SDL_Texture* texture, int x,
                           int y, int w, int h, uint8_t alpha) {
    SDL_SetTextureAlphaMod(texture, alpha);
    SDL_Rect dst = {x, y, w, h};
    SDL_RenderCopy(renderer, texture, nullptr, &dst);
}

} // namespace neuralnet_ui

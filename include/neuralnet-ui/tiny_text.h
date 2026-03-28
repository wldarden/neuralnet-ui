#pragma once

#include <cstdint>

struct SDL_Renderer;

namespace neuralnet_ui {

/// Draw text using a 3x5 bitmap font via SDL_Renderer.
void draw_tiny_text(SDL_Renderer* renderer,
                    int x, int y, const char* text, int scale,
                    uint8_t r, uint8_t g, uint8_t b);

/// Draw a filled circle with activation-based alpha.
/// Alpha = 40 + clamp(activation, 0, 1) * 215.
void render_filled_circle(SDL_Renderer* renderer,
                          float x, float y, float radius, float activation,
                          uint8_t r, uint8_t g, uint8_t b);

} // namespace neuralnet_ui

#include <neuralnet-ui/tiny_text.h>

#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace neuralnet_ui {

namespace {

// 3x5 bitmap font — each char is 5 rows of 3 bits (stored as uint8_t per row)
// Bit order: MSB = left pixel, so 0b111 = 3 pixels across
struct Glyph { uint8_t rows[5]; };

// clang-format off
constexpr Glyph FONT[] = {
    // A-Z (indices 0-25)
    {{0b111, 0b101, 0b111, 0b101, 0b101}}, // A
    {{0b110, 0b101, 0b110, 0b101, 0b110}}, // B
    {{0b111, 0b100, 0b100, 0b100, 0b111}}, // C
    {{0b110, 0b101, 0b101, 0b101, 0b110}}, // D
    {{0b111, 0b100, 0b110, 0b100, 0b111}}, // E
    {{0b111, 0b100, 0b110, 0b100, 0b100}}, // F
    {{0b111, 0b100, 0b101, 0b101, 0b111}}, // G
    {{0b101, 0b101, 0b111, 0b101, 0b101}}, // H
    {{0b111, 0b010, 0b010, 0b010, 0b111}}, // I
    {{0b001, 0b001, 0b001, 0b101, 0b010}}, // J
    {{0b101, 0b101, 0b110, 0b101, 0b101}}, // K
    {{0b100, 0b100, 0b100, 0b100, 0b111}}, // L
    {{0b101, 0b111, 0b111, 0b101, 0b101}}, // M
    {{0b101, 0b111, 0b111, 0b111, 0b101}}, // N
    {{0b010, 0b101, 0b101, 0b101, 0b010}}, // O
    {{0b110, 0b101, 0b110, 0b100, 0b100}}, // P
    {{0b010, 0b101, 0b101, 0b110, 0b011}}, // Q
    {{0b110, 0b101, 0b110, 0b101, 0b101}}, // R
    {{0b011, 0b100, 0b010, 0b001, 0b110}}, // S
    {{0b111, 0b010, 0b010, 0b010, 0b010}}, // T
    {{0b101, 0b101, 0b101, 0b101, 0b111}}, // U
    {{0b101, 0b101, 0b101, 0b101, 0b010}}, // V
    {{0b101, 0b101, 0b111, 0b111, 0b101}}, // W
    {{0b101, 0b101, 0b010, 0b101, 0b101}}, // X
    {{0b101, 0b101, 0b010, 0b010, 0b010}}, // Y
    {{0b111, 0b001, 0b010, 0b100, 0b111}}, // Z
    // 0-9 (indices 26-35)
    {{0b111, 0b101, 0b101, 0b101, 0b111}}, // 0
    {{0b010, 0b110, 0b010, 0b010, 0b111}}, // 1
    {{0b111, 0b001, 0b111, 0b100, 0b111}}, // 2
    {{0b111, 0b001, 0b111, 0b001, 0b111}}, // 3
    {{0b101, 0b101, 0b111, 0b001, 0b001}}, // 4
    {{0b111, 0b100, 0b111, 0b001, 0b111}}, // 5
    {{0b111, 0b100, 0b111, 0b101, 0b111}}, // 6
    {{0b111, 0b001, 0b001, 0b001, 0b001}}, // 7
    {{0b111, 0b101, 0b111, 0b101, 0b111}}, // 8
    {{0b111, 0b101, 0b111, 0b001, 0b111}}, // 9
};
// clang-format on

// Special chars (indices 36+)
constexpr Glyph FONT_SPECIAL[] = {
    {{0b000, 0b000, 0b000, 0b000, 0b010}}, // . (36)
    {{0b000, 0b000, 0b111, 0b000, 0b000}}, // - (37)
    {{0b001, 0b001, 0b010, 0b100, 0b100}}, // / (38)
    {{0b000, 0b010, 0b111, 0b010, 0b000}}, // + (39)
    {{0b000, 0b010, 0b000, 0b010, 0b000}}, // : (40)
};

int glyph_index(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= '0' && c <= '9') return 26 + (c - '0');
    if (c == '.') return 100;  // special
    if (c == '-') return 101;
    if (c == '/') return 102;
    if (c == '+') return 103;
    if (c == ':') return 104;
    return -1;
}

const Glyph& get_glyph(int idx) {
    if (idx >= 100) return FONT_SPECIAL[idx - 100];
    return FONT[idx];
}

} // namespace

void render_filled_circle(SDL_Renderer* renderer,
                          float x, float y, float radius, float activation,
                          uint8_t r, uint8_t g, uint8_t b) {
    auto a = static_cast<uint8_t>(40 + std::clamp(activation, 0.0f, 1.0f) * 215);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);

    // Simple filled circle approximation
    int cx = static_cast<int>(x);
    int cy = static_cast<int>(y);
    int rad = static_cast<int>(radius);
    for (int dy = -rad; dy <= rad; ++dy) {
        int dx = static_cast<int>(std::sqrt(static_cast<float>(rad * rad - dy * dy)));
        SDL_RenderDrawLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

void draw_tiny_text(SDL_Renderer* renderer,
                    int x, int y, const char* text, int scale,
                    uint8_t r, uint8_t g, uint8_t b) {
    SDL_SetRenderDrawColor(renderer, r, g, b, 220);
    int cursor_x = x;
    for (const char* p = text; *p; ++p) {
        int idx = glyph_index(*p);
        if (idx < 0) {
            cursor_x += 4 * scale;  // space for unknown chars
            continue;
        }
        const auto& glyph = get_glyph(idx);
        for (int row = 0; row < 5; ++row) {
            for (int col = 0; col < 3; ++col) {
                if (glyph.rows[row] & (0b100 >> col)) {
                    SDL_Rect px = {cursor_x + col * scale, y + row * scale, scale, scale};
                    SDL_RenderFillRect(renderer, &px);
                }
            }
        }
        cursor_x += 4 * scale;  // 3px char + 1px gap
    }
}

} // namespace neuralnet_ui

#pragma once

/// @file net_background.h
/// @brief Procedural background texture generator for neural network viewers.
///
/// Generates a dark, atmospheric background with glowing nodes and connection
/// lines concentrated at the edges, leaving a clear center for overlaying
/// content (neural net visualizations, UI panels, etc).
///
/// ## Quick start
///
///   neuralnet_ui::NetBackgroundConfig cfg;
///   cfg.r = 0; cfg.g = 200; cfg.b = 200;  // teal nodes
///   SDL_Texture* bg = neuralnet_ui::generate_net_background(renderer, cfg);
///
///   // Each frame:
///   neuralnet_ui::render_net_background(renderer, bg, x, y, w, h);
///
///   // Cleanup:
///   SDL_DestroyTexture(bg);
///
/// ## Rendering layers (generated in order)
///
///   1. Dark base fill         — solid color, fills the entire texture
///   2. Accent overlay         — optional tinted fill at low alpha for edge color
///   3. Connection lines       — drawn between nearby nodes, alpha fades with distance
///   4. Node glows             — layered circles from soft outer glow to bright core
///   5. Radial vignette        — dark gradient erasing center content for a clear zone
///
/// ## Color system
///
///   - `r, g, b`               — Primary color for nodes and connection lines
///   - `base_r, base_g, base_b` — Dark base color (the "void" in the center and vignette)
///   - `accent_r, accent_g, accent_b, accent_alpha` — Optional edge flair tint
///
///   The accent layer is drawn BETWEEN the base fill and the nodes, so the
///   vignette (which uses base color) erases it from the center, leaving the
///   accent visible only at the edges.
///
/// ## Node placement
///
///   Nodes are placed randomly but biased toward edges using a radial
///   probability test. Nodes inside `clear_radius` are always rejected.
///   Nodes between `clear_radius` and the edge have increasing acceptance
///   probability. `num_nodes` is the candidate count — the actual node count
///   is lower because many candidates are rejected.
///
/// ## Reuse
///
///   The texture is generated once and can be reused across frames.
///   Call `render_net_background()` to blit it into any rectangle at any
///   alpha level. Change `seed` for a different random pattern.
///   Change `render_alpha` to control overall brightness when composited.

#include <cstdint>

struct SDL_Renderer;
struct SDL_Texture;

namespace neuralnet_ui {

/// Configuration for procedural background generation.
/// All fields have sensible defaults — construct and override what you need.
struct NetBackgroundConfig {

    // --- Texture dimensions ---
    int width = 512;             ///< Texture width in pixels (generated once)
    int height = 512;            ///< Texture height in pixels (generated once)

    // --- Primary color (nodes and connection lines) ---
    uint8_t r = 255;             ///< Node/line red component
    uint8_t g = 153;             ///< Node/line green component
    uint8_t b = 0;               ///< Node/line blue component (default: orange)

    // --- Base color (dark background and vignette) ---
    uint8_t base_r = 15;         ///< Base fill red (also used by vignette)
    uint8_t base_g = 15;         ///< Base fill green
    uint8_t base_b = 25;         ///< Base fill blue (default: near-black with blue tint)

    // --- Accent color (optional edge flair, drawn over base, erased by vignette) ---
    uint8_t accent_r = 255;      ///< Accent red
    uint8_t accent_g = 219;      ///< Accent green
    uint8_t accent_b = 0;        ///< Accent blue (default: gold)
    uint8_t accent_alpha = 40;   ///< Accent visibility (0 = off, 255 = solid)

    // --- Node appearance ---
    int num_nodes = 80;          ///< Candidate count (actual count is lower due to radial rejection)
    float glow_radius = 20.0f;   ///< Outermost glow ring radius in pixels
    float core_radius = 2.5f;    ///< Bright center dot radius in pixels
    uint8_t core_alpha = 180;    ///< Brightness of the core circle (0-255)
    bool white_hotspot = true;   ///< Tiny white dot at the very center of each node

    // --- Connection lines ---
    float connection_radius = 0.25f;  ///< Max distance to draw edges (fraction of max dimension)
    uint8_t line_max_alpha = 120;     ///< Alpha of the closest/brightest connections (0-255)

    // --- Radial vignette (dark center clear zone) ---
    float clear_radius = 0.65f;       ///< Center clear zone size (fraction of min dimension, 0-1)
    float vignette_falloff = 2.0f;    ///< How far vignette gradient extends (multiplier of clear_radius)
    int vignette_steps = 40;          ///< Gradient smoothness (more steps = smoother, slower)

    // --- Compositing ---
    uint8_t render_alpha = 180;  ///< Overall texture alpha when blitted (0 = invisible, 255 = full)

    // --- Random seed ---
    uint32_t seed = 42;          ///< RNG seed for reproducible patterns (change for different layouts)
};

/// Generate a procedural network-pattern background texture.
///
/// Creates an SDL_Texture with glowing nodes and connections arranged in
/// a ring pattern around the edges, with a dark clear center. The texture
/// is generated once and can be reused across frames via render_net_background().
///
/// @param renderer  The SDL_Renderer to create the texture on
/// @param config    Generation parameters (colors, node count, layout, etc.)
/// @return          An SDL_Texture owned by the caller. Destroy with SDL_DestroyTexture().
///                  Returns nullptr on failure.
///
/// @note The texture uses SDL_BLENDMODE_BLEND, suitable for compositing
///       over other content.
[[nodiscard]] SDL_Texture* generate_net_background(
    SDL_Renderer* renderer, const NetBackgroundConfig& config);

/// Blit a background texture into a rectangle with alpha blending.
///
/// Call this each frame to draw the background. The texture is stretched
/// to fill the destination rectangle. Use `alpha` to control overall
/// brightness (or pass config.render_alpha for the configured default).
///
/// @param renderer  The SDL_Renderer to draw to
/// @param texture   Texture returned by generate_net_background()
/// @param x, y      Top-left corner of the destination rectangle
/// @param w, h      Size of the destination rectangle
/// @param alpha     Overall alpha for this blit (0-255, default 100)
void render_net_background(SDL_Renderer* renderer, SDL_Texture* texture,
                           int x, int y, int w, int h, uint8_t alpha = 100);

} // namespace neuralnet_ui

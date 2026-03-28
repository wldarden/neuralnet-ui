# neuralnet-ui — Neural Net Rendering Library

- **GitHub:** [wldarden/neuralnet-ui](https://github.com/wldarden/neuralnet-ui)
- **Dependencies:** [neuralnet](https://github.com/wldarden/neuralnet) (expected at `../neuralnet/`), SDL2, ImGui
- **Used by:** NeuroFlyer

SDL2/ImGui rendering companion for the neuralnet library. Provides reusable neural net visualization that projects can wrap with domain-specific logic.

## Headers

| Header | Purpose |
|--------|---------|
| `render_neural_net.h` | Full interactive net renderer (nodes, connections, weights, hover inspection) |
| `render_net_topology.h` | Lightweight topology-only preview (no weights, no interaction) |
| `net_background.h` | Procedural starfield/nebula backgrounds for net panels |
| `tiny_text.h` | Bitmap font for scaled text rendering in net views |

## Two-Layer Design

Projects use a two-layer rendering approach:
1. **Generic layer** (this library): `render_neural_net()` / `render_net_topology()` — handles layout, drawing, interaction
2. **Project-specific wrapper**: e.g., NeuroFlyer's `render_variant_net()` — configures the generic renderer with domain-specific data (sensor colors, node labels, ship design)

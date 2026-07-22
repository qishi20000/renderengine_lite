#pragma once
#include "renderer/RenderPass.h"

namespace rel::renderer {

// Optional pass: 2D overlays (parking guide lines, radar distance markers,
// logo) drawn as simple textured quads with alpha blending
// (materials/overlay_ui.*). Runs last, before PresentPass.
class OverlayUIPass final : public RenderPass {
public:
    void execute(Engine& engine, View& view, backend::DriverApi& driver) override;
};

} // namespace rel::renderer

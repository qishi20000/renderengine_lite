#pragma once
#include "renderer/RenderPass.h"

namespace rel::renderer {

// The core AVM pass: renders the bowl/plane mesh with the
// materials/surround_view.{vert,frag} material, which does fisheye unwarp +
// multi-camera UV sampling + overlap-region blending in a single pass
// (see ARCHITECTURE.md sections 4 and 7 for why this must stay single-pass
// on Adreno's tile-based architecture). Reads N camera external textures
// bound by avm::CameraStreamManager before this pass runs.
class SurroundViewPass final : public RenderPass {
public:
    void setup(Engine& engine, View& view) override;
    void execute(Engine& engine, View& view, backend::DriverApi& driver) override;
};

} // namespace rel::renderer

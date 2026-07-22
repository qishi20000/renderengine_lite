#pragma once
#include "renderer/RenderPass.h"

namespace rel::renderer {

// Optional pass: draws the 3D car model on top of the surround view using a
// simple Blinn-Phong material (materials/simple_lit.*) — deliberately no
// PBR/IBL, see ARCHITECTURE.md section 12.
class CarModelPass final : public RenderPass {
public:
    void execute(Engine& engine, View& view, backend::DriverApi& driver) override;
};

} // namespace rel::renderer

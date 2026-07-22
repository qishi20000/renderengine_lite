#include "renderer/passes/SurroundViewPass.h"

namespace rel::renderer {

void SurroundViewPass::setup(Engine& engine, View& view) {
    // No lazy FBO/program setup needed yet: the bowl mesh's Material is
    // created up-front by avm::BowlMeshGenerator and bound per-Renderable
    // already. This override exists as the natural place to add e.g.
    // resolution-change-triggered FBO recreation later.
    (void)engine; (void)view;
}

void SurroundViewPass::execute(Engine& engine, View& view, backend::DriverApi& driver) {
    // TODO(M4): once the scene carries more than the bowl mesh, filter
    // drawAllRenderables to only the surround-view layer (e.g. via a
    // per-Renderable layer mask) instead of drawing everything here.
    drawAllRenderables(engine, view, driver);
}

} // namespace rel::renderer

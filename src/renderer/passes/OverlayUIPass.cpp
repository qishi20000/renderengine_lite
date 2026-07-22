#include "renderer/passes/OverlayUIPass.h"

namespace rel::renderer {

void OverlayUIPass::execute(Engine& engine, View& view, backend::DriverApi& driver) {
    // TODO(M5): same layer-filtering caveat as CarModelPass — left as a
    // no-op until Scene/RenderableManager support drawing a distinct UI
    // overlay layer without re-drawing the surround view mesh.
    (void)engine; (void)view; (void)driver;
}

} // namespace rel::renderer

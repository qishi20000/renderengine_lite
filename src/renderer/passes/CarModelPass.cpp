#include "renderer/passes/CarModelPass.h"

namespace rel::renderer {

void CarModelPass::execute(Engine& engine, View& view, backend::DriverApi& driver) {
    // TODO(M5): draw only car-model entities once Scene/RenderableManager
    // support a layer/tag filter (see RenderPass::drawAllRenderables TODO
    // and ARCHITECTURE.md section 4). Left as a no-op for now — enabling
    // this before layer filtering exists would redraw the bowl mesh a
    // second time on top of itself.
    (void)engine; (void)view; (void)driver;
}

} // namespace rel::renderer

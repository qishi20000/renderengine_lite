#include "rel/Renderer.h"

#include "rel/SwapChain.h"
#include "rel/View.h"

#include "engine/EngineImpl.h"
#include "engine/ResourceImpls.h"
#include "renderer/RendererImpl.h"

namespace rel {

Renderer::~Renderer() { delete mImpl; }

bool Renderer::beginFrame(SwapChain* swapChain) {
    mImpl->activeSwapChain = swapChain;
    mImpl->engine->getImpl()->driver->makeCurrent();
    return true;
}

void Renderer::render(View& view) {
    // Deliberately ONE physical GL render pass (begin/end) shared by every
    // logical RenderPass in the chain — see ARCHITECTURE.md sections 4 & 7:
    // on Adreno's tile-based architecture, splitting SurroundView/CarModel/
    // OverlayUI into separate FBO passes would cost real bandwidth for no
    // benefit, since they all draw into the same target back-to-back.
    auto& driver = *mImpl->engine->getImpl()->driver;

    backend::RenderPassDescriptor desc;
    desc.target = driver.getDefaultRenderTarget();
    desc.clearColor[0] = 0.0f; desc.clearColor[1] = 0.0f;
    desc.clearColor[2] = 0.0f; desc.clearColor[3] = 1.0f;
    desc.clearColorBuffer = true;
    desc.clearDepthBuffer = true;

    driver.beginRenderPass(desc);
    for (auto& pass : mImpl->passes) {
        pass->setup(*mImpl->engine, view);
        pass->execute(*mImpl->engine, view, driver);
    }
    driver.endRenderPass();
}

void Renderer::endFrame() {
    mImpl->engine->getImpl()->driver->commit();
}

} // namespace rel

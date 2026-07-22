#pragma once
#include <cstdint>
#include "rel/rel_fwd.h"

namespace rel {

// A View binds a Scene + Camera + viewport + which RenderPass pipeline to
// run. In this lite engine there is typically exactly one View ("the AVM
// surround view"), possibly a second for a debug/desktop preview — nothing
// like Filament's multi-view compositing is needed.
class View {
public:
    // Public (unlike backend::Viewport, which is an implementation detail):
    // Renderer::render() (src/renderer/Renderer.cpp) reads this every frame
    // to fill in backend::RenderPassDescriptor::viewport, since the default
    // render target (the window surface) has no size of its own to fall
    // back on — see the comment on backend::Viewport in
    // src/backend/DriverApi.h for why that matters.
    struct Viewport { int32_t x = 0, y = 0; uint32_t width = 0, height = 0; };

    void setScene(Scene* scene) { mScene = scene; }
    void setCamera(Camera* camera) { mCamera = camera; }
    void setViewport(int32_t x, int32_t y, uint32_t width, uint32_t height);

    Scene* getScene() const { return mScene; }
    Camera* getCamera() const { return mCamera; }
    const Viewport& getViewport() const { return mViewport; }

private:
    friend class Engine;
    View() = default;
    Scene* mScene = nullptr;
    Camera* mCamera = nullptr;
    Viewport mViewport;
};

} // namespace rel

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
    void setScene(Scene* scene) { mScene = scene; }
    void setCamera(Camera* camera) { mCamera = camera; }
    void setViewport(int32_t x, int32_t y, uint32_t width, uint32_t height);

    Scene* getScene() const { return mScene; }
    Camera* getCamera() const { return mCamera; }

private:
    friend class Engine;
    View() = default;
    Scene* mScene = nullptr;
    Camera* mCamera = nullptr;
    struct Viewport { int32_t x = 0, y = 0; uint32_t width = 0, height = 0; } mViewport;
};

} // namespace rel

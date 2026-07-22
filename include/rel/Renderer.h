#pragma once
#include "rel/rel_fwd.h"

namespace rel {

// Drives one frame: runs the configured RenderPass chain against a View
// (see src/renderer/RenderPass.h). Deliberately NOT a FrameGraph — see
// ARCHITECTURE.md section 4 for the rationale specific to AVM's fixed,
// small pass count.
class Renderer {
public:
    ~Renderer();

    bool beginFrame(SwapChain* swapChain);
    void render(View& view);
    void endFrame();

    // Internal use only: see Camera::Impl comment above.
    struct Impl;
    Impl* getImpl() const { return mImpl; }

private:
    friend class Engine;
    Renderer() = default;
    Impl* mImpl = nullptr;
};

} // namespace rel

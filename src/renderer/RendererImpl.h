#pragma once
#include <memory>
#include <vector>

#include "rel/Renderer.h"
#include "renderer/RenderPass.h"

namespace rel {

struct Renderer::Impl {
    Engine* engine = nullptr;
    SwapChain* activeSwapChain = nullptr;
    // Default AVM pass chain, populated in Engine::create() (see
    // ARCHITECTURE.md section 4). Order matters: this vector *is* the
    // pipeline, in place of a FrameGraph.
    std::vector<std::unique_ptr<renderer::RenderPass>> passes;
};

} // namespace rel

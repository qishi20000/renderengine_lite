#pragma once
#include "rel/rel_fwd.h"

namespace rel {

// Wraps the platform window surface (ANativeWindow on Android). Passed to
// Renderer::beginFrame(); mirrors Filament's SwapChain concept but with no
// headless/PBuffer support beyond what the desktop preview needs.
class SwapChain {
public:
private:
    friend class Engine;
    SwapChain() = default;
    struct Impl;
    Impl* mImpl = nullptr;
};

} // namespace rel

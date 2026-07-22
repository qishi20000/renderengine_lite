#pragma once
#include "rel/rel_fwd.h"

namespace rel {

// Wraps the platform window surface (ANativeWindow on Android). Passed to
// Renderer::beginFrame(); mirrors Filament's SwapChain concept but with no
// headless/PBuffer support beyond what the desktop preview needs.
class SwapChain {
public:
    ~SwapChain();

    // Internal use only: see Camera::Impl comment above.
    struct Impl;
    Impl* getImpl() const { return mImpl; }

private:
    friend class Engine;
    SwapChain() = default;
    Impl* mImpl = nullptr;
};

} // namespace rel

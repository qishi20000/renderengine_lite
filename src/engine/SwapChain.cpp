#include "rel/SwapChain.h"

#include "engine/ResourceImpls.h"

namespace rel {

// SwapChain::Impl (see ResourceImpls.h) is just a raw, non-owning
// nativeWindow pointer today — actual EGL surface lifetime is owned by
// backend::Platform (see PlatformEGLAndroid). This destructor only needs to
// exist somewhere with Impl's full definition visible; Engine::destroy()
// is where a real backend-level teardown hook would go if SwapChain grows
// backend-owned resources later.
SwapChain::~SwapChain() { delete mImpl; }

} // namespace rel

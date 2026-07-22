#include "rel/Engine.h"

#include <algorithm>
#include <cstdio>

#include "engine/EngineImpl.h"
#include "engine/ResourceImpls.h"
#include "renderer/RendererImpl.h"
#include "renderer/passes/CarModelPass.h"
#include "renderer/passes/OverlayUIPass.h"
#include "renderer/passes/SurroundViewPass.h"
#include "backend/opengl/GLDriver.h"

#if defined(__ANDROID__)
#include "platform/android/PlatformEGLAndroid.h"
#endif

namespace rel {

Engine::Engine() : mImpl(std::make_unique<Impl>()) {}
Engine::~Engine() = default;

std::unique_ptr<Engine> Engine::create(void* nativeWindow) {
    // NOTE: this is the one place in the whole engine that knows about a
    // concrete backend. Everything above this line (Scene/View/Renderer/
    // Material/...) only ever talks to backend::DriverApi. A future
    // Vulkan backend only needs a new branch here (e.g. selected by a
    // build flag or runtime capability probe) — see ARCHITECTURE.md
    // section 9.
    auto engine = std::unique_ptr<Engine>(new Engine());
    auto& impl = *engine->mImpl;

#if defined(__ANDROID__)
    auto platform = std::make_unique<backend::android::PlatformEGLAndroid>();
    if (!platform->createContext(nativeWindow)) {
        std::fprintf(stderr, "[REL] Engine::create: failed to create EGL context\n");
        return nullptr;
    }
    impl.platform = std::move(platform);
#else
    (void)nativeWindow;
    std::fprintf(stderr,
        "[REL] Engine::create: no Platform implementation for this OS. "
        "Build for Android, or provide a PlatformEGLDesktop (see "
        "samples/desktop_preview/README.md) before calling Engine::create().\n");
    return nullptr;
#endif

    impl.driver = std::make_unique<backend::opengl::GLDriver>(*impl.platform);
    impl.driver->makeCurrent();

    impl.swapChain = std::unique_ptr<SwapChain>(new SwapChain());
    impl.swapChain->mImpl = new SwapChain::Impl{nativeWindow};

    impl.renderer = std::unique_ptr<Renderer>(new Renderer());
    impl.renderer->mImpl = new Renderer::Impl();
    impl.renderer->mImpl->engine = engine.get();

    // Default AVM pass chain (ARCHITECTURE.md section 4). CarModelPass /
    // OverlayUIPass are optional in principle; kept unconditional here for
    // simplicity until a use case needs runtime pass toggling.
    impl.renderer->mImpl->passes.push_back(std::make_unique<renderer::SurroundViewPass>());
    impl.renderer->mImpl->passes.push_back(std::make_unique<renderer::CarModelPass>());
    impl.renderer->mImpl->passes.push_back(std::make_unique<renderer::OverlayUIPass>());

    return engine;
}

Entity Engine::createEntity() { return mImpl->entityManager.create(); }
void Engine::destroyEntity(Entity e) { mImpl->entityManager.destroy(e); }

Scene* Engine::createScene() {
    auto scene = std::unique_ptr<Scene>(new Scene());
    Scene* ptr = scene.get();
    mImpl->scenes.push_back(std::move(scene));
    return ptr;
}

View* Engine::createView() {
    auto view = std::unique_ptr<View>(new View());
    View* ptr = view.get();
    mImpl->views.push_back(std::move(view));
    return ptr;
}

Camera* Engine::createCamera(Entity e) {
    auto camera = std::unique_ptr<Camera>(new Camera(e));
    camera->mImpl = new Camera::Impl();
    Camera* ptr = camera.get();
    mImpl->cameras.push_back(std::move(camera));
    return ptr;
}

Renderer* Engine::getRenderer() { return mImpl->renderer.get(); }
SwapChain* Engine::getSwapChain() { return mImpl->swapChain.get(); }

namespace {
template <typename T>
void eraseOwned(std::vector<std::unique_ptr<T>>& pool, T* ptr) {
    pool.erase(std::remove_if(pool.begin(), pool.end(),
                               [&](const std::unique_ptr<T>& p) { return p.get() == ptr; }),
               pool.end());
}
} // namespace

void Engine::destroy(Scene* s) { eraseOwned(mImpl->scenes, s); }
void Engine::destroy(View* v) { eraseOwned(mImpl->views, v); }
void Engine::destroy(Material* m) { eraseOwned(mImpl->materials, m); }
void Engine::destroy(Texture* t) { eraseOwned(mImpl->textures, t); }
void Engine::destroy(VertexBuffer* vb) { eraseOwned(mImpl->vertexBuffers, vb); }
void Engine::destroy(IndexBuffer* ib) { eraseOwned(mImpl->indexBuffers, ib); }

} // namespace rel

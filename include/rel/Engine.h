#pragma once
#include <memory>
#include "rel/rel_fwd.h"
#include "rel/Entity.h"

namespace rel {

// Engine is the top-level factory/owner for every REL object: it owns the
// backend::DriverApi instance (GLDriver today), the EntityManager and its
// component managers (Transform/Renderable/...), and every Scene/View/
// Material/Texture/Buffer created through it. There is normally exactly one
// Engine per process, created once on the render thread after the EGL
// context/surface is available.
//
// Usage sketch (see samples/ once implemented):
//   auto engine = Engine::create(nativeWindow);
//   Scene* scene = engine->createScene();
//   View* view = engine->createView();
//   view->setScene(scene);
//   ...
//   engine->getRenderer()->render(*view);
class Engine {
public:
    // `nativeWindow` is an ANativeWindow* on Android (opaque here to keep
    // this header platform-free); on desktop preview builds it is whatever
    // the ANGLE/EGL shim expects.
    static std::unique_ptr<Engine> create(void* nativeWindow);
    ~Engine();

    Entity createEntity();
    void destroyEntity(Entity e);

    // Creates (on first call for `entity`) or updates the entity's local
    // transform. `mat4x4` is a 16-float column-major matrix (GLSL/GLES
    // convention). See rel::RenderableBuilder for attaching geometry to an
    // entity, and RenderPass::drawAllRenderables (src/renderer/RenderPass.cpp)
    // for how this feeds into the "u_model" uniform every frame.
    void setTransform(Entity entity, const float* mat4x4);

    Scene* createScene();
    View* createView();
    Camera* createCamera(Entity e);
    Renderer* getRenderer();
    SwapChain* getSwapChain();

    // Destruction of REL-owned objects goes through the Engine so it can
    // route to the backend's (possibly deferred) resource destruction.
    void destroy(Scene*);
    void destroy(View*);
    void destroy(Material*);
    void destroy(Texture*);
    void destroy(VertexBuffer*);
    void destroy(IndexBuffer*);

    // Internal use only (src/engine/, src/renderer/, src/avm/): Impl is an
    // incomplete type here and only fully defined in
    // src/engine/EngineImpl.h, so code outside the engine implementation
    // cannot dereference it even though this accessor is technically public.
    struct Impl;
    Impl* getImpl() const { return mImpl.get(); }

private:
    Engine();
    std::unique_ptr<Impl> mImpl;
};

} // namespace rel

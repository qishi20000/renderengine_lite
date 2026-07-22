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

private:
    Engine();
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace rel

#pragma once
#include <memory>
#include <vector>

#include "rel/Engine.h"
#include "rel/Camera.h"
#include "rel/IndexBuffer.h"
#include "rel/Material.h"
#include "rel/Renderer.h"
#include "rel/Scene.h"
#include "rel/SwapChain.h"
#include "rel/Texture.h"
#include "rel/VertexBuffer.h"
#include "rel/View.h"

#include "backend/DriverApi.h"
#include "backend/Platform.h"
#include "engine/EntityManager.h"
#include "engine/RenderableManager.h"
#include "engine/TransformManager.h"

namespace rel {

// The single owner of every REL object and of the backend. See
// ARCHITECTURE.md sections 1-3: Engine ties the ECS-lite managers to a
// concrete backend::DriverApi (GLDriver today) without either side knowing
// about the other's implementation details.
struct Engine::Impl {
    std::unique_ptr<backend::Platform> platform;
    std::unique_ptr<backend::DriverApi> driver;

    EntityManager entityManager;
    TransformManager transformManager;
    RenderableManager renderableManager;

    std::unique_ptr<SwapChain> swapChain;
    std::unique_ptr<Renderer> renderer;

    // Engine-owned object pools. Using vector<unique_ptr<T>> rather than a
    // free-list/handle pool here since Scene/View/Material/... churn is
    // very low (created once at startup for AVM) unlike the GPU resources
    // in backend::opengl::GLDriver, which do use a pooled allocator.
    std::vector<std::unique_ptr<Scene>> scenes;
    std::vector<std::unique_ptr<View>> views;
    std::vector<std::unique_ptr<Camera>> cameras;
    std::vector<std::unique_ptr<Material>> materials;
    std::vector<std::unique_ptr<Texture>> textures;
    std::vector<std::unique_ptr<VertexBuffer>> vertexBuffers;
    std::vector<std::unique_ptr<IndexBuffer>> indexBuffers;
};

} // namespace rel

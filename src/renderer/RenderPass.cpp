#include "renderer/RenderPass.h"

#include "rel/Camera.h"
#include "rel/MaterialInstance.h"
#include "rel/Scene.h"
#include "rel/Texture.h"
#include "rel/View.h"

#include "engine/EngineImpl.h"
#include "engine/ResourceImpls.h"

namespace rel::renderer {

namespace {

// Binds a MaterialInstance's staged parameters (uniforms/textures) onto the
// currently-bound program. Assumes bindProgram() was already called for
// this instance's program.
void applyMaterialParams(backend::DriverApi& driver, const MaterialInstance& mi) {
    uint32_t nextTextureUnit = 0;
    for (const auto& param : mi.getImpl()->params) {
        switch (param.kind) {
            case MaterialParam::Kind::Float4:
                driver.setUniformFloat4(param.name, param.f4);
                break;
            case MaterialParam::Kind::Mat4:
                driver.setUniformMat4(param.name, param.m4.data());
                break;
            case MaterialParam::Kind::Texture:
                if (param.tex) {
                    // TODO(M2): also bind the sampler index (u_xxx uniform
                    // = nextTextureUnit) once DriverApi exposes
                    // setUniformInt; GLSL samplers default to unit 0
                    // otherwise, which only works for single-texture
                    // materials like simple_lit/overlay_ui today.
                    driver.bindTexture(nextTextureUnit++, param.tex->getImpl()->handle, {});
                }
                break;
        }
    }
}

} // namespace

void RenderPass::drawAllRenderables(Engine& engine, View& view, backend::DriverApi& driver) {
    Scene* scene = view.getScene();
    Camera* camera = view.getCamera();
    if (!scene || !camera) return;

    const mat4 viewProj = camera->getImpl()->getViewProj();
    auto& renderableManager = engine.getImpl()->renderableManager;
    auto& transformManager = engine.getImpl()->transformManager;

    for (Entity entity : scene->getEntities()) {
        auto renderableInst = renderableManager.getInstance(entity);
        if (renderableInst == RenderableManager::kInvalidInstance) continue;

        const auto& renderable = renderableManager.get(renderableInst);
        if (!renderable.material) continue;

        driver.bindProgram(renderable.material->getImpl()->program);
        driver.setUniformMat4("u_viewProj", viewProj.data());

        auto xformInst = transformManager.getInstance(entity);
        if (xformInst != TransformManager::kInvalidInstance) {
            const mat4& world = transformManager.getWorldTransform(xformInst);
            driver.setUniformMat4("u_model", world.data());
        }

        applyMaterialParams(driver, *renderable.material);
        driver.setRasterState({});

        driver.draw(renderable.primitiveType, renderable.vb, renderable.ib, renderable.indexCount,
                    renderable.attributes.data(), static_cast<uint32_t>(renderable.attributes.size()));
    }
}

} // namespace rel::renderer

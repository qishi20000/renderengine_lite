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
                    const uint32_t unit = nextTextureUnit++;
                    driver.bindTexture(unit, param.tex->getImpl()->handle, {});
                    // Point the sampler uniform (which may be a single
                    // sampler2D or one element of a sampler array, e.g.
                    // "u_cameraY[2]") at the texture unit we just bound —
                    // GLSL ES does not infer this from bindTexture() alone.
                    driver.setUniformInt(param.name, static_cast<int>(unit));
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

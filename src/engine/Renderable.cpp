#include "rel/Renderable.h"

#include "engine/EngineImpl.h"
#include "engine/ResourceImpls.h"

namespace rel {

void RenderableBuilder::build(Engine& engine, Entity entity) {
    RenderableManager::Builder builder;
    builder.geometry(mVertexBuffer->getImpl()->handle, mIndexBuffer->getImpl()->handle,
                      toBackendPrimitiveType(mType), mIndexCount);
    for (const auto& attr : mAttributes) {
        builder.attribute(toBackendVertexAttribute(attr));
    }
    builder.material(mMaterial);
    builder.build(entity, engine.getImpl()->renderableManager);
}

} // namespace rel

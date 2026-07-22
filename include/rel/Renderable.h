#pragma once
#include <cstdint>
#include <vector>

#include "rel/rel_fwd.h"
#include "rel/Entity.h"
#include "rel/VertexBuffer.h" // VertexAttributeDesc, ElementType

namespace rel {

enum class PrimitiveType : uint8_t { Points, Lines, LineStrip, Triangles, TriangleStrip };

// Attaches geometry + a MaterialInstance to an Entity, making it render as
// part of any Scene the entity is added to (via drawAllRenderables, see
// src/renderer/RenderPass.cpp). Thin public-facing wrapper around the
// internal RenderableManager (src/engine/RenderableManager.h) — kept as its
// own header/class (mirroring VertexBuffer/IndexBuffer's Builder pattern)
// rather than folded into Scene, since "what an entity looks like" and
// "which entities a view considers" are separate concerns.
class RenderableBuilder {
public:
    RenderableBuilder& geometry(VertexBuffer* vb, IndexBuffer* ib, PrimitiveType type, uint32_t indexCount) {
        mVertexBuffer = vb; mIndexBuffer = ib; mType = type; mIndexCount = indexCount;
        return *this;
    }
    RenderableBuilder& attribute(const VertexAttributeDesc& attr) { mAttributes.push_back(attr); return *this; }
    RenderableBuilder& material(MaterialInstance* materialInstance) { mMaterial = materialInstance; return *this; }

    void build(Engine& engine, Entity entity);

private:
    VertexBuffer* mVertexBuffer = nullptr;
    IndexBuffer* mIndexBuffer = nullptr;
    PrimitiveType mType = PrimitiveType::Triangles;
    uint32_t mIndexCount = 0;
    std::vector<VertexAttributeDesc> mAttributes;
    MaterialInstance* mMaterial = nullptr;
};

} // namespace rel

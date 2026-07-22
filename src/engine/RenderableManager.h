#pragma once
#include <vector>
#include <unordered_map>

#include "rel/Entity.h"
#include "backend/DriverEnums.h"
#include "backend/Handles.h"
#include "utils/Math.h"

namespace rel {

class MaterialInstance;

// Sparse Entity -> renderable (geometry + material) storage. Mirrors
// Filament's RenderableManager but only carries the fields AVM scenes
// actually need (no skinning/morphing/multi-primitive-per-renderable/
// instancing — add if/when a concrete use case appears).
class RenderableManager {
public:
    using Instance = uint32_t;
    static constexpr Instance kInvalidInstance = static_cast<Instance>(-1);

    struct Builder {
        Builder& geometry(backend::VertexBufferHandle vb, backend::IndexBufferHandle ib,
                           backend::PrimitiveType type, uint32_t indexCount) {
            mVb = vb; mIb = ib; mType = type; mIndexCount = indexCount;
            return *this;
        }
        Builder& material(MaterialInstance* mi) { mMaterial = mi; return *this; }
        Builder& boundingBox(const Box& box) { mAabb = box; return *this; }

        Instance build(Entity e, RenderableManager& manager) {
            return manager.addRenderable(e, mVb, mIb, mType, mIndexCount, mMaterial, mAabb);
        }

    private:
        backend::VertexBufferHandle mVb;
        backend::IndexBufferHandle mIb;
        backend::PrimitiveType mType = backend::PrimitiveType::Triangles;
        uint32_t mIndexCount = 0;
        MaterialInstance* mMaterial = nullptr;
        Box mAabb;
    };

    struct Renderable {
        Entity entity;
        backend::VertexBufferHandle vb;
        backend::IndexBufferHandle ib;
        backend::PrimitiveType primitiveType;
        uint32_t indexCount;
        MaterialInstance* material;
        Box aabb;
    };

    Instance getInstance(Entity e) const {
        auto it = mEntityToInstance.find(e);
        return it == mEntityToInstance.end() ? kInvalidInstance : it->second;
    }

    const Renderable& get(Instance i) const { return mRenderables[i]; }
    size_t count() const { return mRenderables.size(); }

private:
    Instance addRenderable(Entity e, backend::VertexBufferHandle vb, backend::IndexBufferHandle ib,
                            backend::PrimitiveType type, uint32_t indexCount,
                            MaterialInstance* mat, const Box& aabb) {
        Instance i = static_cast<Instance>(mRenderables.size());
        mRenderables.push_back({e, vb, ib, type, indexCount, mat, aabb});
        mEntityToInstance[e] = i;
        return i;
    }

    std::vector<Renderable> mRenderables;
    std::unordered_map<Entity, Instance> mEntityToInstance;
};

} // namespace rel

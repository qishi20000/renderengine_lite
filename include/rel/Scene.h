#pragma once
#include <vector>
#include "rel/rel_fwd.h"
#include "rel/Entity.h"

namespace rel {

// A Scene is deliberately dumb: just the set of entities to consider for
// rendering, plus (optionally, later) a coarse culling structure. All real
// per-entity data lives in Engine's TransformManager/RenderableManager —
// see ARCHITECTURE.md section 3.
class Scene {
public:
    void addEntity(Entity e) { mEntities.push_back(e); }
    void removeEntity(Entity e);
    const std::vector<Entity>& getEntities() const { return mEntities; }

private:
    friend class Engine;
    Scene() = default;
    std::vector<Entity> mEntities;
};

} // namespace rel

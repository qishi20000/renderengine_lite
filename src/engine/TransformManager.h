#pragma once
#include <vector>
#include <unordered_map>

#include "rel/Entity.h"
#include "utils/Math.h"

namespace rel {

// Sparse Entity -> local/world transform storage with a flat parent-index
// hierarchy (no need for a generic scene-graph tree structure: AVM scenes
// have at most a few dozen transform nodes — bowl mesh, per-camera quads,
// car model parts, UI anchors).
class TransformManager {
public:
    using Instance = uint32_t;
    static constexpr Instance kInvalidInstance = static_cast<Instance>(-1);

    Instance create(Entity e, Instance parent, const mat4& local) {
        Instance i = static_cast<Instance>(mNodes.size());
        mNodes.push_back({e, parent, local, mat4::identity()});
        mEntityToInstance[e] = i;
        updateWorld(i);
        return i;
    }

    Instance getInstance(Entity e) const {
        auto it = mEntityToInstance.find(e);
        return it == mEntityToInstance.end() ? kInvalidInstance : it->second;
    }

    void setTransform(Instance i, const mat4& local) {
        mNodes[i].local = local;
        updateWorld(i);
        // NOTE: children are not automatically re-propagated here yet;
        // for AVM's shallow, mostly-static hierarchy this is called rarely
        // (e.g. car model root pose), so a full dirty-propagation pass can
        // be added later (see TODO in Renderer::render) without a redesign.
    }

    const mat4& getWorldTransform(Instance i) const { return mNodes[i].world; }

private:
    struct Node {
        Entity entity;
        Instance parent;
        mat4 local;
        mat4 world;
    };

    void updateWorld(Instance i) {
        Node& n = mNodes[i];
        n.world = (n.parent == kInvalidInstance) ? n.local : mNodes[n.parent].world * n.local;
    }

    std::vector<Node> mNodes;
    std::unordered_map<Entity, Instance> mEntityToInstance;
};

} // namespace rel

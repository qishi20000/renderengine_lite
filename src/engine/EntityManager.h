#pragma once
#include <vector>
#include <cstdint>

#include "rel/Entity.h"

// See ARCHITECTURE.md section 3: intentionally not a full ECS. Entities are
// just generation-checked indices; per-aspect data (transform, renderable,
// camera) lives in separate sparse Manager classes below, each mapping
// Entity -> its own dense "Instance" index.

namespace rel {

class EntityManager {
public:
    Entity create() {
        if (!mFreeList.empty()) {
            uint32_t idx = mFreeList.back();
            mFreeList.pop_back();
            return Entity{packId(idx, mGenerations[idx])};
        }
        uint32_t idx = static_cast<uint32_t>(mGenerations.size());
        mGenerations.push_back(1);
        return Entity{packId(idx, mGenerations[idx])};
    }

    void destroy(Entity e) {
        uint32_t idx = indexOf(e);
        if (idx >= mGenerations.size()) return;
        mGenerations[idx]++;
        mFreeList.push_back(idx);
    }

    bool isAlive(Entity e) const {
        uint32_t idx = indexOf(e);
        return idx < mGenerations.size() && mGenerations[idx] == generationOf(e);
    }

private:
    // Pack {index: 24 bits, generation: 8 bits} into Entity::id.
    static uint32_t packId(uint32_t index, uint8_t generation) {
        return (index & 0x00FFFFFFu) | (static_cast<uint32_t>(generation) << 24);
    }
    static uint32_t indexOf(Entity e) { return e.id & 0x00FFFFFFu; }
    static uint8_t generationOf(Entity e) { return static_cast<uint8_t>(e.id >> 24); }

    std::vector<uint8_t> mGenerations;
    std::vector<uint32_t> mFreeList;
};

} // namespace rel

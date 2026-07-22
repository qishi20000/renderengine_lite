#include "rel/Scene.h"

#include <algorithm>

namespace rel {

void Scene::removeEntity(Entity e) {
    mEntities.erase(std::remove(mEntities.begin(), mEntities.end(), e), mEntities.end());
}

} // namespace rel

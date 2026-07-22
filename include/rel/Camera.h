#pragma once
#include "rel/rel_fwd.h"
#include "rel/Entity.h"

namespace rel {

// AVM cameras are almost always orthographic-top-down or a fixed virtual
// perspective over the bowl mesh — no need for a generic multi-projection
// camera rig, but both projections are exposed since car-model preview
// shots typically want perspective.
class Camera {
public:
    void setProjectionPerspective(float fovYDegrees, float aspect, float near, float far);
    void setProjectionOrtho(float left, float right, float bottom, float top, float near, float far);
    void lookAt(float eyeX, float eyeY, float eyeZ,
                float centerX, float centerY, float centerZ,
                float upX, float upY, float upZ);

    Entity getEntity() const { return mEntity; }

private:
    friend class Engine;
    explicit Camera(Entity e) : mEntity(e) {}
    Entity mEntity;
    struct Impl;
    Impl* mImpl = nullptr;
};

} // namespace rel

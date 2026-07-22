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
    ~Camera();

    void setProjectionPerspective(float fovYDegrees, float aspect, float near, float far);
    void setProjectionOrtho(float left, float right, float bottom, float top, float near, float far);
    void lookAt(float eyeX, float eyeY, float eyeZ,
                float centerX, float centerY, float centerZ,
                float upX, float upY, float upZ);

    Entity getEntity() const { return mEntity; }

    // Internal use only (renderer/, engine/): Impl is an incomplete type
    // here and only fully defined in src/engine/ResourceImpls.h, so callers
    // outside the engine implementation cannot dereference it.
    struct Impl;
    Impl* getImpl() const { return mImpl; }

private:
    friend class Engine;
    explicit Camera(Entity e) : mEntity(e) {}
    Entity mEntity;
    Impl* mImpl = nullptr;
};

} // namespace rel

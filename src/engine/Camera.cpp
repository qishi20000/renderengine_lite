#include "rel/Camera.h"

#include "engine/ResourceImpls.h"

namespace rel {

namespace {
constexpr float kPi = 3.14159265358979323846f;
}

Camera::~Camera() { delete mImpl; }

void Camera::setProjectionPerspective(float fovYDegrees, float aspect, float near, float far) {
    mImpl->projection = perspective(fovYDegrees * kPi / 180.0f, aspect, near, far);
}

void Camera::setProjectionOrtho(float left, float right, float bottom, float top, float near, float far) {
    mImpl->projection = ortho(left, right, bottom, top, near, far);
}

void Camera::lookAt(float eyeX, float eyeY, float eyeZ,
                     float centerX, float centerY, float centerZ,
                     float upX, float upY, float upZ) {
    mImpl->view = rel::lookAt({eyeX, eyeY, eyeZ}, {centerX, centerY, centerZ}, {upX, upY, upZ});
}

} // namespace rel

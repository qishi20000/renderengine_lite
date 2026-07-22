#pragma once
#include <cstdint>
#include <vector>
#include <string>

#include "utils/Math.h"

namespace rel::avm {

// Per-camera intrinsic (fisheye distortion) + extrinsic (pose relative to
// the vehicle body frame) calibration, typically produced offline by a
// factory calibration tool and loaded at startup (JSON/binary blob — format
// intentionally left open here, see loadFromFile()).
struct CameraCalibration {
    std::string cameraName;     // e.g. "front", "rear", "left", "right"
    float intrinsics[9] = {};   // 3x3 camera matrix
    float distortion[4] = {};   // fisheye (equidistant) distortion coeffs k1..k4
    mat4 extrinsics;            // camera-from-vehicle transform
    uint32_t imageWidth = 0;
    uint32_t imageHeight = 0;
};

class FisheyeCalibration {
public:
    bool loadFromFile(const std::string& path);
    const std::vector<CameraCalibration>& getCalibrations() const { return mCalibrations; }

    // Projects a world/vehicle-space point into a given camera's normalized
    // UV space, applying the fisheye distortion model. Used offline by
    // BowlMeshGenerator to bake per-vertex UVs, not per-frame at runtime.
    bool projectToCameraUV(uint32_t cameraIndex, const vec3& vehicleSpacePoint, float& outU, float& outV) const;

private:
    std::vector<CameraCalibration> mCalibrations;
};

} // namespace rel::avm

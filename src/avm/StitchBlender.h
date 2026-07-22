#pragma once
#include <cstdint>
#include "utils/Math.h"

namespace rel::avm {

// Computes per-vertex blend weights for overlapping camera regions. Baked
// into the bowl mesh by BowlMeshGenerator (see materials/surround_view.*),
// so at runtime this is pure data, not a shader computation.
class StitchBlender {
public:
    enum class Strategy { LinearAlpha, DistanceWeighted };

    explicit StitchBlender(Strategy strategy = Strategy::DistanceWeighted) : mStrategy(strategy) {}

    // Returns the blend weight in [0, 1] for `cameraIndex` at a given
    // vehicle-space point, given the set of cameras covering that point.
    float computeWeight(uint32_t cameraIndex, const vec3& vehicleSpacePoint,
                         const uint32_t* overlappingCameras, uint32_t overlapCount) const;

private:
    Strategy mStrategy;
};

} // namespace rel::avm

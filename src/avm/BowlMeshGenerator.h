#pragma once
#include <cstdint>
#include <vector>

#include "avm/FisheyeCalibration.h"
#include "avm/StitchBlender.h"

namespace rel { class Engine; class VertexBuffer; class IndexBuffer; }

namespace rel::avm {

// Generates the bowl-shaped (or flat, for simple top-down mode) ground mesh
// used by SurroundViewPass, baking per-vertex camera UVs + blend weights so
// the fragment shader stays branch-free (see materials/surround_view.frag).
//
// This runs once at startup (and again if calibration is refreshed live,
// e.g. after a service recalibration) — not part of the per-frame render
// path, so it's fine for this to be relatively CPU-heavy / run on a Job
// thread rather than the render thread.
class BowlMeshGenerator {
public:
    struct Params {
        float groundRadius = 3.0f;   // meters, flat center region
        float wallRadius = 8.0f;     // meters, bowl wall outer radius
        float wallHeight = 2.5f;     // meters
        uint32_t radialSegments = 64;
        uint32_t ringSegments = 24;
    };

    struct GeneratedMesh {
        VertexBuffer* vertexBuffer = nullptr;
        IndexBuffer* indexBuffer = nullptr;
        uint32_t indexCount = 0;
    };

    GeneratedMesh generate(Engine& engine, const Params& params,
                            const FisheyeCalibration& calibration,
                            const StitchBlender& blender);
};

} // namespace rel::avm

#pragma once
// Forward declarations for the public RenderEngineLite (REL) API.
// Keep this header free of any OpenGL ES / Vulkan specific types so that
// consumers never need to know which backend is active.

#include <cstdint>

namespace rel {

class Engine;
class Scene;
class View;
class Camera;
class Renderer;
class SwapChain;
class Material;
class MaterialInstance;
class VertexBuffer;
class IndexBuffer;
class Texture;
class RenderTarget;

struct Entity;

} // namespace rel

namespace rel::backend {

class DriverApi;
class Platform;

// Opaque, API-agnostic GPU resource handles. See backend/Handles.h for the
// full definition. Forward-declared here so public headers can reference
// them without pulling in backend internals.
struct HandleBase;
template<typename T> struct Handle;

} // namespace rel::backend

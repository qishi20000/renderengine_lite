#pragma once

namespace rel { class View; class Engine; }
namespace rel::backend { class DriverApi; }

namespace rel::renderer {

// Explicit, hand-ordered pass interface — the lightweight replacement for a
// FrameGraph (see ARCHITECTURE.md section 4). Each pass is a self-contained
// unit so that, if a real FrameGraph is ever justified later, migrating one
// pass at a time (execute() body barely changes, only resource declaration
// moves to a graph builder) is straightforward.
class RenderPass {
public:
    virtual ~RenderPass() = default;

    // One-time / lazy resource creation (FBOs, programs). Safe to call
    // every frame; implementations should no-op after first successful
    // setup themselves.
    virtual void setup(Engine& engine, View& view) {}

    // NOTE: Renderer::render() wraps the *entire* pass chain in a single
    // driver.beginRenderPass()/endRenderPass() pair — individual passes
    // must NOT begin/end their own render pass. See ARCHITECTURE.md
    // section 7 for why (Adreno tile bandwidth).
    virtual void execute(Engine& engine, View& view, backend::DriverApi& driver) = 0;

protected:
    // Shared "draw every Renderable in view's Scene with its bound
    // MaterialInstance" loop, used by all three built-in passes today.
    // Once passes need to filter by layer/tag (e.g. CarModelPass should
    // only draw car-model entities, not the bowl mesh), add a filter
    // predicate parameter here rather than duplicating the loop — see
    // ARCHITECTURE.md section 4.
    static void drawAllRenderables(Engine& engine, View& view, backend::DriverApi& driver);
};

} // namespace rel::renderer

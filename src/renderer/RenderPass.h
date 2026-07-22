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

    virtual void execute(Engine& engine, View& view, backend::DriverApi& driver) = 0;
};

} // namespace rel::renderer

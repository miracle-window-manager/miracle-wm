/**
Copyright (C) 2024  Matthew Kosarek

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef MIR_RENDERER_GL_RENDERER_H_
#define MIR_RENDERER_GL_RENDERER_H_

#include "primitive.h"
#include "program_factory.h"
#include "surface_tracker.h"

#include <GLES2/gl2.h>
#include <mir/geometry/rectangle.h>
#include <mir/graphics/buffer_id.h>
#include <mir/graphics/renderable.h>
#include <mir/renderer/renderer.h>
#include <miral/window_manager_tools.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mir
{
namespace graphics
{
    class GLRenderingProvider;
}
namespace graphics::gl
{
    class OutputSurface;
}
}

namespace miracle
{
class MiracleConfig;
class CompositorState;
class WindowToolsAccessor;

class Renderer : public mir::renderer::Renderer
{
public:
    Renderer(std::shared_ptr<mir::graphics::GLRenderingProvider> gl_interface,
        std::unique_ptr<mir::graphics::gl::OutputSurface> output,
        std::shared_ptr<MiracleConfig> const& config,
        SurfaceTracker& surface_tracker,
        CompositorState const& compositor_state,
        std::shared_ptr<WindowToolsAccessor> const& accessor);
    ~Renderer() override = default;

    // These are called with a valid GL context:
    void set_viewport(mir::geometry::Rectangle const& rect) override;
    void set_output_transform(glm::mat2 const&) override;
    auto render(mir::graphics::RenderableList const&) const -> std::unique_ptr<mir::graphics::Framebuffer> override;

    // This is called _without_ a GL context:
    void suspend() override;

private:
    /**
     * tessellate defines the list of triangles that will be used to render
     * the surface. By default it just returns 4 vertices for a rectangle.
     * However you can override its behaviour to tessellate more finely and
     * deform freely for effects like wobbly windows.
     *
     * \param [in,out] primitives The list of rendering primitives to be
     *                            grown and/or modified.
     * \param [in]     renderable The renderable surface being tessellated.
     *
     * \note The cohesion of this function to gl::Renderer is quite loose and it
     *       does not strictly need to reside here.
     *       However it seems a good choice under gl::Renderer while this remains
     *       the only OpenGL-specific class in the display server, and
     *       tessellation is very much OpenGL-specific.
     */
    static void tessellate(std::vector<mir::gl::Primitive>& primitives,
        mir::graphics::Renderable const& renderable);

    struct DrawData
    {
        bool enabled = false;
        bool needs_outline = false;
        glm::mat4 workspace_transform = glm::mat4(1.f);
        bool is_focused = false;

        struct
        {
            bool enabled = false;
            glm::vec4 color;
            int size;
        } outline_context;
    };

    DrawData get_draw_data(mir::graphics::Renderable const&) const;
    /// Draws the current renderable and returns a follow-up draw if required.
    DrawData draw(mir::graphics::Renderable const& renderable, DrawData const& data) const;
    void update_gl_viewport();

    std::unique_ptr<mir::graphics::gl::OutputSurface> const output_surface;
    GLfloat clear_color[4];
    bool has_stencil_support = false;
    mutable long long frameno = 0;
    std::unique_ptr<ProgramFactory> const program_factory;
    mir::geometry::Rectangle viewport;
    glm::mat4 screen_to_gl_coords;
    glm::mat4 display_transform;
    std::vector<mir::gl::Primitive> mutable primitives;
    std::shared_ptr<mir::graphics::GLRenderingProvider> const gl_interface;
    std::shared_ptr<MiracleConfig> config;
    SurfaceTracker& surface_tracker;
    CompositorState const& compositor_state;
    std::shared_ptr<WindowToolsAccessor> const& accessor;
};

}

#endif // MIR_RENDERER_GL_RENDERER_H_

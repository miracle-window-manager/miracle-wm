/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRACLE_WM_STUB_SURFACE_H
#define MIRACLE_WM_STUB_SURFACE_H

#include <mir/scene/surface.h>

namespace miracle::test
{
class StubSurface : public mir::scene::Surface
{
public:
    void initial_placement_done() override {}
    
    auto content_offset() const -> mir::geometry::Displacement override
    {
        return mir::geometry::Displacement();
    }

    auto primary_buffer_stream() const -> std::shared_ptr<mir::frontend::BufferStream> override
    {
        return std::shared_ptr<mir::frontend::BufferStream>();
    }

    auto wayland_surface() -> mir::wayland::Weak<mir::frontend::WlSurface> const& override
    {
        throw std::logic_error("Unimplemented");
    }

    bool input_area_contains(mir::geometry::Point const& point) const override
    {
        return false;
    }

    mir::input::InputReceptionMode reception_mode() const override
    {
        return mir::input::InputReceptionMode::normal;
    }

    void consume(std::shared_ptr<MirEvent const> const& event) override
    {
    }

    auto visible_on_lock_screen() const -> bool override
    {
        return false;
    }

    void register_interest(std::weak_ptr<mir::scene::SurfaceObserver> const& observer) override
    {
    }

    void register_interest(std::weak_ptr<mir::scene::SurfaceObserver> const& observer, mir::Executor& executor) override
    {
    }

    void register_early_observer(std::weak_ptr<mir::scene::SurfaceObserver> const& observer,
        mir::Executor& executor) override
    {
    }

    void unregister_interest(mir::scene::SurfaceObserver const& observer) override
    {
    }

    std::string name() const override
    {
        return "";
    }

    mir::geometry::Size content_size() const override
    {
        return {};
    }

    mir::geometry::Rectangle input_bounds() const override
    {
        return {};
    }

    mir::geometry::Point top_left() const override
    {
        return mir::geometry::Point();
    }

    mir::geometry::Size window_size() const override
    {
        return mir::geometry::Size();
    }

    mir::graphics::RenderableList generate_renderables(mir::compositor::CompositorID id) const override
    {
        return mir::graphics::RenderableList();
    }

    MirWindowType type() const override
    {
        return mir_window_type_utility;
    }

    MirWindowState state() const override
    {
        return mir_window_state_attached;
    }

    auto state_tracker() const -> mir::scene::SurfaceStateTracker override
    {
        return mir::scene::SurfaceStateTracker(mir_window_state_attached);
    }

    void hide() override
    {
    }

    void show() override
    {
    }

    bool visible() const override
    {
        return false;
    }

    void move_to(mir::geometry::Point const& top_left) override
    {
    }

    void set_input_region(std::vector<mir::geometry::Rectangle> const& region) override
    {
    }

    std::vector<mir::geometry::Rectangle> get_input_region() const override
    {
        return std::vector<mir::geometry::Rectangle>();
    }

    void resize(mir::geometry::Size const& window_size) override
    {
    }

    void set_transformation(glm::mat4 const& t) override
    {
    }

    void set_alpha(float alpha) override
    {
    }

    void set_orientation(MirOrientation orientation) override
    {
    }

    void set_cursor_image(std::shared_ptr<mir::graphics::CursorImage> const& image) override
    {
    }

    std::shared_ptr<mir::graphics::CursorImage> cursor_image() const override
    {
        return std::shared_ptr<mir::graphics::CursorImage>();
    }

    void set_reception_mode(mir::input::InputReceptionMode mode) override
    {
    }

    void request_client_surface_close() override
    {
    }

    std::shared_ptr<Surface> parent() const override
    {
        return std::shared_ptr<Surface>();
    }

    int configure(MirWindowAttrib attrib, int value) override
    {
        return 0;
    }

    int query(MirWindowAttrib attrib) const override
    {
        return 0;
    }

    void rename(std::string const& title) override
    {
    }

    void set_streams(std::list<mir::scene::StreamInfo> const& streams) override
    {
    }

    void set_confine_pointer_state(MirPointerConfinementState state) override
    {
    }

    MirPointerConfinementState confine_pointer_state() const override
    {
        return mir_pointer_unconfined;
    }

    void placed_relative(mir::geometry::Rectangle const& placement) override
    {
    }

    auto depth_layer() const -> MirDepthLayer override
    {
        return mir_depth_layer_overlay;
    }

    void set_depth_layer(MirDepthLayer depth_layer) override
    {
    }

    void set_visible_on_lock_screen(bool visible) override
    {
    }

    std::optional<mir::geometry::Rectangle> clip_area() const override
    {
        return std::optional<mir::geometry::Rectangle>();
    }

    void set_clip_area(std::optional<mir::geometry::Rectangle> const& area) override
    {
    }

    auto focus_state() const -> MirWindowFocusState override
    {
        return mir_window_focus_state_unfocused;
    }

    void set_focus_state(MirWindowFocusState focus_state) override
    {
    }

    auto application_id() const -> std::string override
    {
        return std::string();
    }

    void set_application_id(std::string const& application_id) override
    {
    }

    auto session() const -> std::weak_ptr<mir::scene::Session> override
    {
        return std::weak_ptr<mir::scene::Session>();
    }

    void set_window_margins(mir::geometry::DeltaY top, mir::geometry::DeltaX left, mir::geometry::DeltaY bottom,
        mir::geometry::DeltaX right) override
    {
    }

    auto focus_mode() const -> MirFocusMode override
    {
        return mir_focus_mode_disabled;
    }

    void set_focus_mode(MirFocusMode focus_mode) override
    {
    }
};
}

#endif // MIRACLE_WM_STUB_SURFACE_H

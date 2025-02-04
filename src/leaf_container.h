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

#ifndef MIRACLEWM_LEAF_NODE_H
#define MIRACLEWM_LEAF_NODE_H

#include "container.h"
#include "layout_scheme.h"
#include "scratchpad_state.h"
#include "window_controller.h"

#include <miral/window.h>
#include <miral/window_manager_tools.h>
#include <optional>

namespace geom = mir::geometry;

namespace miracle
{

class Config;
class CompositorState;
class RenderDataManager;
class OutputManager;

/// A [LeafContainer] always contains a single window.
class LeafContainer : public Container
{
public:
    LeafContainer(
        Workspace* workspace,
        WindowController& node_interface,
        geom::Rectangle area,
        std::shared_ptr<Config> const& config,
        std::shared_ptr<ParentContainer> const& parent,
        CompositorState const& state,
        OutputManager* output_manager);
    ~LeafContainer();

    void associate_to_window(miral::Window const&);
    [[nodiscard]] geom::Rectangle get_logical_area() const override;
    [[nodiscard]] geom::Rectangle get_visible_area() const override;
    void set_logical_area(geom::Rectangle const& target_rect, bool with_animations = true) override;
    std::weak_ptr<ParentContainer> get_parent() const override;
    void set_parent(std::shared_ptr<ParentContainer> const&) override;
    void set_state(MirWindowState state);
    bool is_fullscreen() const override;
    void constrain() override;
    size_t get_min_width() const override;
    size_t get_min_height() const override;
    void handle_ready() override;
    void handle_modify(miral::WindowSpecification const&) override;
    void handle_raise() override;
    bool resize(Direction direction, int pixels) override;
    bool set_size(std::optional<int> const& width, std::optional<int> const& height) override;
    bool toggle_fullscreen() override;
    mir::geometry::Rectangle confirm_placement(
        MirWindowState, mir::geometry::Rectangle const&) override;
    void on_open() override;
    void on_focus_gained() override;
    void on_focus_lost() override;
    void on_move_to(geom::Point const&) override;
    void handle_request_move(MirInputEvent const* input_event) override;
    void handle_request_resize(MirInputEvent const* input_event, MirResizeEdge edge) override;
    void request_horizontal_layout() override;
    void request_vertical_layout() override;
    void toggle_layout(bool cycle_thru_all) override;
    [[nodiscard]] std::optional<miral::Window> window() const override { return window_; }
    void commit_changes() override;
    void show() override;
    void hide() override;
    Workspace* get_workspace() const override;
    void set_workspace(Workspace*) override;
    Output* get_output() const override;
    glm::mat4 get_transform() const override;
    void set_transform(glm::mat4 transform) override;
    uint32_t animation_handle() const override;
    void animation_handle(uint32_t uint_32) override;
    bool is_focused() const override;
    ContainerType get_type() const override;
    bool select_next(Direction) override;
    bool pinned() const override;
    bool pinned(bool) override;
    bool move(Direction) override;
    bool move_by(Direction, int) override;
    bool move_by(float, float) override;
    bool move_to(Container& other) override;
    bool move_to(int, int) override;
    bool toggle_tabbing() override;
    bool toggle_stacking() override;
    bool drag_start() override;
    void drag(int x, int y) override;
    bool drag_stop() override;
    bool set_layout(LayoutScheme) override;
    bool anchored() const override;
    ScratchpadState scratchpad_state() const override;
    void scratchpad_state(ScratchpadState) override;
    LayoutScheme get_layout() const override;
    nlohmann::json to_json() const override;

    static std::shared_ptr<LeafContainer> handle_select(
        Container& from,
        Direction direction);

private:
    Workspace* workspace;
    WindowController& window_controller;
    geom::Rectangle logical_area;
    std::optional<geom::Rectangle> next_logical_area;
    bool next_with_animations = true;
    std::shared_ptr<Config> config;
    miral::Window window_;
    std::weak_ptr<ParentContainer> parent;
    CompositorState const& state;
    OutputManager* output_manager;

    std::optional<MirWindowState> before_shown_state;
    std::optional<MirWindowState> next_state;
    std::optional<MirDepthLayer> next_depth_layer;
    glm::mat4 transform = glm::mat4(1.f);
    uint32_t animation_handle_ = 0;
    bool is_dragging_ = false;
    geom::Point dragged_position;

    static void handle_resize(Container* container, Direction direction, int amount);
    static void handle_layout_scheme(Container* container, LayoutScheme scheme);
};

} // miracle

#endif // MIRACLEWM_LEAF_NODE_H

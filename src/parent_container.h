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

#ifndef MIRACLEWM_PARENT_NODE_H
#define MIRACLEWM_PARENT_NODE_H

#include "container.h"
#include "layout_scheme.h"
#include "window_controller.h"
#include <mir/geometry/rectangle.h>

namespace geom = mir::geometry;

namespace miracle
{

class LeafContainer;
class MiracleConfig;
class TilingWindowTree;
class CompositorState;

/// A parent container defines the layout of containers beneath it.
/// The container
class ParentContainer : public Container
{
public:
    ParentContainer(WindowController&,
        geom::Rectangle,
        std::shared_ptr<MiracleConfig> const&,
        TilingWindowTree* tree,
        std::shared_ptr<ParentContainer> const& parent,
        CompositorState const& state);
    geom::Rectangle get_logical_area() const override;
    geom::Rectangle get_visible_area() const override;
    size_t num_nodes() const;
    std::shared_ptr<LeafContainer> create_space_for_window(int index = -1);
    std::shared_ptr<LeafContainer> confirm_window(miral::Window const&);
    void graft_existing(std::shared_ptr<Container> const& node, int index);
    std::shared_ptr<ParentContainer> convert_to_parent(std::shared_ptr<Container> const& container);
    void set_logical_area(geom::Rectangle const& target_rect) override;
    void swap_nodes(std::shared_ptr<Container> const& first, std::shared_ptr<Container> const& second);
    void remove(std::shared_ptr<Container> const& node);
    void commit_changes() override;
    std::shared_ptr<Container> at(size_t i) const;
    std::shared_ptr<LeafContainer> get_nth_window(size_t i) const;
    std::shared_ptr<Container> find_where(std::function<bool(std::shared_ptr<Container> const&)> func) const;
    LayoutScheme get_direction() { return scheme; }
    std::vector<std::shared_ptr<Container>> const& get_sub_nodes() const;
    [[nodiscard]] int get_index_of_node(Container const* node) const;
    [[nodiscard]] int get_index_of_node(std::shared_ptr<Container> const& node) const;
    [[nodiscard]] int get_index_of_node(Container const&) const;
    void constrain() override;
    size_t get_min_width() const override;
    size_t get_min_height() const override;
    std::weak_ptr<ParentContainer> get_parent() const override;
    void set_parent(std::shared_ptr<ParentContainer> const&) override;
    void handle_ready() override;
    void handle_modify(miral::WindowSpecification const& specification) override;
    void handle_request_move(MirInputEvent const* input_event) override;
    void handle_request_resize(MirInputEvent const* input_event, MirResizeEdge edge) override;
    void handle_raise() override;
    bool resize(Direction direction) override;
    bool toggle_fullscreen() override;
    void request_horizontal_layout() override;
    void request_vertical_layout() override;
    void toggle_layout(bool cycle_thru_all) override;
    void set_tree(TilingWindowTree*);
    void on_focus_gained() override;
    void on_focus_lost() override;
    void on_move_to(mir::geometry::Point const& top_left) override;
    mir::geometry::Rectangle
    confirm_placement(MirWindowState state, mir::geometry::Rectangle const& rectangle) override;
    ContainerType get_type() const override;
    void show() override;
    void hide() override;
    void on_open() override;
    Workspace* get_workspace() const override;
    Output* get_output() const override;
    glm::mat4 get_transform() const override;
    void set_transform(glm::mat4 transform) override;
    glm::mat4 get_workspace_transform() const override;
    glm::mat4 get_output_transform() const override;
    uint32_t animation_handle() const override;
    void animation_handle(uint32_t uint_32) override;
    bool is_focused() const override;
    std::optional<miral::Window> window() const override;
    bool select_next(Direction) override;
    bool pinned(bool) override;
    bool pinned() const override;
    bool move(Direction direction) override;
    bool move_by(Direction direction, int pixels) override;
    bool move_to(int x, int y) override;
    bool is_fullscreen() const override;
    bool toggle_tabbing() override;
    bool toggle_stacking() override;
    bool set_layout(LayoutScheme scheme) override;
    LayoutScheme get_layout() const override;
    nlohmann::json to_json() const override;
    [[nodiscard]] LayoutScheme get_scheme() const { return scheme; }

private:
    WindowController& node_interface;
    geom::Rectangle logical_area;
    TilingWindowTree* tree;
    std::shared_ptr<MiracleConfig> config;
    std::weak_ptr<ParentContainer> parent;
    CompositorState const& state;

    LayoutScheme scheme = LayoutScheme::horizontal;
    std::vector<std::shared_ptr<Container>> sub_nodes;
    std::shared_ptr<LeafContainer> pending_node;

    geom::Rectangle create_space(int pending_index);
    void relayout();
};

} // miracle

#endif // MIRACLEWM_PARENT_NODE_H

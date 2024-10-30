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

#ifndef MIRACLE_TREE_H
#define MIRACLE_TREE_H

#include "container.h"
#include "direction.h"
#include "layout_scheme.h"
#include <memory>
#include <mir/geometry/rectangle.h>
#include <miral/window.h>
#include <miral/window_manager_tools.h>
#include <miral/window_specification.h>
#include <miral/zone.h>
#include <vector>

namespace geom = mir::geometry;

namespace miracle
{

class CompositorState;
class MiracleConfig;
class WindowController;
class LeafContainer;
class Workspace;

class TilingWindowTreeInterface
{
public:
    virtual std::vector<miral::Zone> const& get_zones() = 0;
    virtual Workspace* get_workspace() const = 0;
};

class TilingWindowTree
{
public:
    TilingWindowTree(
        std::unique_ptr<TilingWindowTreeInterface> tree_interface,
        WindowController&,
        CompositorState const&,
        std::shared_ptr<MiracleConfig> const& options,
        geom::Rectangle const& area);
    ~TilingWindowTree();

    /// Place a window in the specified container if one is provided.
    /// Otherwise, the container is placed at the root node.
    miral::WindowSpecification place_new_window(
        const miral::WindowSpecification& requested_specification,
        std::shared_ptr<ParentContainer> const& container);

    std::shared_ptr<LeafContainer> confirm_window(
        miral::WindowInfo const&,
        std::shared_ptr<ParentContainer> const& container);

    void graft(std::shared_ptr<ParentContainer> const&);
    void graft(std::shared_ptr<LeafContainer> const&);

    /// Try to resize the current active window in the provided direction
    bool resize_container(Direction direction, Container&);

    /// Move the active window in the provided direction
    bool move_container(Direction direction, Container&);

    /// Select the next window in the provided direction
    bool select_next(Direction direction, Container&);

    /// Toggle the active window between fullscreen and not fullscreen
    bool toggle_fullscreen(LeafContainer&);

    bool has_fullscreen_window() const { return is_active_window_fullscreen; }

    void request_layout(Container&, LayoutScheme);

    /// Request a change to vertical window placement
    void request_vertical_layout(Container&);

    /// Request a change to horizontal window placement
    void request_horizontal_layout(Container&);

    /// Request that the provided container become tabbed.
    void request_tabbing_layout(Container&);

    /// Request that the provided container become stacked.
    void request_stacking_layout(Container&);

    // Request a change from the current layout scheme to another layout scheme
    void toggle_layout(Container&, bool cycle_thru_all);

    /// Advises us to focus the provided container.
    void advise_focus_gained(LeafContainer&);

    /// Called when the container was deleted.
    void advise_delete_window(std::shared_ptr<Container> const&);

    /// Called when the physical display is resized.
    void set_area(geom::Rectangle const& new_area);

    geom::Rectangle get_area() const;

    std::shared_ptr<LeafContainer> select_window_from_point(int x, int y);

    bool advise_fullscreen_container(LeafContainer&);
    bool advise_restored_container(LeafContainer&);
    bool handle_container_ready(LeafContainer&);

    bool confirm_placement_on_display(
        Container& container,
        MirWindowState new_state,
        mir::geometry::Rectangle& new_placement);

    void foreach_node(std::function<void(std::shared_ptr<Container>)> const&);

    /// Shows the containers in this tree and returns a fullscreen container, if any
    std::shared_ptr<LeafContainer> show();

    /// Hides the containers in this tree
    void hide();

    void recalculate_root_node_area();
    bool is_empty();

    [[nodiscard]] Workspace* get_workspace() const;
    [[nodiscard]] std::shared_ptr<ParentContainer> const& get_root() const { return root_lane; }

private:
    struct MoveResult
    {
        enum
        {
            traversal_type_invalid,
            traversal_type_insert,
            traversal_type_prepend,
            traversal_type_append
        } traversal_type
            = traversal_type_invalid;
        std::shared_ptr<Container> node = nullptr;
    };

    WindowController& window_controller;
    CompositorState const& state;
    std::shared_ptr<MiracleConfig> config;
    std::shared_ptr<ParentContainer> root_lane;
    std::unique_ptr<TilingWindowTreeInterface> tree_interface;

    bool is_active_window_fullscreen = false;
    bool is_hidden = false;
    int config_handle = 0;

    void handle_layout_scheme(LayoutScheme direction, Container& container);
    void handle_resize(Container& node, Direction direction, int amount);

    /// Constrains the container to its tile in the tree
    bool constrain(Container&);

    /// Removes the node from the tree
    /// @returns The parent that will need to have its changes committed
    std::shared_ptr<ParentContainer> handle_remove(std::shared_ptr<Container> const& node);

    /// Transfer a node from its current parent to the parent of 'to'
    /// in a position right after 'to'.
    /// @returns The two parents who will need to have their changes committed
    std::tuple<std::shared_ptr<ParentContainer>, std::shared_ptr<ParentContainer>> transfer_node(
        std::shared_ptr<Container> const& node,
        std::shared_ptr<Container> const& to);

    /// From the provided node, find the next node in the provided direction.
    /// This method is guaranteed to return a Window node, not a Lane.
    MoveResult handle_move(Container& from, Direction direction);

    /// Selects the next node in the provided direction
    /// @returns The next selectable window or nullptr if none is found
    static std::shared_ptr<LeafContainer> handle_select(Container& from, Direction direction);

    std::shared_ptr<LeafContainer> active_container() const;
};

}

#endif // MIRACLE_TREE_H

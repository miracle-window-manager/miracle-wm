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
class Config;
class WindowController;
class LeafContainer;
class Workspace;
class OutputManager;

class TilingWindowTreeInterface
{
public:
    virtual std::vector<miral::Zone> const& get_zones() = 0;
    virtual Workspace* get_workspace() const = 0;
};

class TilingWindowTree
{
public:
    virtual ~TilingWindowTree() = default;

    /// Place a window in the specified container if one is provided.
    /// Otherwise, the container is placed at the root node.
    virtual miral::WindowSpecification place_new_window(
        const miral::WindowSpecification& requested_specification,
        std::shared_ptr<ParentContainer> const& container)
        = 0;

    virtual std::shared_ptr<LeafContainer> confirm_window(
        miral::WindowInfo const&,
        std::shared_ptr<ParentContainer> const& container)
        = 0;

    [[deprecated("Use move_to_tree instead")]]
    virtual void graft(std::shared_ptr<Container> const&, std::shared_ptr<ParentContainer> const& parent, int index = -1)
        = 0;

    /// Move the active window in the provided direction
    virtual bool move_container(Direction direction, Container&) = 0;

    /// Move [to_move] to the current position of [target]. [target] does
    /// not have to be in the tree.
    virtual bool move_to(Container& to_move, Container& target) = 0;

    /// Moves [to_move] to the first available position of the tree and handles
    /// the process of updating its old position in tree, if any.
    virtual bool move_to_tree(std::shared_ptr<Container> const& container) = 0;

    /// Called when the container was deleted.
    virtual void advise_delete_window(std::shared_ptr<Container> const&) = 0;

    /// Called when the physical display is resized.
    virtual void set_area(geom::Rectangle const& new_area) = 0;

    virtual geom::Rectangle get_area() const = 0;

    virtual void foreach_node(std::function<void(std::shared_ptr<Container> const&)> const&) const = 0;
    virtual bool foreach_node_pred(std::function<bool(std::shared_ptr<Container> const&)> const&) const = 0;

    /// Shows the containers in this tree and returns a fullscreen container, if any
    virtual std::shared_ptr<LeafContainer> show() = 0;

    /// Hides the containers in this tree
    virtual void hide() = 0;

    virtual void recalculate_root_node_area() = 0;
    virtual bool is_empty() const = 0;

    [[nodiscard]] virtual Workspace* get_workspace() const = 0;
    [[nodiscard]] virtual std::shared_ptr<ParentContainer> const& get_root() const = 0;
};

class MiralTilingWindowTree : public TilingWindowTree
{
public:
    MiralTilingWindowTree(
        std::unique_ptr<TilingWindowTreeInterface> tree_interface,
        WindowController&,
        CompositorState const&,
        OutputManager* output_manager,
        std::shared_ptr<Config> const& options,
        geom::Rectangle const& area);
    ~MiralTilingWindowTree();

    miral::WindowSpecification place_new_window(
        const miral::WindowSpecification& requested_specification,
        std::shared_ptr<ParentContainer> const& container) override;
    std::shared_ptr<LeafContainer> confirm_window(
        miral::WindowInfo const&,
        std::shared_ptr<ParentContainer> const& container) override;
    void graft(std::shared_ptr<Container> const&, std::shared_ptr<ParentContainer> const& parent, int index = -1) override;
    bool move_container(Direction direction, Container&) override;
    bool move_to(Container& to_move, Container& target) override;
    bool move_to_tree(std::shared_ptr<Container> const& container) override;
    void advise_delete_window(std::shared_ptr<Container> const&) override;
    void set_area(geom::Rectangle const& new_area) override;
    geom::Rectangle get_area() const override;
    void foreach_node(std::function<void(std::shared_ptr<Container> const&)> const&) const override;
    bool foreach_node_pred(std::function<bool(std::shared_ptr<Container> const&)> const&) const override;
    std::shared_ptr<LeafContainer> show() override;
    void hide() override;
    void recalculate_root_node_area() override;
    bool is_empty() const override;
    [[nodiscard]] Workspace* get_workspace() const override;
    [[nodiscard]] std::shared_ptr<ParentContainer> const& get_root() const override { return root_lane; }

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
    OutputManager* output_manager;
    std::shared_ptr<Config> config;
    std::shared_ptr<ParentContainer> root_lane;
    std::unique_ptr<TilingWindowTreeInterface> tree_interface;

    bool is_hidden = false;
    int config_handle = 0;

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
};

}

#endif // MIRACLE_TREE_H

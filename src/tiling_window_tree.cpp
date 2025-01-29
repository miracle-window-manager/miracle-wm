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

#define MIR_LOG_COMPONENT "window_tree"

#include "tiling_window_tree.h"
#include "compositor_state.h"
#include "config.h"
#include "leaf_container.h"
#include "output.h"
#include "parent_container.h"
#include "window_helpers.h"

#include <cmath>
#include <memory>
#include <mir/log.h>

using namespace miracle;

MiralTilingWindowTree::MiralTilingWindowTree(
    std::unique_ptr<TilingWindowTreeInterface> tree_interface,
    WindowController& window_controller,
    CompositorState const& state,
    OutputManager* output_manager,
    std::shared_ptr<Config> const& config,
    geom::Rectangle const& area) :
    root_lane { std::make_shared<ParentContainer>(
        window_controller,
        area,
        config,
        this,
        nullptr,
        state,
        output_manager) },
    config { config },
    window_controller { window_controller },
    state { state },
    output_manager { output_manager },
    tree_interface { std::move(tree_interface) }
{
    recalculate_root_node_area();
    config_handle = config->register_listener([&](auto&)
    {
        recalculate_root_node_area();
    });
}

MiralTilingWindowTree::~MiralTilingWindowTree()
{
    config->unregister_listener(config_handle);
}

miral::WindowSpecification MiralTilingWindowTree::place_new_window(
    const miral::WindowSpecification& requested_specification,
    std::shared_ptr<ParentContainer> const& parent_)
{
    auto parent = parent_ ? parent_ : root_lane;
    auto container = parent->create_space_for_window();
    auto rect = container->get_visible_area();

    miral::WindowSpecification new_spec = requested_specification;
    new_spec.server_side_decorated() = false;
    new_spec.min_width() = geom::Width { 0 };
    new_spec.max_width() = geom::Width { std::numeric_limits<int>::max() };
    new_spec.min_height() = geom::Height { 0 };
    new_spec.max_height() = geom::Height { std::numeric_limits<int>::max() };
    new_spec.size() = rect.size;
    new_spec.top_left() = rect.top_left;

    return new_spec;
}

std::shared_ptr<LeafContainer> MiralTilingWindowTree::confirm_window(
    miral::WindowInfo const& window_info,
    std::shared_ptr<ParentContainer> const& container)
{
    auto parent = container ? container : root_lane;
    return parent->confirm_window(window_info.window());
}

void MiralTilingWindowTree::graft(
    std::shared_ptr<Container> const& leaf,
    std::shared_ptr<ParentContainer> const& parent,
    int index)
{
    leaf->tree(this);
    parent->graft_existing(leaf, index == -1 ? (int)parent->num_nodes() : index);
    parent->commit_changes();
}

void MiralTilingWindowTree::set_area(geom::Rectangle const& new_area)
{
    root_lane->set_logical_area(new_area);
    root_lane->commit_changes();
}

geom::Rectangle MiralTilingWindowTree::get_area() const
{
    return root_lane->get_logical_area();
}

bool MiralTilingWindowTree::move_container(miracle::Direction direction, Container& container)
{
    auto traversal_result = handle_move(container, direction);
    switch (traversal_result.traversal_type)
    {
    case MoveResult::traversal_type_insert:
    {
        move_to(container, *traversal_result.node);
        break;
    }
    case MoveResult::traversal_type_append:
    {
        auto lane_node = Container::as_parent(traversal_result.node);
        auto moving_node = container.shared_from_this();
        handle_remove(moving_node);
        lane_node->graft_existing(moving_node, lane_node->num_nodes());
        lane_node->commit_changes();
        break;
    }
    case MoveResult::traversal_type_prepend:
    {
        auto lane_node = Container::as_parent(traversal_result.node);
        auto moving_node = container.shared_from_this();
        handle_remove(moving_node);
        lane_node->graft_existing(moving_node, 0);
        lane_node->commit_changes();
        break;
    }
    default:
    {
        mir::log_error("Unable to move window");
        return false;
    }
    }

    return true;
}

bool MiralTilingWindowTree::move_to(Container& to_move, Container& target)
{
    auto target_parent = target.get_parent().lock();
    if (!target_parent)
    {
        mir::log_warning("Unable to move active window: second_window has no second_parent");
        return false;
    }

    auto active_parent = Container::as_parent(to_move.get_parent().lock());
    if (active_parent == target_parent)
    {
        active_parent->swap_nodes(to_move.shared_from_this(), target.shared_from_this());
        active_parent->commit_changes();
        return true;
    }

    // Transfer the node to the new parent.
    auto [first, second] = transfer_node(to_move.shared_from_this(), target.shared_from_this());
    first->commit_changes();
    second->commit_changes();
    return true;
}

bool MiralTilingWindowTree::move_to_tree(std::shared_ptr<Container> const& container)
{
    // When we remove [node] from its initial position, there's a chance
    // that the target_lane was melted into another lane. Hence, we need to return it
    auto to_update = handle_remove(container);

    root_lane->graft_existing(container, root_lane->num_nodes());
    container->tree(root_lane->tree());
    to_update->commit_changes();
    root_lane->commit_changes();
    return true;
}

void MiralTilingWindowTree::advise_delete_window(std::shared_ptr<Container> const& container)
{
    auto parent = handle_remove(container);
    parent->commit_changes();
}

namespace
{
LayoutScheme from_direction(Direction direction)
{
    switch (direction)
    {
    case Direction::up:
    case Direction::down:
        return LayoutScheme::vertical;
    case Direction::right:
    case Direction::left:
        return LayoutScheme::horizontal;
    default:
        mir::log_error(
            "from_direction: somehow we are trying to create a LayoutScheme from an incorrect Direction");
        return LayoutScheme::horizontal;
    }
}
}

MiralTilingWindowTree::MoveResult MiralTilingWindowTree::handle_move(Container& from, Direction direction)
{
    // Algorithm:
    //  1. Perform the _select algorithm. If that passes, then we want to be where the selected node
    //     currently is
    //  2. If our parent layout direction does not equal the root layout direction, we can append
    //     or prepend to the root
    if (auto insert_node = LeafContainer::handle_select(from, direction))
    {
        return {
            MoveResult::traversal_type_insert,
            insert_node
        };
    }

    auto parent = from.get_parent().lock();
    if (root_lane == parent)
    {
        auto new_layout_direction = from_direction(direction);
        if (new_layout_direction == root_lane->get_direction())
            return {};

        auto after_root_lane = std::make_shared<ParentContainer>(
            window_controller,
            root_lane->get_logical_area(),
            config,
            this,
            nullptr,
            state,
            output_manager);
        after_root_lane->set_layout(new_layout_direction);
        after_root_lane->graft_existing(root_lane, 0);
        root_lane = after_root_lane;
        recalculate_root_node_area();
    }

    bool is_negative = is_negative_direction(direction);
    if (is_negative)
        return {
            MoveResult::traversal_type_prepend,
            root_lane
        };
    else
        return {
            MoveResult::traversal_type_append,
            root_lane
        };
}

std::shared_ptr<ParentContainer> MiralTilingWindowTree::handle_remove(std::shared_ptr<Container> const& node)
{
    auto parent = Container::as_parent(node->get_parent().lock());
    if (parent == nullptr)
        return nullptr;

    if (parent->num_nodes() == 1 && parent->get_parent().lock())
    {
        // Remove the entire lane if this lane is now empty
        auto prev_active = parent;
        parent = Container::as_parent(parent->get_parent().lock());
        parent->remove(prev_active);
    }
    else
    {
        parent->remove(node);
    }

    return parent;
}

std::tuple<std::shared_ptr<ParentContainer>, std::shared_ptr<ParentContainer>> MiralTilingWindowTree::transfer_node(
    std::shared_ptr<Container> const& node, std::shared_ptr<Container> const& to)
{
    // When we remove [node] from its initial position, there's a chance
    // that the target_lane was melted into another lane. Hence, we need to return it
    auto to_update = handle_remove(node);
    auto target_parent = Container::as_parent(to->get_parent().lock());
    auto index = target_parent->get_index_of_node(to);
    target_parent->graft_existing(node, index + 1);
    node->tree(target_parent->tree());

    return { target_parent, to_update };
}

void MiralTilingWindowTree::recalculate_root_node_area()
{
    for (auto const& zone : tree_interface->get_zones())
    {
        root_lane->set_logical_area(zone.extents());
        root_lane->commit_changes();
        break;
    }
}

namespace
{
std::shared_ptr<Container> foreach_node_internal(
    std::function<bool(std::shared_ptr<Container> const&)> const& f,
    std::shared_ptr<Container> const& parent)
{
    if (f(parent))
        return parent;

    if (parent->is_leaf())
        return nullptr;

    for (auto& node : Container::as_parent(parent)->get_sub_nodes())
    {
        if (auto result = foreach_node_internal(f, node))
            return result;
    }

    return nullptr;
}
}

void MiralTilingWindowTree::foreach_node(std::function<void(std::shared_ptr<Container> const&)> const& f) const
{
    foreach_node_internal(
        [&](auto const& node)
    { f(node); return false; },
        root_lane);
}

bool MiralTilingWindowTree::foreach_node_pred(std::function<bool(std::shared_ptr<Container> const&)> const& f) const
{
    return foreach_node_internal(
               [&](auto const& node)
    { return f(node); },
               root_lane)
        != nullptr;
}

void MiralTilingWindowTree::hide()
{
    if (is_hidden)
    {
        mir::log_warning("Tree is already hidden");
        return;
    }

    is_hidden = true;
    root_lane->hide();
}

std::shared_ptr<LeafContainer> MiralTilingWindowTree::show()
{
    if (!is_hidden)
    {
        mir::log_warning("Tree is already shown");
        return nullptr;
    }

    root_lane->show();
    is_hidden = false;

    // TODO: This check is probably unnecessary
    std::shared_ptr<Container> fullscreen_node = nullptr;
    foreach_node([&](auto const& container)
    {
        if (container->is_fullscreen())
            fullscreen_node = container;
    });

    return Container::as_leaf(fullscreen_node);
}

bool MiralTilingWindowTree::is_empty() const
{
    return root_lane->num_nodes() == 0;
}

Workspace* MiralTilingWindowTree::get_workspace() const
{
    return tree_interface->get_workspace();
}

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
#include <iostream>
#include <memory>
#include <mir/log.h>

using namespace miracle;

TilingWindowTree::TilingWindowTree(
    std::unique_ptr<TilingWindowTreeInterface> tree_interface,
    WindowController& window_controller,
    CompositorState const& state,
    std::shared_ptr<Config> const& config,
    geom::Rectangle const& area) :
    root_lane { std::make_shared<ParentContainer>(
        window_controller,
        area,
        config,
        this,
        nullptr,
        state) },
    config { config },
    window_controller { window_controller },
    state { state },
    tree_interface { std::move(tree_interface) }
{
    recalculate_root_node_area();
    config_handle = config->register_listener([&](auto&)
    {
        recalculate_root_node_area();
    });
}

TilingWindowTree::~TilingWindowTree()
{
    config->unregister_listener(config_handle);
}

miral::WindowSpecification TilingWindowTree::place_new_window(
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

std::shared_ptr<LeafContainer> TilingWindowTree::confirm_window(
    miral::WindowInfo const& window_info,
    std::shared_ptr<ParentContainer> const& container)
{
    auto parent = container ? container : root_lane;
    return parent->confirm_window(window_info.window());
}

void TilingWindowTree::graft(
    std::shared_ptr<ParentContainer> const& incoming,
    std::shared_ptr<ParentContainer> const& parent,
    int index)
{
    incoming->set_tree(this);
    parent->graft_existing(incoming, index == -1 ? (int)parent->num_nodes() : index);
    parent->commit_changes();
}

void TilingWindowTree::graft(
    std::shared_ptr<LeafContainer> const& leaf,
    std::shared_ptr<ParentContainer> const& parent,
    int index)
{
    leaf->set_tree(this);
    parent->graft_existing(leaf, index == -1 ? (int)parent->num_nodes() : index);
    parent->commit_changes();
}

bool TilingWindowTree::resize_container(miracle::Direction direction, int pixels, Container& container)
{
    handle_resize(container, direction, pixels);
    return true;
}

bool TilingWindowTree::set_size(std::optional<int> const& width, std::optional<int> const& height, Container& container)
{
    auto const& rectangle = container.get_visible_area();
    int new_width = width ? width.value() : rectangle.size.width.as_int();
    int new_height = height ? height.value() : rectangle.size.height.as_int();
    int diff_x = new_width - rectangle.size.width.as_int();
    int diff_y = new_height - rectangle.size.height.as_int();

    if (diff_x < 0)
        handle_resize(container, Direction::left, -diff_x);
    else
        handle_resize(container, Direction::right, diff_x);

    if (diff_y < 0)
        handle_resize(container, Direction::up, -diff_y);
    else
        handle_resize(container, Direction::down, diff_y);

    return true;
}

bool TilingWindowTree::select_next(
    miracle::Direction direction, Container& container)
{
    auto node = handle_select(container, direction);
    if (!node)
    {
        mir::log_warning("Unable to select the next window: handle_select failed");
        return false;
    }

    window_controller.select_active_window(node->window().value());
    return true;
}

bool TilingWindowTree::toggle_fullscreen(LeafContainer& container)
{
    if (container.is_fullscreen())
        advise_fullscreen_container(container);
    else
        advise_restored_container(container);
    return true;
}

void TilingWindowTree::set_area(geom::Rectangle const& new_area)
{
    root_lane->set_logical_area(new_area);
    root_lane->commit_changes();
}

geom::Rectangle TilingWindowTree::get_area() const
{
    return root_lane->get_logical_area();
}

bool TilingWindowTree::move_container(miracle::Direction direction, Container& container)
{
    auto traversal_result = handle_move(container, direction);
    switch (traversal_result.traversal_type)
    {
    case MoveResult::traversal_type_insert:
    {
        auto target_node = traversal_result.node;
        if (!target_node)
        {
            mir::log_warning("Unable to move active window: target_window not found");
            return false;
        }

        auto target_parent = target_node->get_parent().lock();
        if (!target_parent)
        {
            mir::log_warning("Unable to move active window: second_window has no second_parent");
            return false;
        }

        auto active_parent = Container::as_parent(container.get_parent().lock());
        if (active_parent == target_parent)
        {
            active_parent->swap_nodes(container.shared_from_this(), target_node);
            active_parent->commit_changes();
            break;
        }

        auto [first, second] = transfer_node(container.shared_from_this(), target_node);
        first->commit_changes();
        second->commit_changes();
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

bool TilingWindowTree::move_to(Container& to_move, Container& target)
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

    auto [first, second] = transfer_node(to_move.shared_from_this(), target.shared_from_this());
    first->commit_changes();
    second->commit_changes();
    return true;
}

void TilingWindowTree::request_layout(Container& container, miracle::LayoutScheme scheme)
{
    handle_layout_scheme(scheme, container);
}

void TilingWindowTree::request_vertical_layout(Container& container)
{
    handle_layout_scheme(LayoutScheme::vertical, container);
}

void TilingWindowTree::request_horizontal_layout(Container& container)
{
    handle_layout_scheme(LayoutScheme::horizontal, container);
}

void TilingWindowTree::request_tabbing_layout(Container& container)
{
    handle_layout_scheme(LayoutScheme::tabbing, container);
}

void TilingWindowTree::request_stacking_layout(Container& container)
{
    handle_layout_scheme(LayoutScheme::stacking, container);
}

void TilingWindowTree::toggle_layout(Container& container, bool cycle_thru_all)
{
    auto parent = Container::as_parent(container.get_parent().lock());
    if (!parent)
    {
        mir::log_error("toggle_layout: unable to get parent container");
        return;
    }

    if (cycle_thru_all)
        handle_layout_scheme(get_next_layout(parent->get_direction()), container);
    else
    {
        if (parent->get_direction() == LayoutScheme::horizontal)
            handle_layout_scheme(LayoutScheme::vertical, container);
        else if (parent->get_direction() == LayoutScheme::vertical)
            handle_layout_scheme(LayoutScheme::horizontal, container);
        else
            mir::log_error("Parent with stack layout scheme cannot be toggled");
    }
}

void TilingWindowTree::handle_layout_scheme(LayoutScheme scheme, Container& container)
{
    auto parent = container.get_parent().lock();
    if (!parent)
    {
        mir::log_warning("handle_layout_scheme: parent is not set");
        return;
    }

    // If the parent already has more than just [container] as a child AND
    // the parent is NOT a tabbing/stacking parent, then we create a new parent for this
    // single [container].
    if (parent->num_nodes() > 1
        && parent->get_direction() != LayoutScheme::tabbing
        && parent->get_direction() != LayoutScheme::stacking)
        parent = parent->convert_to_parent(container.shared_from_this());

    parent->set_layout(scheme);
}

void TilingWindowTree::advise_focus_gained(LeafContainer& container)
{
    if (container.is_fullscreen())
        window_controller.raise(container.window().value());
    else if (auto const& parent = container.get_parent().lock())
        parent->on_focus_gained();
}

void TilingWindowTree::advise_delete_window(std::shared_ptr<Container> const& container)
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

bool is_negative_direction(Direction direction)
{
    return direction == Direction::left || direction == Direction::up;
}

bool is_vertical_direction(Direction direction)
{
    return direction == Direction::up || direction == Direction::down;
}

std::shared_ptr<LeafContainer> get_closest_window_to_select_from_node(
    std::shared_ptr<Container> node,
    miracle::Direction direction)
{
    // This function attempts to get the first window within a node provided the direction that we are coming
    // from as a hint. If the node that we want to move to has the same direction as that which we are coming
    // from, a seamless experience would mean that - at times - we select the _LAST_ node in that list, instead
    // of the first one. This makes it feel as though we are moving "across" the screen.
    if (node->is_leaf())
        return Container::as_leaf(node);

    bool is_vertical = is_vertical_direction(direction);
    bool is_negative = is_negative_direction(direction);
    auto lane_node = Container::as_parent(node);
    if (is_vertical && lane_node->get_direction() == LayoutScheme::vertical
        || !is_vertical && lane_node->get_direction() == LayoutScheme::horizontal)
    {
        if (is_negative)
        {
            auto sub_nodes = lane_node->get_sub_nodes();
            for (auto i = sub_nodes.size() - 1; i != 0; i--)
            {
                if (auto retval = get_closest_window_to_select_from_node(sub_nodes[i], direction))
                    return retval;
            }
        }
    }

    for (auto const& sub_node : lane_node->get_sub_nodes())
    {
        if (auto retval = get_closest_window_to_select_from_node(sub_node, direction))
            return retval;
    }

    return nullptr;
}
}

std::shared_ptr<LeafContainer> TilingWindowTree::handle_select(
    Container& from,
    Direction direction)
{
    // Algorithm:
    //  1. Retrieve the parent
    //  2. If the parent matches the target direction, then
    //     we select the next node in the direction
    //  3. If the current_node does NOT match the target direction,
    //     then we climb the tree until we find a current_node who matches
    //  4. If none match, we return nullptr
    bool is_vertical = is_vertical_direction(direction);
    bool is_negative = is_negative_direction(direction);
    auto current_node = from.shared_from_this();
    auto parent = current_node->get_parent().lock();
    if (!parent)
    {
        mir::log_warning("Cannot handle_select the root node");
        return nullptr;
    }

    do
    {
        auto grandparent_direction = parent->get_direction();
        int index = parent->get_index_of_node(current_node);
        if (is_vertical && (grandparent_direction == LayoutScheme::vertical || grandparent_direction == LayoutScheme::stacking)
            || !is_vertical && (grandparent_direction == LayoutScheme::horizontal || grandparent_direction == LayoutScheme::tabbing))
        {
            if (is_negative)
            {
                if (index > 0)
                    return get_closest_window_to_select_from_node(parent->at(index - 1), direction);
            }
            else
            {
                if (index < parent->num_nodes() - 1)
                    return get_closest_window_to_select_from_node(parent->at(index + 1), direction);
            }
        }

        current_node = parent;
        parent = Container::as_parent(parent->get_parent().lock());
    } while (parent != nullptr);

    return nullptr;
}

TilingWindowTree::MoveResult TilingWindowTree::handle_move(Container& from, Direction direction)
{
    // Algorithm:
    //  1. Perform the _select algorithm. If that passes, then we want to be where the selected node
    //     currently is
    //  2. If our parent layout direction does not equal the root layout direction, we can append
    //     or prepend to the root
    if (auto insert_node = handle_select(from, direction))
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
            state);
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

void TilingWindowTree::handle_resize(
    Container& node,
    Direction direction,
    int amount)
{
    auto parent = Container::as_parent(node.get_parent().lock());
    if (parent == nullptr)
    {
        // Can't resize, most likely the root
        return;
    }

    bool is_vertical = direction == Direction::up || direction == Direction::down;
    bool is_main_axis_movement = (is_vertical && parent->get_direction() == LayoutScheme::vertical)
        || (!is_vertical && parent->get_direction() == LayoutScheme::horizontal);

    if (is_main_axis_movement && parent->num_nodes() == 1)
    {
        // Can't resize if we only have ourselves!
        return;
    }

    if (!is_main_axis_movement)
    {
        handle_resize(*parent, direction, amount);
        return;
    }

    bool is_negative = direction == Direction::left || direction == Direction::up;
    auto resize_amount = is_negative ? -amount : amount;
    auto nodes = parent->get_sub_nodes();
    std::vector<geom::Rectangle> pending_node_resizes;
    if (is_vertical)
    {
        int height_for_others = (int)floor(-(double)resize_amount / static_cast<double>(nodes.size() - 1));
        int total_height = 0;
        for (size_t i = 0; i < nodes.size(); i++)
        {
            auto const& other_node = nodes[i];
            auto other_rect = other_node->get_logical_area();
            if (node.shared_from_this() == other_node)
                other_rect.size.height = geom::Height { other_rect.size.height.as_int() + resize_amount };
            else
                other_rect.size.height = geom::Height { other_rect.size.height.as_int() + height_for_others };

            if (i != 0)
            {
                auto const& prev_rect = pending_node_resizes[i - 1];
                other_rect.top_left.y = geom::Y { prev_rect.top_left.y.as_int() + prev_rect.size.height.as_int() };
            }

            if (other_rect.size.height.as_int() <= other_node->get_min_height())
            {
                mir::log_warning("Unable to resize a rectangle that would cause another to be negative");
                return;
            }

            total_height += other_rect.size.height.as_int();
            pending_node_resizes.push_back(other_rect);
        }

        // Due to some rounding errors, we may have to extend the final node
        int leftover_height = parent->get_logical_area().size.height.as_int() - total_height;
        pending_node_resizes.back().size.height = geom::Height { pending_node_resizes.back().size.height.as_int() + leftover_height };
    }
    else
    {
        int width_for_others = (int)floor((double)-resize_amount / static_cast<double>(nodes.size() - 1));
        int total_width = 0;
        for (size_t i = 0; i < nodes.size(); i++)
        {
            auto const& other_node = nodes[i];
            auto other_rect = other_node->get_logical_area();
            if (node.shared_from_this() == other_node)
                other_rect.size.width = geom::Width { other_rect.size.width.as_int() + resize_amount };
            else
                other_rect.size.width = geom::Width { other_rect.size.width.as_int() + width_for_others };

            if (i != 0)
            {
                auto const& prev_rect = pending_node_resizes[i - 1];
                other_rect.top_left.x = geom::X { prev_rect.top_left.x.as_int() + prev_rect.size.width.as_int() };
            }

            if (other_rect.size.width.as_int() <= other_node->get_min_width())
            {
                mir::log_warning("Unable to resize a rectangle that would cause another to be negative");
                return;
            }

            total_width += other_rect.size.width.as_int();
            pending_node_resizes.push_back(other_rect);
        }

        // Due to some rounding errors, we may have to extend the final node
        int leftover_width = parent->get_logical_area().size.width.as_int() - total_width;
        pending_node_resizes.back().size.width = geom::Width { pending_node_resizes.back().size.width.as_int() + leftover_width };
    }

    for (size_t i = 0; i < nodes.size(); i++)
    {
        nodes[i]->set_logical_area(pending_node_resizes[i]);
        nodes[i]->commit_changes();
    }
}

std::shared_ptr<ParentContainer> TilingWindowTree::handle_remove(std::shared_ptr<Container> const& node)
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

std::tuple<std::shared_ptr<ParentContainer>, std::shared_ptr<ParentContainer>> TilingWindowTree::transfer_node(
    std::shared_ptr<Container> const& node, std::shared_ptr<Container> const& to)
{
    // We are moving the active window to a new lane
    auto to_update = handle_remove(node);

    // Note: When we remove moving_node from its initial position, there's a chance
    // that the target_lane was melted into another lane. Hence, we need to run it
    auto target_parent = Container::as_parent(to->get_parent().lock());
    auto index = target_parent->get_index_of_node(to);
    target_parent->graft_existing(node, index + 1);

    return { target_parent, to_update };
}

void TilingWindowTree::recalculate_root_node_area()
{
    for (auto const& zone : tree_interface->get_zones())
    {
        root_lane->set_logical_area(zone.extents());
        root_lane->commit_changes();
        break;
    }
}

bool TilingWindowTree::advise_fullscreen_container(LeafContainer& container)
{
    auto window = container.window().value();
    window_controller.select_active_window(window);
    window_controller.raise(window);
    return true;
}

bool TilingWindowTree::advise_restored_container(LeafContainer& container)
{
    auto active = state.focused_container();
    if (active && active->window() == container.window().value())
    {
        container.set_logical_area(container.get_logical_area());
        container.commit_changes();
    }

    return true;
}

bool TilingWindowTree::handle_container_ready(LeafContainer& container)
{
    constrain(container);
    if (state.focused_container() && state.focused_container()->is_fullscreen())
        return true;

    auto window = container.window().value();
    auto& info = window_controller.info_for(window);
    if (info.can_be_active())
        window_controller.select_active_window(window);

    return true;
}

bool TilingWindowTree::confirm_placement_on_display(
    Container& container,
    MirWindowState new_state,
    mir::geometry::Rectangle& new_placement)
{
    //    auto rect = container.get_visible_area();
    switch (new_state)
    {
    case mir_window_state_restored:
        //        new_placement = rect;
        break;
    default:
        break;
    }

    return true;
}

bool TilingWindowTree::constrain(Container& container)
{
    if (is_hidden)
        return false;

    if (container.get_parent().expired())
    {
        mir::log_error("Unable to constrain node without parent");
        return true;
    }

    container.get_parent().lock()->constrain();
    return true;
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

void TilingWindowTree::foreach_node(std::function<void(std::shared_ptr<Container> const&)> const& f)
{
    foreach_node_internal(
        [&](auto const& node)
    { f(node); return false; },
        root_lane);
}

bool TilingWindowTree::foreach_node_pred(std::function<bool(std::shared_ptr<Container> const&)> const& f)
{
    return foreach_node_internal(
               [&](auto const& node)
    { return f(node); },
               root_lane)
        != nullptr;
}

void TilingWindowTree::hide()
{
    if (is_hidden)
    {
        mir::log_warning("Tree is already hidden");
        return;
    }

    is_hidden = true;
    root_lane->hide();
}

std::shared_ptr<LeafContainer> TilingWindowTree::show()
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

bool TilingWindowTree::is_empty()
{
    return root_lane->num_nodes() == 0;
}

Workspace* TilingWindowTree::get_workspace() const
{
    return tree_interface->get_workspace();
}

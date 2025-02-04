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

#define MIR_LOG_COMPONENT "leaf_container"

#include "leaf_container.h"
#include "compositor_state.h"
#include "config.h"
#include "container_group_container.h"
#include "output.h"
#include "output_manager.h"
#include "parent_container.h"
#include "render_data_manager.h"
#include "window_helpers.h"
#include "workspace.h"

#include <cmath>
#include <mir/log.h>
#include <mir/scene/session.h>
#include <mir/scene/surface.h>
#include <mir_toolkit/common.h>

using namespace miracle;

namespace
{
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

const char* scratchpad_state_to_string(ScratchpadState state)
{
    switch (state)
    {
    case ScratchpadState::none:
        return "none";
    case ScratchpadState::fresh:
        return "fresh";
    case ScratchpadState::changed:
        return "changed";
    default:
        return "unknown";
    }
}

std::shared_ptr<ParentContainer> handle_remove_container(std::shared_ptr<Container> const& container)
{
    auto parent = Container::as_parent(container->get_parent().lock());
    if (parent == nullptr)
        return nullptr;

    if (parent->num_nodes() == 1 && parent->get_parent().lock())
    {
        // Remove the entire parent if this parent is now empty
        auto prev_active = parent;
        parent = Container::as_parent(parent->get_parent().lock());
        parent->remove(prev_active);
    }
    else
    {
        parent->remove(container);
    }

    return parent;
}

std::tuple<std::shared_ptr<ParentContainer>, std::shared_ptr<ParentContainer>> transfer_node(
    std::shared_ptr<Container> const& node, std::shared_ptr<Container> const& to)
{
    // When we remove [node] from its initial position, there's a chance
    // that the target_lane was melted into another lane. Hence, we need to return it
    auto to_update = handle_remove_container(node);
    auto target_parent = Container::as_parent(to->get_parent().lock());
    auto index = target_parent->get_index_of_node(to);
    target_parent->graft_existing(node, index + 1);
    node->set_workspace(target_parent->get_workspace());

    return { target_parent, to_update };
}
}

LeafContainer::LeafContainer(
    Workspace* workspace,
    WindowController& node_interface,
    geom::Rectangle area,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<ParentContainer> const& parent,
    CompositorState const& state,
    OutputManager* output_manager) :
    workspace { workspace },
    window_controller { node_interface },
    logical_area { std::move(area) },
    config { config },
    parent { parent },
    state { state },
    output_manager { output_manager }
{
}

LeafContainer::~LeafContainer()
{
    state.render_data_manager()->remove(*this);
}

void LeafContainer::associate_to_window(miral::Window const& in_window)
{
    window_ = in_window;
    state.render_data_manager()->add(*this);
}

geom::Rectangle LeafContainer::get_logical_area() const
{
    return next_logical_area ? next_logical_area.value() : logical_area;
}

void LeafContainer::set_logical_area(geom::Rectangle const& target_rect, bool with_animations)
{
    next_logical_area = target_rect;
    next_with_animations = with_animations;
}

std::weak_ptr<ParentContainer> LeafContainer::get_parent() const
{
    return parent;
}

void LeafContainer::set_parent(std::shared_ptr<ParentContainer> const& in_parent)
{
    parent = in_parent;

    miral::WindowSpecification spec;
    spec.depth_layer() = !in_parent->anchored() ? mir_depth_layer_above : mir_depth_layer_application;
    window_controller.modify(window_, spec);
}

void LeafContainer::set_state(MirWindowState state)
{
    next_state = state;
}

geom::Rectangle LeafContainer::get_visible_area() const
{
    // TODO: Could cache these half values in the config
    int const half_gap_x = (int)(ceil((double)config->get_inner_gaps_x() / 2.0));
    int const half_gap_y = (int)(ceil((double)config->get_inner_gaps_y() / 2.0));
    auto neighbors = get_neighbors();
    int x = logical_area.top_left.x.as_int();
    int y = logical_area.top_left.y.as_int();
    int width = logical_area.size.width.as_int();
    int height = logical_area.size.height.as_int();
    if (neighbors[(int)Direction::left])
    {
        x += half_gap_x;
        width -= half_gap_x;
    }
    if (neighbors[(int)Direction::right])
    {
        width -= half_gap_x;
    }
    if (neighbors[(int)Direction::up])
    {
        y += half_gap_y;
        height -= half_gap_y;
    }
    if (neighbors[(int)Direction::down])
    {
        height -= half_gap_y;
    }

    int const border_size = config->get_border_config().size;
    x += border_size;
    width -= 2 * border_size;
    y += border_size;
    height -= 2 * border_size;

    return {
        geom::Point { x,     y      },
        geom::Size { width, height }
    };
}

void LeafContainer::constrain()
{
    if (window_controller.is_fullscreen(window_) || is_dragging_)
        window_controller.noclip(window_);
    else
        window_controller.clip(window_, get_visible_area());
}

size_t LeafContainer::get_min_width() const
{
    return 50;
}

size_t LeafContainer::get_min_height() const
{
    return 50;
}

void LeafContainer::handle_ready()
{
    constrain();
    if (!state.focused_container() || !state.focused_container()->is_fullscreen())
    {
        auto& info = window_controller.info_for(window_);
        if (info.can_be_active())
            window_controller.select_active_window(window_);
    }

    if (window_controller.is_fullscreen(window_))
        toggle_fullscreen();
}

void LeafContainer::handle_modify(miral::WindowSpecification const& modifications)
{
    auto const& info = window_controller.info_for(window_);

    auto mods = modifications;
    if (mods.size().is_set())
        window_controller.set_size_hack(animation_handle_, mods.size().value());

    if (mods.state().is_set() && mods.state().value() != info.state())
    {
        set_state(mods.state().value());
        commit_changes();

        if (window_helpers::is_window_fullscreen(mods.state().value()))
        {
            window_controller.select_active_window(window_);
            window_controller.raise(window_);
        }
        else if (mods.state().value() == mir_window_state_restored)
        {
            auto active = state.focused_container();
            if (active && active->window() == window_)
            {
                set_logical_area(get_logical_area());
                commit_changes();
            }
        }
    }

    window_controller.modify(window_, mods);
}

void LeafContainer::handle_raise()
{
    window_controller.select_active_window(window_);
}

bool LeafContainer::resize(miracle::Direction direction, int pixels)
{
    handle_resize(this, direction, pixels);
    return true;
}

void LeafContainer::handle_resize(Container* container, Direction direction, int amount)
{
    auto sh_parent = container->get_parent().lock();
    if (!sh_parent)
        return;

    bool is_vertical = direction == Direction::up || direction == Direction::down;
    bool is_main_axis_movement = (is_vertical && sh_parent->get_direction() == LayoutScheme::vertical)
        || (!is_vertical && sh_parent->get_direction() == LayoutScheme::horizontal);

    if (is_main_axis_movement && sh_parent->num_nodes() == 1)
    {
        // Can't resize if we only have ourselves!
        return;
    }

    if (!is_main_axis_movement)
    {
        handle_resize(sh_parent.get(), direction, amount);
        return;
    }

    bool is_negative = direction == Direction::left || direction == Direction::up;
    auto resize_amount = is_negative ? -amount : amount;
    auto nodes = sh_parent->get_sub_nodes();
    std::vector<geom::Rectangle> pending_node_resizes;
    if (is_vertical)
    {
        int height_for_others = (int)floor(-(double)resize_amount / static_cast<double>(nodes.size() - 1));
        int total_height = 0;
        for (size_t i = 0; i < nodes.size(); i++)
        {
            auto const& other_node = nodes[i];
            auto other_rect = other_node->get_logical_area();
            if (container == other_node.get())
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
        int leftover_height = sh_parent->get_logical_area().size.height.as_int() - total_height;
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
            if (container == other_node.get())
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
        int leftover_width = sh_parent->get_logical_area().size.width.as_int() - total_width;
        pending_node_resizes.back().size.width = geom::Width { pending_node_resizes.back().size.width.as_int() + leftover_width };
    }

    for (size_t i = 0; i < nodes.size(); i++)
    {
        nodes[i]->set_logical_area(pending_node_resizes[i]);
        nodes[i]->commit_changes();
    }
}

bool LeafContainer::set_size(std::optional<int> const& width, std::optional<int> const& height)
{
    auto rectangle = get_visible_area();
    int new_width = width ? width.value() : rectangle.size.width.as_int();
    int new_height = height ? height.value() : rectangle.size.height.as_int();
    int diff_x = new_width - rectangle.size.width.as_int();
    int diff_y = new_height - rectangle.size.height.as_int();

    if (diff_x < 0)
        resize(Direction::left, -diff_x);
    else
        resize(Direction::right, diff_x);

    if (diff_y < 0)
        resize(Direction::up, -diff_y);
    else
        resize(Direction::down, diff_y);

    return true;
}

void LeafContainer::show()
{
    next_state = before_shown_state;
    before_shown_state.reset();
    commit_changes();
    window_controller.raise(window_);
}

void LeafContainer::hide()
{
    before_shown_state = window_controller.get_state(window_);
    next_state = mir_window_state_hidden;
    commit_changes();
    window_controller.send_to_back(window_);
}

bool LeafContainer::toggle_fullscreen()
{
    if (window_controller.is_fullscreen(window_))
    {
        next_state = mir_window_state_restored;
        next_depth_layer = !parent.lock()->anchored() ? mir_depth_layer_above : mir_depth_layer_application;
    }
    else
    {
        next_state = mir_window_state_fullscreen;
        next_depth_layer = mir_depth_layer_always_on_top;
        window_controller.select_active_window(window_);
        window_controller.raise(window_);
    }

    commit_changes();
    return true;
}

mir::geometry::Rectangle LeafContainer::confirm_placement(
    MirWindowState state, mir::geometry::Rectangle const& placement)
{
    return placement;
}

void LeafContainer::on_open()
{
    window_controller.open(window_);
}

void LeafContainer::on_focus_gained()
{
    if (auto sh_parent = parent.lock())
        sh_parent->on_focus_gained();
    state.render_data_manager()->focus_change(*this);
}

void LeafContainer::on_focus_lost()
{
    state.render_data_manager()->focus_change(*this);
}

void LeafContainer::on_move_to(geom::Point const&)
{
}

bool LeafContainer::is_fullscreen() const
{
    return window_controller.is_fullscreen(window_);
}

void LeafContainer::commit_changes()
{
    if (next_state)
    {
        window_controller.change_state(window_, next_state.value());
        constrain();
        next_state.reset();
    }

    if (next_depth_layer)
    {
        miral::WindowSpecification spec;
        spec.depth_layer() = next_depth_layer.value();
        window_controller.modify(window_, spec);
        next_depth_layer.reset();
    }

    if (next_logical_area)
    {
        auto previous = get_visible_area();
        logical_area = next_logical_area.value();
        next_logical_area.reset();
        if (!window_controller.is_fullscreen(window_))
        {
            auto next_visible_area = get_visible_area();
            if (is_dragging_ && next_visible_area.top_left != dragged_position)
                next_visible_area.top_left = dragged_position;

            window_controller.set_rectangle(window_, previous, next_visible_area, next_with_animations);
            next_with_animations = true;
        }
    }
}

void LeafContainer::handle_request_move(MirInputEvent const* input_event)
{
}

void LeafContainer::handle_request_resize(MirInputEvent const* input_event, MirResizeEdge edge)
{
}

void LeafContainer::request_horizontal_layout()
{
    handle_layout_scheme(this, LayoutScheme::horizontal);
}

void LeafContainer::request_vertical_layout()
{
    handle_layout_scheme(this, LayoutScheme::vertical);
}

void LeafContainer::toggle_layout(bool cycle_thru_all)
{
    auto sh_parent = parent.lock();
    if (!sh_parent)
    {
        mir::log_error("toggle_layout: unable to get parent container");
        return;
    }

    if (cycle_thru_all)
        handle_layout_scheme(this, get_next_layout(sh_parent->get_direction()));
    else
    {
        if (sh_parent->get_direction() == LayoutScheme::horizontal)
            handle_layout_scheme(this, LayoutScheme::vertical);
        else if (sh_parent->get_direction() == LayoutScheme::vertical)
            handle_layout_scheme(this, LayoutScheme::horizontal);
        else
            mir::log_error("Parent with stack layout scheme cannot be toggled");
    }
}

Workspace* LeafContainer::get_workspace() const
{
    return workspace;
}

void LeafContainer::set_workspace(miracle::Workspace* in)
{
    workspace = in;
}

Output* LeafContainer::get_output() const
{
    return workspace->get_output();
}

glm::mat4 LeafContainer::get_transform() const
{
    return transform;
}

void LeafContainer::set_transform(glm::mat4 transform_)
{
    if (auto surface = window_.operator std::shared_ptr<mir::scene::Surface>())
    {
        surface->set_transformation(transform_);
        transform = transform_;
        state.render_data_manager()->transform_change(*this);
    }
}

uint32_t LeafContainer::animation_handle() const
{
    return animation_handle_;
}

void LeafContainer::animation_handle(uint32_t handle)
{
    animation_handle_ = handle;
}

bool LeafContainer::is_focused() const
{
    if ((state.focused_container() && state.focused_container().get() == this) || parent.lock()->is_focused())
        return true;

    auto group = Container::as_group(state.focused_container());
    if (!group)
        return false;

    return group->contains(shared_from_this());
}

ContainerType LeafContainer::get_type() const
{
    return ContainerType::leaf;
}

bool LeafContainer::select_next(miracle::Direction direction)
{
    auto next = handle_select(*this, direction);
    if (!next)
    {
        mir::log_warning("Unable to select the next window: handle_select failed");
        return false;
    }

    window_controller.select_active_window(next->window().value());
    return true;
}

std::shared_ptr<LeafContainer> LeafContainer::handle_select(
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

bool LeafContainer::pinned(bool value)
{
    if (auto sh_parent = parent.lock())
        return sh_parent->pinned(value);
    return false;
}

bool LeafContainer::pinned() const
{
    if (auto sh_parent = parent.lock())
        return sh_parent->pinned();
    return false;
}

bool LeafContainer::move(miracle::Direction direction)
{
    return workspace->move_container(direction, *this);
}

bool LeafContainer::move_by(Direction, int)
{
    return false;
}

bool LeafContainer::move_by(float dx, float dy)
{
    if (auto sh_parent = parent.lock())
        return sh_parent->move_by(dx, dy);
    return false;
}

/// Move this container to the position of the [target].
/// \returns true if the move was successful, otherwise false.
bool LeafContainer::move_to(Container& target)
{
    auto target_parent = target.get_parent().lock();
    if (!target_parent)
    {
        mir::log_warning("Unable to move active window: second_window has no second_parent");
        return false;
    }

    auto active_parent = Container::as_parent(get_parent().lock());
    if (active_parent == target_parent)
    {
        active_parent->swap_nodes(shared_from_this(), target.shared_from_this());
        active_parent->commit_changes();
        return true;
    }

    // Transfer the node to the new parent.
    auto [first, second] = transfer_node(shared_from_this(), target.shared_from_this());
    first->commit_changes();
    second->commit_changes();
    return true;
}

bool LeafContainer::move_to(int x, int y)
{
    return false;
}

bool LeafContainer::toggle_tabbing()
{
    if (auto sh_parent = parent.lock())
    {
        if (sh_parent->get_direction() == LayoutScheme::tabbing)
            request_horizontal_layout();
        else
            handle_layout_scheme(this, LayoutScheme::tabbing);
    }
    return true;
}

bool LeafContainer::toggle_stacking()
{
    if (auto sh_parent = parent.lock())
    {
        if (sh_parent->get_direction() == LayoutScheme::stacking)
            request_horizontal_layout();
        else
            handle_layout_scheme(this, LayoutScheme::stacking);
    }
    return true;
}

bool LeafContainer::drag_start()
{
    if (is_dragging_)
        mir::log_error("Attempting to start a drag when we are already dragging");

    is_dragging_ = true;
    constrain();
    return true;
}

void LeafContainer::drag(int x, int y)
{
    if (!is_dragging_)
        return;

    miral::WindowSpecification spec;
    spec.top_left() = { x, y };
    dragged_position = { x, y };
    window_controller.modify(window_, spec);
}

bool LeafContainer::drag_stop()
{
    if (!is_dragging_)
        mir::log_error("Attempting to stop a drag when we are not dragging");

    is_dragging_ = false;

    miral::WindowSpecification spec;
    auto visible_area = get_visible_area();
    geom::Rectangle previous = { dragged_position, visible_area.size };
    window_controller.set_rectangle(window_, previous, visible_area);
    constrain();
    return true;
}

bool LeafContainer::set_layout(LayoutScheme scheme)
{
    handle_layout_scheme(this, scheme);
    return true;
}

bool LeafContainer::anchored() const
{
    return !parent.expired() && parent.lock()->anchored();
}

ScratchpadState LeafContainer::scratchpad_state() const
{
    if (!parent.expired())
        return parent.lock()->scratchpad_state();

    return ScratchpadState::none;
}

void LeafContainer::scratchpad_state(ScratchpadState next_scratchpad_state)
{
    if (!parent.expired())
        return parent.lock()->scratchpad_state(next_scratchpad_state);
}

void LeafContainer::handle_layout_scheme(Container* container, LayoutScheme scheme)
{
    auto parent = container->get_parent().lock();
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
        parent = parent->convert_to_parent(container->shared_from_this());

    parent->set_layout(scheme);
}

LayoutScheme LeafContainer::get_layout() const
{
    auto sh_parent = parent.lock().get();
    if (!sh_parent)
        return LayoutScheme::none;

    if (sh_parent->num_nodes() == 1)
        return sh_parent->get_layout();

    return LayoutScheme::none;
}

nlohmann::json LeafContainer::to_json() const
{
    auto const app = window_.application();
    auto const& win_info = window_controller.info_for(window_);
    auto visible_area = get_visible_area();
    auto workspace = get_workspace();
    auto output = get_output();
    auto locked_parent = parent.lock();
    bool visible = true;

    if (output_manager->focused() != output)
        visible = false;

    if (output->active() != workspace)
        visible = false;

    if (locked_parent == nullptr)
        visible = false;

    if (locked_parent->get_scheme() == LayoutScheme::stacking || locked_parent->get_scheme() == LayoutScheme::tabbing)
        if (!is_focused())
            visible = false;

    nlohmann::json properties = nlohmann::json::object();
    return {
        { "id",                   reinterpret_cast<std::uintptr_t>(this)                                                                                                                                                                                                                                   },
        { "name",                 app->name()                                                                                                                                                                                                                                                              },
        { "rect",                 {
                      { "x", logical_area.top_left.x.as_int() },
                      { "y", logical_area.top_left.y.as_int() },
                      { "width", logical_area.size.width.as_int() },
                      { "height", logical_area.size.height.as_int() },
                  }                                                                                                                                                                                                                                                                       },
        { "focused",              visible && is_focused()                                                                                                                                                                                                                                                  },
        { "focus",                std::vector<int>()                                                                                                                                                                                                                                                       },
        { "border",               "normal"                                                                                                                                                                                                                                                                 },
        { "current_border_width", config->get_border_config().size                                                                                                                                                                                                                                         },
        { "layout",               "none"                                                                                                                                                                                                                                                                   },
        { "orientation",          "none"                                                                                                                                                                                                                                                                   },
        { "percent",              get_percent_of_parent()                                                                                                                                                                                                                                                  },
        { "window_rect",          {
                                                                                                                                                                                                                                                                                     { "x", visible_area.top_left.x.as_int() },
                                                                                                                                                                                                                                                                                     { "y", visible_area.top_left.y.as_int() },
                                                                                                                                                                                                                                                                                     { "width", visible_area.size.width.as_int() },
                                                                                                                                                                                                                                                                                     { "height", visible_area.size.height.as_int() },
                                                                                                                                                                                                                                                                                 } },
        { "deco_rect",            {
                           { "x", 0 },
                           { "y", 0 },
                           { "width", logical_area.size.width.as_int() },
                           { "height", logical_area.size.height.as_int() },
                       }                                                                                                                                                                                                                                                             },
        { "geometry",             {
                          { "x", 0 },
                          { "y", 0 },
                          { "width", logical_area.size.width.as_int() },
                          { "height", logical_area.size.height.as_int() },
                      }                                                                                                                                                                                                                                                               },
        { "window",               0                                                                                                                                                                                                                                                                        }, // TODO
        { "urgent",               false                                                                                                                                                                                                                                                                    },
        { "floating_nodes",       std::vector<int>()                                                                                                                                                                                                                                                       },
        { "sticky",               false                                                                                                                                                                                                                                                                    },
        { "type",                 "con"                                                                                                                                                                                                                                                                    },
        { "fullscreen_mode",      is_fullscreen() ? 1 : 0                                                                                                                                                                                                                                                  }, // TODO: Support value 2
        { "pid",                  app->process_id()                                                                                                                                                                                                                                                        },
        { "app_id",               win_info.application_id()                                                                                                                                                                                                                                                },
        { "visible",              visible                                                                                                                                                                                                                                                                  },
        { "shell",                "miracle-wm"                                                                                                                                                                                                                                                             }, // TODO
        { "inhibit_idle",         false                                                                                                                                                                                                                                                                    },
        { "idle_inhibitors",      {
                                                            { "application", "none" },
                                                            { "user", "visible" },
                                                        }                                                                                                                                                                                                                      },
        { "window_properties",    properties                                                                                                                                                                                                                                                               }, // TODO
        { "nodes",                std::vector<int>()                                                                                                                                                                                                                                                       },
        { "scratchpad_state",     scratchpad_state_to_string(scratchpad_state())                                                                                                                                                                                                                           }
    };
}

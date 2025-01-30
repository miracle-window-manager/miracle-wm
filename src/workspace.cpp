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

#define MIR_LOG_COMPONENT "workspace_content"

#include "workspace.h"
#include "compositor_state.h"
#include "config.h"
#include "container_group_container.h"
#include "floating_tree_container.h"
#include "floating_window_container.h"
#include "leaf_container.h"
#include "output.h"
#include "output_manager.h"
#include "parent_container.h"
#include "render_data_manager.h"
#include "shell_component_container.h"
#include "window_helpers.h"

#include <cassert>
#include <mir/log.h>
#include <mir/scene/surface.h>
#include <miral/zone.h>

using namespace miracle;

namespace
{
std::shared_ptr<ParentContainer> handle_remove_container(std::shared_ptr<Container> const& container)
{
    auto parent = Container::as_parent(container->get_parent().lock());
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
        parent->remove(container);
    }

    return parent;
}

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

MiralWorkspace::MiralWorkspace(
    miracle::Output* output,
    uint32_t id,
    std::optional<int> num,
    std::optional<std::string> name,
    std::shared_ptr<Config> const& config,
    WindowController& window_controller,
    CompositorState const& state,
    std::shared_ptr<MinimalWindowManager> const& floating_window_manager,
    OutputManager* output_manager) :
    output { output },
    id_ { id },
    num_ { num },
    name_ { name },
    window_controller { window_controller },
    state { state },
    config { config },
    output_manager { output_manager },
    floating_window_manager { floating_window_manager },
    root(std::make_shared<ParentContainer>(
        window_controller, output->get_area(), config, this, nullptr, state, output_manager))
{
    config_handle = config->register_listener([this](auto const&)
    {
        recalculate_area();
    });
}

MiralWorkspace::~MiralWorkspace()
{
    config->unregister_listener(config_handle);
}

void MiralWorkspace::set_area(mir::geometry::Rectangle const& area)
{
    root->set_logical_area(area);
    root->commit_changes();
}

void MiralWorkspace::recalculate_area()
{
    for (auto const& zone : output->get_app_zones())
    {
        root->set_logical_area(zone.extents());
        root->commit_changes();
        break;
    }
}

AllocationHint MiralWorkspace::allocate_position(
    miral::ApplicationInfo const& app_info,
    miral::WindowSpecification& requested_specification,
    AllocationHint const& hint)
{
    // If there's no ideal layout type, use the one provided by the workspace
    auto const& workspace_config = config->get_workspace_config(num_, name_);
    auto const layout = hint.container_type == ContainerType::none
        ? workspace_config.layout ? workspace_config.layout.value() : ContainerType::leaf
        : hint.container_type;
    switch (layout)
    {
    case ContainerType::leaf:
    {
        auto parent = hint.parent ? hint.parent.value() : get_layout_container();
        requested_specification = parent->place_new_window(requested_specification);
        return { ContainerType::leaf, parent };
    }
    case ContainerType::floating_window:
    {
        requested_specification = floating_window_manager->place_new_window(app_info, requested_specification);
        requested_specification.server_side_decorated() = false;
        return { ContainerType::floating_window };
    }
    default:
        return { layout };
    }
}

std::shared_ptr<Container> MiralWorkspace::create_container(
    miral::WindowInfo const& window_info,
    AllocationHint const& hint)
{
    std::shared_ptr<Container> container = nullptr;
    miral::WindowSpecification spec;
    switch (hint.container_type)
    {
    case ContainerType::leaf:
    {
        assert(hint.parent.has_value());
        container = hint.parent.value()->confirm_window(window_info.window());
        break;
    }
    case ContainerType::floating_window:
    {
        floating_window_manager->advise_new_window(window_info);
        container = add_floating_window(window_info.window());
        break;
    }
    case ContainerType::shell:
        container = std::make_shared<ShellComponentContainer>(window_info.window(), window_controller);
        break;
    default:
        mir::log_error("Unsupported window type: %d", (int)hint.container_type);
        break;
    }

    spec.userdata() = container;
    spec.min_width() = mir::geometry::Width(0);
    spec.min_height() = mir::geometry::Height(0);
    window_controller.modify(window_info.window(), spec);
    return container;
}

void MiralWorkspace::handle_ready_hack(LeafContainer& container)
{
    // TODO: Hack
    //  By default, new windows are raised. To properly maintain the ordering, we must
    //  raise floating windows and then raise fullscreen windows.
    for (auto const& window : floating_windows)
        window_controller.raise(window->window().value());

    if (state.focused_container() && state.focused_container()->is_fullscreen())
        window_controller.raise(state.focused_container()->window().value());
}

void MiralWorkspace::delete_container(std::shared_ptr<Container> const& container)
{
    switch (container->get_type())
    {
    case ContainerType::leaf:
    {
        auto parent = handle_remove_container(container);
        parent->commit_changes();
        break;
    }
    case ContainerType::floating_window:
    {
        auto floating = Container::as_floating(container);
        floating_window_manager->advise_delete_window(window_controller.info_for(floating->window().value()));
        floating_windows.erase(
            std::remove(floating_windows.begin(), floating_windows.end(), floating), floating_windows.end());
        break;
    }
    default:
        mir::log_error("Unsupported window type: %d", (int)container->get_type());
        return;
    }
}

void MiralWorkspace::advise_focus_gained(std::shared_ptr<Container> const& container)
{
    last_selected_container = container;
}

void MiralWorkspace::show()
{
    root->show();
    for (auto const& floating : floating_windows)
        floating->show();
}

void MiralWorkspace::hide()
{
    root->hide();
    for (auto const& floating : floating_windows)
        floating->hide();
}

void MiralWorkspace::for_each_window(std::function<bool(std::shared_ptr<Container>)> const& f) const
{
    for (auto const& window : floating_windows)
    {
        if (!window->window())
        {
            mir::log_error("MiralWorkspace::for_each_window: floating window has no window");
            continue;
        }

        auto container = window_controller.get_container(window->window().value());
        if (container)
        {
            if (f(container))
                return;
        }
    }

    foreach_node_internal([&](std::shared_ptr<Container> const& node)
    {
        if (auto leaf = Container::as_leaf(node))
        {
            if (!leaf->window())
            {
                mir::log_error("MiralWorkspace::for_each_window: tiled window has no window");
                return false;
            }

            auto container = window_controller.get_container(leaf->window().value());
            if (container && f(container))
                return true;
        }

        return false;
    }, root);
}

void MiralWorkspace::transfer_pinned_windows_to(std::shared_ptr<Workspace> const& other)
{
    for (auto it = floating_windows.begin(); it != floating_windows.end();)
    {
        auto container = window_controller.get_container(it->get()->window().value());
        if (!container)
        {
            mir::log_error("transfer_pinned_windows_to: floating window lacks container");
            it++;
            continue;
        }

        auto floating = Container::as_floating(container);
        if (floating && floating->pinned())
        {
            other->graft(floating);
            it = floating_windows.erase(it);
        }
        else
            it++;
    }
}

std::shared_ptr<FloatingWindowContainer> MiralWorkspace::add_floating_window(miral::Window const& window)
{
    auto floating = std::make_shared<FloatingWindowContainer>(
        window, floating_window_manager, window_controller, this, state, config, output_manager);
    floating_windows.push_back(floating);
    return floating;
}

void MiralWorkspace::remove_floating_hack(std::shared_ptr<Container> const& container)
{
    assert(container->get_type() == ContainerType::floating_window);
    Container::as_floating(container)->set_workspace(nullptr);
    floating_windows.erase(
        std::remove(floating_windows.begin(), floating_windows.end(), container), floating_windows.end());
}

bool MiralWorkspace::move_container(miracle::Direction direction, Container& container)
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
        handle_remove_container(moving_node);
        lane_node->graft_existing(moving_node, lane_node->num_nodes());
        lane_node->commit_changes();
        break;
    }
    case MoveResult::traversal_type_prepend:
    {
        auto lane_node = Container::as_parent(traversal_result.node);
        auto moving_node = container.shared_from_this();
        handle_remove_container(moving_node);
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

bool MiralWorkspace::move_to(Container& to_move, Container& target)
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

bool MiralWorkspace::move_to(Container& to_move)
{
    root->graft_existing(to_move.shared_from_this(), root->num_nodes());
    to_move.set_workspace(this);
    return true;
}

MiralWorkspace::MoveResult MiralWorkspace::handle_move(Container& from, Direction direction)
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
    if (root == parent)
    {
        auto new_layout_direction = from_direction(direction);
        if (new_layout_direction == root->get_direction())
            return {};

        auto after_root_lane = std::make_shared<ParentContainer>(
            window_controller,
            root->get_logical_area(),
            config,
            this,
            nullptr,
            state,
            output_manager);
        after_root_lane->set_layout(new_layout_direction);
        after_root_lane->graft_existing(root, 0);
        root = after_root_lane;
        recalculate_area();
    }

    bool is_negative = is_negative_direction(direction);
    if (is_negative)
        return {
            MoveResult::traversal_type_prepend,
            root
        };
    else
        return {
            MoveResult::traversal_type_append,
            root
        };
}

std::tuple<std::shared_ptr<ParentContainer>, std::shared_ptr<ParentContainer>> MiralWorkspace::transfer_node(
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

void MiralWorkspace::select_first_window()
{
    // Check if the selected container is already on this workspace
    if (state.focused_container() && state.focused_container()->get_workspace() == this)
        return;

    // First, try and select the previously selected container if it is still around
    if (!last_selected_container.expired())
    {
        auto last_selected = last_selected_container.lock();
        if (last_selected->get_type() == ContainerType::leaf)
        {
            auto found = foreach_node_internal([&](std::shared_ptr<Container> const& container)
            {
                if (container == last_selected)
                {
                    window_controller.select_active_window(container->window().value());
                    return true;
                }

                return false;
            }, root);

            if (found)
                return;
        }
        else if (last_selected->get_type() == ContainerType::floating_window)
        {
            for (auto const& floating : floating_windows)
            {
                if (floating == last_selected)
                {
                    window_controller.select_active_window(floating->window().value());
                    return;
                }
            }
        }
    }

    // Otherwise, select the first available tiling window followed by floating windows
    auto found = foreach_node_internal([&](std::shared_ptr<Container> const& container)
    {
        if (Container::as_leaf(container))
        {
            window_controller.select_active_window(container->window().value());
            return true;
        }

        return false;
    }, root);

    if (found)
        return;

    if (!floating_windows.empty())
    {
        window_controller.select_active_window(floating_windows[0]->window().value());
        return;
    }

    // If all fails, select nothing
    window_controller.select_active_window(miral::Window {});
}

Output* MiralWorkspace::get_output() const
{
    return output;
}

void MiralWorkspace::set_output(Output* new_output)
{
    this->output = new_output;
    set_area(output->get_area());
}

void MiralWorkspace::workspace_transform_change_hack()
{
    // TODO: Ugh, sad. I am forced to set the surface transform so that the surface is rerendered
    for_each_window([&](std::shared_ptr<Container> const& container)
    {
        auto window = container->window();
        state.render_data_manager()->workspace_transform_change(*container);
        if (window)
        {
            auto surface = window->operator std::shared_ptr<mir::scene::Surface>();
            if (surface)
                surface->set_transformation(container->get_transform());
        }
        return false;
    });
}

bool MiralWorkspace::is_empty() const
{
    return root->num_nodes() == 0 && floating_windows.empty();
}

void MiralWorkspace::graft(std::shared_ptr<Container> const& container)
{
    switch (container->get_type())
    {
    case ContainerType::floating_window:
    {
        auto floating = Container::as_floating(container);
        floating_windows.push_back(floating);
        break;
    }
    case ContainerType::parent:
    case ContainerType::leaf:
        root->graft_existing(container, root->num_nodes());
        root->commit_changes();
        break;
    default:
        mir::log_error("MiralWorkspace::graft: ungraftable container type: %d", (int)container->get_type());
        break;
    }

    container->set_workspace(this);
}

std::shared_ptr<ParentContainer> MiralWorkspace::get_layout_container()
{
    if (!state.focused_container())
        return root;

    auto parent = state.focused_container()->get_parent().lock();
    if (!parent)
        return root;

    if (parent->get_workspace() != this)
        return root;

    return parent;
}

std::string MiralWorkspace::display_name() const
{
    std::stringstream ss;
    if (num_ && name_)
        ss << num_.value() << ":" << name_.value();
    else if (name_)
        return name_.value();
    else if (num_)
        return std::to_string(num_.value());
    else
        ss << "Unknown #" << id_;

    return ss.str();
}

nlohmann::json MiralWorkspace::to_json() const
{
    bool is_output_focused = output_manager->focused() == output;
    bool const is_focused = output->active() == this;

    // Note: The reported workspace area appears to be the placement
    // area of the root tree.
    //   See: https://i3wm.org/docs/ipc.html#_tree_reply
    auto area = root->get_logical_area();

    nlohmann::json floating_nodes = nlohmann::json::array();
    for (auto const& container : floating_windows)
        floating_nodes.push_back(container->to_json());

    nlohmann::json nodes = nlohmann::json::array();
    for (auto const& container : root->get_sub_nodes())
        nodes.push_back(container->to_json());

    return {
        {
         "num",
         num_ ? num_.value() : -1,
         },
        { "id", reinterpret_cast<std::uintptr_t>(this) },
        { "type", "workspace" },
        { "name", display_name() },
        { "visible", is_output_focused && is_focused },
        { "focused", is_output_focused && is_focused },
        { "urgent", false },
        { "output", output->name() },
        { "border", "none" },
        { "current_border_width", 0 },
        { "layout", to_string(root->get_scheme()) },
        { "orientation", "none" },
        { "window_rect", {
                             { "x", 0 },
                             { "y", 0 },
                             { "width", 0 },
                             { "height", 0 },
                         } },
        { "deco_rect", {
                           { "x", 0 },
                           { "y", 0 },
                           { "width", 0 },
                           { "height", 0 },
                       } },
        { "geometry", {
                          { "x", 0 },
                          { "y", 0 },
                          { "width", 0 },
                          { "height", 0 },
                      } },
        { "window", nullptr },
        { "floating_nodes", floating_nodes },
        { "rect", {
                                                                                   { "x", area.top_left.x.as_int() },
                                                                                   { "y", area.top_left.y.as_int() },
                                                                                   { "width", area.size.width.as_int() },
                                                                                   { "height", area.size.height.as_int() },
                                                                               } },
        { "nodes", nodes }
    };
}

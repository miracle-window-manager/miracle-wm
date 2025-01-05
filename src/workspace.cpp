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
#include "parent_container.h"
#include "shell_component_container.h"
#include "tiling_window_tree.h"
#include "window_helpers.h"

#include <mir/log.h>
#include <mir/scene/surface.h>
#include <miral/zone.h>

using namespace miracle;

namespace
{
class OutputTilingWindowTreeInterface : public TilingWindowTreeInterface
{
public:
    explicit OutputTilingWindowTreeInterface(Output* screen, Workspace* workspace) :
        screen { screen },
        workspace { workspace }
    {
    }

    std::vector<miral::Zone> const& get_zones() override
    {
        return screen->get_app_zones();
    }

    Workspace* get_workspace() const override
    {
        return workspace;
    }

private:
    Output* screen;
    Workspace* workspace;
};

}

Workspace::Workspace(
    miracle::Output* output,
    uint32_t id,
    std::optional<int> num,
    std::optional<std::string> name,
    std::shared_ptr<Config> const& config,
    WindowController& window_controller,
    CompositorState const& state,
    std::shared_ptr<MinimalWindowManager> const& floating_window_manager) :
    output { output },
    id_ { id },
    num_ { num },
    name_ { name },
    window_controller { window_controller },
    state { state },
    config { config },
    floating_window_manager { floating_window_manager },
    tree(std::make_shared<TilingWindowTree>(
        std::make_unique<OutputTilingWindowTreeInterface>(output, this),
        window_controller, state, config, output->get_area()))
{
}

void Workspace::set_area(mir::geometry::Rectangle const& area)
{
    tree->set_area(area);
}

void Workspace::recalculate_area()
{
    tree->recalculate_root_node_area();
}

AllocationHint Workspace::allocate_position(
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
        auto placement_tree = hint.placement_tree ? hint.placement_tree : tree.get();
        requested_specification = placement_tree->place_new_window(requested_specification, get_layout_container());
        return { ContainerType::leaf, placement_tree };
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

std::shared_ptr<Container> Workspace::create_container(
    miral::WindowInfo const& window_info,
    AllocationHint const& hint)
{
    std::shared_ptr<Container> container = nullptr;
    miral::WindowSpecification spec;
    switch (hint.container_type)
    {
    case ContainerType::leaf:
    {
        container = hint.placement_tree->confirm_window(window_info, get_layout_container());
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

    // TODO: hack
    //  Warning: We need to advise fullscreen only after we've associated the userdata() appropriately
    if (hint.container_type == ContainerType::leaf && window_helpers::is_window_fullscreen(window_info.state()))
    {
        hint.placement_tree->advise_fullscreen_container(*Container::as_leaf(container));
    }
    return container;
}

void Workspace::handle_ready_hack(LeafContainer& container)
{
    // TODO: Hack
    //  By default, new windows are raised. To properly maintain the ordering, we must
    //  raise floating windows and then raise fullscreen windows.
    for (auto const& window : floating_windows)
        window_controller.raise(window->window().value());

    if (state.focused_container() && state.focused_container()->is_fullscreen())
        window_controller.raise(state.focused_container()->window().value());
}

void Workspace::delete_container(std::shared_ptr<Container> const& container)
{
    switch (container->get_type())
    {
    case ContainerType::leaf:
    {
        // TODO: Get the tree for this container
        tree->advise_delete_window(container);
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

void Workspace::advise_focus_gained(std::shared_ptr<Container> const& container)
{
    last_selected_container = container;
}

void Workspace::show()
{
    auto fullscreen_node = tree->show();
    for (auto const& floating : floating_windows)
        floating->show();

    // TODO: ugh that's ugly. Fullscreen nodes should show above floating nodes
    if (fullscreen_node)
    {
        window_controller.select_active_window(fullscreen_node->window().value());
        window_controller.raise(fullscreen_node->window().value());
    }
}

void Workspace::hide()
{
    tree->hide();

    for (auto const& floating : floating_windows)
        floating->hide();
}

void Workspace::for_each_window(std::function<void(std::shared_ptr<Container>)> const& f) const
{
    for (auto const& window : floating_windows)
    {
        if (!window->window())
        {
            mir::log_error("Workspace::for_each_window: floating window has no window");
            continue;
        }

        auto container = window_controller.get_container(window->window().value());
        if (container)
            f(container);
    }

    tree->foreach_node([&](std::shared_ptr<Container> const& node)
    {
        if (auto leaf = Container::as_leaf(node))
        {
            if (!leaf->window())
            {
                mir::log_error("Workspace::for_each_window: tiled window has no window");
                return;
            }

            auto container = window_controller.get_container(leaf->window().value());
            if (container)
                f(container);
        }
    });
}

void Workspace::transfer_pinned_windows_to(std::shared_ptr<Workspace> const& other)
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

bool Workspace::has_floating_window(std::shared_ptr<Container> const& container)
{
    for (auto const& other : floating_windows)
    {
        if (other == container)
            return true;
    }

    return false;
}

std::shared_ptr<FloatingWindowContainer> Workspace::add_floating_window(miral::Window const& window)
{
    auto floating = std::make_shared<FloatingWindowContainer>(
        window, floating_window_manager, window_controller, this, state, config);
    floating_windows.push_back(floating);
    return floating;
}

void Workspace::remove_floating_hack(std::shared_ptr<Container> const& container)
{
    assert(container->get_type() == ContainerType::floating_window);
    Container::as_floating(container)->set_workspace(nullptr);
    floating_windows.erase(
        std::remove(floating_windows.begin(), floating_windows.end(), container), floating_windows.end());
}

void Workspace::select_first_window()
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
            auto found = tree->foreach_node_pred([&](std::shared_ptr<Container> const& container)
            {
                if (container == last_selected)
                {
                    window_controller.select_active_window(container->window().value());
                    return true;
                }

                return false;
            });

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
    auto found = tree->foreach_node_pred([&](std::shared_ptr<Container> const& container)
    {
        if (Container::as_leaf(container))
        {
            window_controller.select_active_window(container->window().value());
            return true;
        }

        return false;
    });

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

Output* Workspace::get_output() const
{
    return output;
}

void Workspace::trigger_rerender()
{
    // TODO: Ugh, sad. I am forced to set the surface transform so that the surface is rerendered
    for_each_window([&](std::shared_ptr<Container> const& container)
    {
        auto window = container->window();
        if (window)
        {
            auto surface = window->operator std::shared_ptr<mir::scene::Surface>();
            if (surface)
                surface->set_transformation(container->get_transform());
        }
    });
}

bool Workspace::is_empty() const
{
    return tree->is_empty() && floating_windows.empty();
}

void Workspace::graft(std::shared_ptr<Container> const& container)
{
    switch (container->get_type())
    {
    case ContainerType::floating_window:
    {
        auto floating = Container::as_floating(container);
        floating->set_workspace(this);
        floating_windows.push_back(floating);
        break;
    }
    case ContainerType::parent:
        tree->graft(Container::as_parent(container));
        break;
    case ContainerType::leaf:
        tree->graft(Container::as_leaf(container));
        break;
    default:
        mir::log_error("Workspace::graft: ungraftable container type: %d", (int)container->get_type());
        break;
    }
}

std::shared_ptr<ParentContainer> Workspace::get_layout_container()
{
    if (!state.focused_container())
        return nullptr;

    auto parent = state.focused_container()->get_parent().lock();
    if (!parent)
        return nullptr;

    if (parent->get_workspace() != this)
        return nullptr;

    return parent;
}

std::string Workspace::display_name() const
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

nlohmann::json Workspace::to_json() const
{
    bool const is_focused = output->active() == this;

    // Note: The reported workspace area appears to be the placement
    // area of the root tree.
    //   See: https://i3wm.org/docs/ipc.html#_tree_reply
    auto area = tree->get_area();

    nlohmann::json floating_nodes = nlohmann::json::array();
    for (auto const& container : floating_windows)
        floating_nodes.push_back(container->to_json());

    nlohmann::json nodes = nlohmann::json::array();
    auto const root = tree->get_root();
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
        { "visible", output->is_active() && is_focused },
        { "focused", output->is_active() && is_focused },
        { "urgent", false },
        { "output", output->get_output().name() },
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

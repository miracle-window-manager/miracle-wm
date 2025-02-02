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

#define MIR_LOG_COMPONENT "command_controller"

#include "command_controller.h"
#include "config.h"
#include "mode_observer.h"
#include "output_manager.h"
#include "parent_container.h"
#include "scratchpad.h"
#include "window_helpers.h"
#include "workspace_manager.h"

#include <mir/log.h>
#include <miral/runner.h>

using namespace miracle;

CommandController::CommandController(
    std::shared_ptr<Config> config,
    std::recursive_mutex& mutex,
    CompositorState& state,
    WindowController& window_controller,
    WorkspaceManager& workspace_manager,
    ModeObserverRegistrar& mode_observer_registrar,
    std::unique_ptr<CommandControllerInterface> interface,
    Scratchpad& scratchpad_,
    OutputManager* output_manager) :
    config { config },
    mutex { mutex },
    state { state },
    window_controller { window_controller },
    workspace_manager { workspace_manager },
    mode_observer_registrar { mode_observer_registrar },
    interface { std::move(interface) },
    scratchpad_ { scratchpad_ },
    output_manager { output_manager }
{
}

void CommandController::try_toggle_resize_mode()
{
    std::lock_guard lock(mutex);
    if (!state.focused_container())
    {
        set_mode(WindowManagerMode::normal);
        return;
    }

    if (state.focused_container()->get_type() != ContainerType::leaf)
    {
        set_mode(WindowManagerMode::normal);
        return;
    }

    if (state.mode() != WindowManagerMode::normal)
        set_mode(WindowManagerMode::normal);
    else
        set_mode(WindowManagerMode::resizing);
}

bool CommandController::try_request_vertical()
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    if (!state.focused_container())
        return false;

    state.focused_container()->request_vertical_layout();
    return true;
}

bool CommandController::try_toggle_layout(bool cycle_thru_all)
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    if (!state.focused_container())
        return false;

    state.focused_container()->toggle_layout(cycle_thru_all);
    return true;
}

bool CommandController::try_request_horizontal()
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    if (!state.focused_container())
        return false;

    state.focused_container()->request_horizontal_layout();
    return true;
}

bool CommandController::try_resize(miracle::Direction direction, int pixels)
{
    std::lock_guard lock(mutex);
    if (!state.focused_container())
        return false;

    return state.focused_container()->resize(direction, pixels);
}

bool CommandController::try_set_size(std::optional<int> const& width, std::optional<int> const& height)
{
    std::lock_guard lock(mutex);
    if (!state.focused_container())
        return false;

    return state.focused_container()->set_size(width, height);
}

bool CommandController::try_move(miracle::Direction direction)
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    if (!state.focused_container())
        return false;

    return state.focused_container()->move(direction);
}

bool CommandController::try_move_by(miracle::Direction direction, int pixels)
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    if (!state.focused_container())
        return false;

    return state.focused_container()->move_by(direction, pixels);
}

bool CommandController::try_move_to(int x, int y)
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    if (!state.focused_container())
        return false;

    return state.focused_container()->move_to(x, y);
}

void CommandController::select_container(std::shared_ptr<Container> const& container)
{
    std::lock_guard lock(mutex);
    if (container->window())
        window_controller.select_active_window(container->window().value());
    else
    {
        window_controller.select_active_window(miral::Window {});
        state.focus_container(container, true);
    }
}

bool CommandController::try_select(miracle::Direction direction)
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    if (!state.focused_container())
        return false;

    return state.focused_container()->select_next(direction);
}

bool CommandController::try_select_parent()
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    if (!state.focused_container())
        return false;

    if (!state.focused_container()->get_parent().expired())
    {
        select_container(state.focused_container()->get_parent().lock());
        return true;
    }
    else
    {
        mir::log_error("try_select_parent: no parent to select");
        return false;
    }
}

bool CommandController::try_select_child()
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    if (!state.focused_container())
        return false;

    if (state.focused_container()->get_type() != ContainerType::parent)
    {
        mir::log_info("CommandController::try_select_child: parent is not selected");
        return false;
    }

    for (auto const& container : state.containers())
    {
        if (!container.expired())
        {
            auto const lock_container = container.lock();
            if (lock_container->get_parent().expired())
                continue;

            if (lock_container->get_parent().lock() == state.focused_container())
                select_container(lock_container);
        }
    }

    if (!state.focused_container()->get_parent().expired())
    {
        state.focus_container(state.focused_container()->get_parent().lock());
        return true;
    }
    else
    {
        mir::log_error("try_select_parent: no parent to select");
        return false;
    }
}

bool CommandController::try_select_floating()
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    // TODO: reimplement
    //    if (auto to_select = state.get_first_with_type(ContainerType::floating_window))
    //    {
    //        if (auto const& window = to_select->window())
    //        {
    //            window_controller.select_active_window(window.value());
    //            return true;
    //        }
    //    }

    return false;
}

bool CommandController::try_select_tiling()
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    if (auto to_select = state.get_first_with_type(ContainerType::leaf))
    {
        if (auto const& window = to_select->window())
        {
            window_controller.select_active_window(window.value());
            return true;
        }
    }

    return false;
}

bool CommandController::try_select_toggle()
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    if (auto const active = state.focused_container())
    {
        if (active->get_type() == ContainerType::leaf)
            return try_select_floating();
        // TODO: reimplement
        //        else if (active->get_type() == ContainerType::floating_window)
        //            return try_select_tiling();
    }

    return false;
}

bool CommandController::try_close_window()
{
    std::lock_guard lock(mutex);
    if (!state.focused_container())
        return false;

    auto window = state.focused_container()->window();
    if (!window)
        return false;

    window_controller.close(window.value());
    return true;
}

bool CommandController::quit()
{
    std::lock_guard lock(mutex);
    interface->quit();
    return true;
}

bool CommandController::try_toggle_fullscreen()
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    if (!state.focused_container())
        return false;

    return state.focused_container()->toggle_fullscreen();
}

bool CommandController::select_workspace(int number, bool back_and_forth)
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    if (!output_manager->focused())
        return false;

    workspace_manager.request_workspace(output_manager->focused(), number, back_and_forth);
    return true;
}

bool CommandController::select_workspace(std::string const& name, bool back_and_forth)
{
    std::lock_guard lock(mutex);
    // TODO: Handle back_and_forth
    if (state.mode() != WindowManagerMode::normal)
        return false;

    return workspace_manager.request_workspace(output_manager->focused(), name, back_and_forth);
}

bool CommandController::next_workspace()
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    workspace_manager.request_next(output_manager->focused());
    return true;
}

bool CommandController::prev_workspace()
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    workspace_manager.request_prev(output_manager->focused());
    return true;
}

bool CommandController::back_and_forth_workspace()
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    workspace_manager.request_back_and_forth();
    return true;
}

bool CommandController::next_workspace_on_output(miracle::Output const& output)
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    return workspace_manager.request_next_on_output(output);
}

bool CommandController::prev_workspace_on_output(miracle::Output const& output)
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    return workspace_manager.request_prev_on_output(output);
}

bool CommandController::move_active_to_workspace(int number, bool back_and_forth)
{
    std::lock_guard lock(mutex);
    if (!can_move_container())
        return false;

    auto container = state.focused_container();
    container->get_output()->delete_container(container);
    state.unfocus_container(container);

    if (workspace_manager.request_workspace(
            output_manager->focused(), number, back_and_forth))
    {
        output_manager->focused()->graft(container);
        if (container->window().value())
            window_controller.select_active_window(container->window().value());
        return true;
    }

    return false;
}

bool CommandController::move_active_to_workspace_named(std::string const& name, bool back_and_forth)
{
    std::lock_guard lock(mutex);
    if (!can_move_container())
        return false;

    auto container = state.focused_container();
    container->get_output()->delete_container(container);
    state.unfocus_container(container);

    if (workspace_manager.request_workspace(output_manager->focused(), name, back_and_forth))
    {
        output_manager->focused()->graft(container);
        return true;
    }

    return false;
}

bool CommandController::move_active_to_next_workspace()
{
    std::lock_guard lock(mutex);
    if (!can_move_container())
        return false;

    auto container = state.focused_container();
    container->get_output()->delete_container(container);
    state.unfocus_container(container);

    if (workspace_manager.request_next(output_manager->focused()))
    {
        output_manager->focused()->graft(container);
        return true;
    }

    return false;
}

bool CommandController::move_active_to_prev_workspace()
{
    std::lock_guard lock(mutex);
    if (!can_move_container())
        return false;

    auto container = state.focused_container();
    container->get_output()->delete_container(container);
    state.unfocus_container(container);

    if (workspace_manager.request_prev(output_manager->focused()))
    {
        output_manager->focused()->graft(container);
        return true;
    }

    return false;
}

bool CommandController::move_active_to_back_and_forth()
{
    std::lock_guard lock(mutex);
    if (!can_move_container())
        return false;

    auto container = state.focused_container();
    container->get_output()->delete_container(container);
    state.unfocus_container(container);

    if (workspace_manager.request_back_and_forth())
    {
        output_manager->focused()->graft(container);
        return true;
    }

    return false;
}

bool CommandController::move_to_scratchpad()
{
    // TODO: reimplement
    //    std::lock_guard lock(mutex);
    //    if (!can_move_container())
    //        return false;
    //
    //    // Only floating or tiled windows can be moved to the scratchpad
    //    auto container = state.focused_container();
    //    if (container->get_type() != ContainerType::leaf)
    //    {
    //        mir::log_error("move_to_scratchpad: cannot move window to scratchpad: %d", static_cast<int>(container->get_type()));
    //        return false;
    //    }
    //
    //    // If the window isn't floating already, we should make it floating
    //    if (container->get_type() != ContainerType::floating_window)
    //    {
    //        if (!output_manager->focused())
    //            return false;
    //
    //        container = toggle_floating_internal(container);
    //    }
    //
    //    // Remove it from its current workspace since it is no longer wanted there
    //    if (auto workspace = container->get_workspace())
    //        workspace->remove_floating_hack(container);
    //
    //    return scratchpad_.move_to(container);
}

bool CommandController::show_scratchpad()
{
    std::lock_guard lock(mutex);
    // TODO: Only show the window that meets the criteria
    return scratchpad_.toggle_show_all();
}

bool CommandController::can_move_container() const
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    if (!state.focused_container())
        return false;

    if (state.focused_container()->is_fullscreen())
        return false;

    return true;
}

std::shared_ptr<Container> CommandController::toggle_floating_internal(std::shared_ptr<Container> const& container)
{
    switch (container->get_type())
    {
    case ContainerType::leaf:
    {
        auto focused_output = output_manager->focused();
        if (!focused_output)
            return nullptr;

        // Walk up the parent tree to get the root node.
        auto parent = container->get_parent();
        if (parent.expired())
            return nullptr;

        while (!parent.lock()->get_parent().expired())
            parent = parent.lock()->get_parent();

        // Remove the container from whatever workspace it is on.
        auto workspace = container->get_workspace();
        workspace->delete_container(container);

        // If the parent is anchored, we move [container] to a new floating tree.
        if (parent.lock()->anchored())
        {
            geom::Rectangle new_area = {
                geom::Point {
                             container->get_logical_area().top_left.x.as_int() + 50,
                             container->get_logical_area().top_left.y.as_int() + 50 },
                geom::Size {
                             container->get_logical_area().size.width,
                             container->get_logical_area().size.height              }
            };
            auto new_parent = workspace->create_floating_tree(new_area);
            new_parent->graft_existing(container, new_parent->num_nodes());
            container->set_workspace(workspace);
            new_parent->commit_changes();
        }
        else
        {
            // Otherwise, we move the container to the root
            workspace->graft(container);
        }
        return container;
    }
    default:
        mir::log_warning("toggle_floating: has no effect on window of type: %d", (int)container->get_type());
        return nullptr;
    }
}

bool CommandController::toggle_floating()
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    if (!state.focused_container())
        return false;

    toggle_floating_internal(state.focused_container());
    return true;
}

bool CommandController::toggle_pinned_to_workspace()
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    if (!state.focused_container())
        return false;

    return state.focused_container()->pinned(!state.focused_container()->pinned());
}

bool CommandController::set_is_pinned(bool pinned)
{
    std::lock_guard lock(mutex);
    if (state.mode() != WindowManagerMode::normal)
        return false;

    if (!state.focused_container())
        return false;

    return state.focused_container()->pinned(pinned);
}

bool CommandController::toggle_tabbing()
{
    std::lock_guard lock(mutex);
    if (!can_set_layout())
        return false;

    return state.focused_container()->toggle_tabbing();
}

bool CommandController::toggle_stacking()
{
    std::lock_guard lock(mutex);
    if (!can_set_layout())
        return false;

    return state.focused_container()->toggle_stacking();
}

bool CommandController::set_layout(LayoutScheme scheme)
{
    std::lock_guard lock(mutex);
    if (!can_set_layout())
        return false;

    return state.focused_container()->set_layout(scheme);
}

bool CommandController::set_layout_default()
{
    std::lock_guard lock(mutex);
    if (!can_set_layout())
        return false;

    return state.focused_container()->set_layout(config->get_default_layout_scheme());
}

void CommandController::move_cursor_to_output(Output const& output)
{
    auto const& extents = output.get_area();
    window_controller.move_cursor_to(
        extents.top_left.x.as_int() + extents.size.width.as_int() / 2.f,
        extents.top_left.y.as_int() + extents.size.height.as_int() / 2.f);
}

bool CommandController::try_select_next_output()
{
    std::lock_guard lock(mutex);
    for (size_t i = 0; i < output_manager->outputs().size(); i++)
    {
        if (output_manager->outputs()[i].get() == output_manager->focused())
        {
            size_t j = i + 1;
            if (j == output_manager->outputs().size())
                j = 0;

            move_cursor_to_output(*output_manager->outputs()[j]);
            return true;
        }
    }

    return false;
}

bool CommandController::try_select_prev_output()
{
    std::lock_guard lock(mutex);
    for (int i = output_manager->outputs().size() - 1; i >= 0; i++)
    {
        if (output_manager->outputs()[i].get() == output_manager->focused())
        {
            size_t j = i - 1;
            if (j < 0)
                j = output_manager->outputs().size() - 1;

            move_cursor_to_output(*output_manager->outputs()[j]);
            return true;
        }
    }

    return false;
}

Output* CommandController::_next_output_in_direction(Direction direction)
{
    auto const& active = output_manager->focused();
    auto const& active_area = active->get_area();
    for (auto const& output : output_manager->outputs())
    {
        if (output.get() == output_manager->focused())
            continue;

        auto const& other_area = output->get_area();
        switch (direction)
        {
        case Direction::left:
        {
            if (active_area.top_left.x.as_int() == (other_area.top_left.x.as_int() + other_area.size.width.as_int()))
            {
                return output.get();
            }
            break;
        }
        case Direction::right:
        {
            if (active_area.top_left.x.as_int() + active_area.size.width.as_int() == other_area.top_left.x.as_int())
            {
                return output.get();
            }
            break;
        }
        case Direction::up:
        {
            if (active_area.top_left.y.as_int() == (other_area.top_left.y.as_int() + other_area.size.height.as_int()))
            {
                return output.get();
            }
            break;
        }
        case Direction::down:
        {
            if (active_area.top_left.y.as_int() + active_area.size.height.as_int() == other_area.top_left.y.as_int())
            {
                return output.get();
            }
            break;
        }
        default:
            return active;
        }
    }

    return active;
}

bool CommandController::try_select_output(Direction direction)
{
    std::lock_guard lock(mutex);
    auto const& next = _next_output_in_direction(direction);
    if (next != output_manager->focused())
    {
        move_cursor_to_output(*next);
        return true;
    }

    return false;
}

Output* CommandController::_next_output_in_list(std::vector<std::string> const& names)
{
    if (names.empty())
        return output_manager->focused();

    auto current_name = output_manager->focused()->name();
    size_t next = 0;
    for (size_t i = 0; i < names.size(); i++)
    {
        if (names[i] == current_name)
        {
            next = i + 1;
            break;
        }
    }

    if (next == names.size())
        next = 0;

    for (auto const& output : output_manager->outputs())
    {
        if (output->name() == names[next])
            return output.get();
    }

    return output_manager->focused();
}

bool CommandController::try_select_output(std::vector<std::string> const& names)
{
    std::lock_guard lock(mutex);
    if (!output_manager->focused())
        return false;

    auto const& output = _next_output_in_list(names);
    if (output != output_manager->focused())
        move_cursor_to_output(*output);
    return true;
}

bool CommandController::try_move_active_to_output(miracle::Direction direction)
{
    std::lock_guard lock(mutex);
    if (!output_manager->focused())
        return false;

    if (!can_move_container())
        return false;

    auto const& next = _next_output_in_direction(direction);
    if (next != output_manager->focused())
    {
        auto container = state.focused_container();
        container->get_output()->delete_container(container);
        state.unfocus_container(container);

        next->graft(container);
        if (container->window().value())
            window_controller.select_active_window(container->window().value());
        return true;
    }

    return false;
}

bool CommandController::try_move_active_to_current()
{
    std::lock_guard lock(mutex);
    if (!output_manager->focused())
        return false;

    if (!can_move_container())
        return false;

    if (state.focused_container()->get_output() == output_manager->focused())
        return false;

    auto container = state.focused_container();
    container->get_output()->delete_container(container);
    state.unfocus_container(container);

    output_manager->focused()->graft(container);
    if (container->window().value())
        window_controller.select_active_window(container->window().value());
    return true;
}

bool CommandController::try_move_active_to_primary()
{
    std::lock_guard lock(mutex);
    if (output_manager->outputs().empty())
        return false;

    if (!can_move_container())
        return false;

    if (state.focused_container()->get_output() == output_manager->outputs()[0].get())
        return false;

    auto container = state.focused_container();
    container->get_output()->delete_container(container);
    state.unfocus_container(container);

    output_manager->outputs()[0]->graft(container);
    if (container->window().value())
        window_controller.select_active_window(container->window().value());
    return true;
}

bool CommandController::try_move_active_to_nonprimary()
{
    std::lock_guard lock(mutex);
    constexpr int MIN_SIZE_TO_HAVE_NONPRIMARY_OUTPUT = 2;
    if (output_manager->outputs().size() < MIN_SIZE_TO_HAVE_NONPRIMARY_OUTPUT)
        return false;

    if (!can_move_container())
        return false;

    if (output_manager->focused() != output_manager->outputs()[0].get())
        return false;

    auto container = state.focused_container();
    container->get_output()->delete_container(container);
    state.unfocus_container(container);

    output_manager->outputs()[1]->graft(container);
    if (container->window().value())
        window_controller.select_active_window(container->window().value());
    return true;
}

bool CommandController::try_move_active_to_next()
{
    std::lock_guard lock(mutex);
    if (!can_move_container())
        return false;

    auto it = std::find_if(output_manager->outputs().begin(), output_manager->outputs().end(), [output_manager = output_manager](std::unique_ptr<Output> const& output)
    {
        return output.get() == output_manager->focused();
    });

    if (it == output_manager->outputs().end())
    {
        mir::log_error("CommandController::try_move_active_to_next: cannot find active output in list");
        return false;
    }

    it++;
    if (it == output_manager->outputs().end())
        it = output_manager->outputs().begin();

    if (it->get() == output_manager->focused())
        return false;

    if ((*it).get() == state.focused_container()->get_output())
        return false;

    auto container = state.focused_container();
    container->get_output()->delete_container(container);
    state.unfocus_container(container);

    (*it)->graft(container);
    if (container->window().value())
        window_controller.select_active_window(container->window().value());
    return true;
}

bool CommandController::try_move_active(std::vector<std::string> const& names)
{
    std::lock_guard lock(mutex);
    if (!can_move_container())
        return false;

    auto const& output = _next_output_in_list(names);
    if (output != state.focused_container()->get_output())
    {
        auto container = state.focused_container();
        container->get_output()->delete_container(container);
        state.unfocus_container(container);

        output->graft(container);
        if (container->window().value())
            window_controller.select_active_window(container->window().value());
    }

    return true;
}

bool CommandController::can_set_layout() const
{
    if (state.mode() != WindowManagerMode::normal)
        return false;

    if (!state.focused_container())
        return false;

    return !state.focused_container()->is_fullscreen();
}

bool CommandController::reload_config()
{
    std::lock_guard lock(mutex);
    config->reload();
    return true;
}

void CommandController::set_mode(WindowManagerMode mode)
{
    state.mode(mode);
    mode_observer_registrar.advise_changed(state.mode());
}

nlohmann::json CommandController::to_json() const
{
    std::lock_guard lock(mutex);
    geom::Point top_left { INT_MAX, INT_MAX };
    geom::Point bottom_right { 0, 0 };
    nlohmann::json outputs_json = nlohmann::json::array();
    for (auto const& output : output_manager->outputs())
    {
        if (output->is_defunct())
            continue;

        auto& area = output->get_area();

        // Recalculate the total extents of the tree
        if (area.top_left.x.as_int() < top_left.x.as_int())
            top_left.x = geom::X { area.top_left.x.as_int() };
        if (area.top_left.y.as_int() < top_left.y.as_int())
            top_left.y = geom::Y { area.top_left.y.as_int() };

        int bottom_x = area.top_left.x.as_int() + area.size.width.as_int();
        int bottom_y = area.top_left.y.as_int() + area.size.height.as_int();
        if (bottom_x > bottom_right.x.as_int())
            bottom_right.x = geom::X { bottom_x };
        if (bottom_y > bottom_right.y.as_int())
            bottom_right.y = geom::Y { bottom_y };

        outputs_json.push_back(output->to_json());
    }

    geom::Rectangle total_area {
        top_left,
        geom::Size {
                    geom::Width(bottom_right.x.as_int() - top_left.x.as_int()),
                    geom::Height(bottom_right.y.as_int() - top_left.y.as_int()) }
    };
    nlohmann::json root = {
        { "id", 0 },
        { "name", "root" },
        {
         "rect",
         { { "x", total_area.top_left.x.as_int() }, { "y", total_area.top_left.y.as_int() }, { "width", total_area.size.width.as_int() }, { "height", total_area.size.height.as_int() } },
         },
        { "nodes", outputs_json },
        { "type", "root" }
    };
    return root;
}

nlohmann::json CommandController::outputs_json() const
{
    std::lock_guard lock(mutex);
    nlohmann::json j = nlohmann::json::array();
    for (auto const& output : output_manager->outputs())
    {
        if (output->is_defunct())
            continue;

        j.push_back(output->to_json());
    }
    return j;
}

nlohmann::json CommandController::workspaces_json() const
{
    std::lock_guard lock(mutex);
    nlohmann::json j = nlohmann::json::array();
    for (auto workspace : workspace_manager.workspaces())
    {
        if (workspace->get_output()->is_defunct())
            continue;

        j.push_back(workspace->to_json());
    }
    return j;
}

nlohmann::json CommandController::workspace_to_json(uint32_t id) const
{
    std::lock_guard lock(mutex);
    auto workspace = workspace_manager.workspace(id);
    return workspace->to_json();
}

nlohmann::json CommandController::mode_to_json() const
{
    std::lock_guard lock(mutex);
    switch (state.mode())
    {
    case WindowManagerMode::normal:
        return {
            { "name", "default" }
        };
    case WindowManagerMode::resizing:
        return {
            { "name", "resize" }
        };
    case WindowManagerMode::selecting:
        return {
            { "name", "selecting" }
        };
    case WindowManagerMode::dragging:
        return {
            { "name", "dragging" }
        };
    case WindowManagerMode::moving:
        return {
            { "name", "moving" }
        };
    default:
    {
        mir::fatal_error("handle_command: unknown binding state: %d", (int)state.mode());
        return {};
    }
    }
}
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

#define MIR_LOG_COMPONENT "miracle"

#include "policy.h"
#include "animator_loop.h"
#include "config.h"
#include "constants.h"
#include "container_group_container.h"
#include "feature_flags.h"
#include "floating_tree_container.h"
#include "miral_output_factory.h"
#include "output_manager.h"
#include "parent_container.h"
#include "shell_component_container.h"
#include "window_helpers.h"
#include "window_tools_accessor.h"
#include "workspace_manager.h"

#include <iostream>
#include <mir/geometry/rectangle.h>
#include <mir/log.h>
#include <mir/server.h>
#include <mir_toolkit/events/enums.h>
#include <miral/application_info.h>
#include <miral/runner.h>
#include <miral/toolkit_event.h>
#include <miral/window_specification.h>
#include <miral/zone.h>
#include <mutex>

using namespace miracle;

namespace
{
class MirRunnerCommandControllerInterface : public CommandControllerInterface
{
public:
    explicit MirRunnerCommandControllerInterface(miral::MirRunner& runner) :
        runner { runner }
    {
    }

    void quit() override
    {
        runner.stop();
    }

private:
    miral::MirRunner& runner;
};
}

class Policy::Self : public WorkspaceObserver
{
public:
    explicit Self(Policy& policy) :
        policy { policy }
    {
    }

    void on_created(uint32_t) override { }
    void on_removed(uint32_t) override { }
    void on_focused(std::optional<uint32_t> old, uint32_t next) override
    {
        if (old)
        {
            auto const& last_workspace = policy.workspace_manager.workspace(old.value());
            if (!last_workspace)
            {
                mir::log_error("Policy::Self::on_focused missing last workspace");
                return;
            }

            auto const& next_workspace = policy.workspace_manager.workspace(next);
            if (!next_workspace)
            {
                mir::log_error("Policy::Self::on_focused missing next workspace");
                return;
            }

            if (last_workspace->get_output() != next_workspace->get_output())
                policy.command_controller.move_cursor_to_output(*next_workspace->get_output());
        }
    }

    Policy& policy;
    std::recursive_mutex mutex;
};

Policy::Policy(
    miral::WindowManagerTools const& tools,
    AutoRestartingLauncher& external_client_launcher,
    miral::MirRunner& runner,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<Animator> const& animator,
    mir::Server const& server,
    CompositorState& compositor_state,
    std::shared_ptr<WindowToolsAccessor> const& window_tools_accessor) :
    external_client_launcher { external_client_launcher },
    config { config },
    animator { animator },
    state { compositor_state },
    floating_window_manager(std::make_unique<MinimalWindowManager>(tools, config)),
    animator_loop(std::make_unique<ThreadedAnimatorLoop>(animator)),
    output_manager(std::make_unique<OutputManager>(
        std::make_unique<MiralOutputFactory>(
            floating_window_manager,
            state,
            config,
            window_controller,
            *animator))),
    workspace_manager(workspace_observer_registrar, config, output_manager.get()),
    window_controller(tools, *animator, state, config, server.the_main_loop(), this),
    scratchpad_(window_controller, output_manager.get()),
    self(std::make_shared<Self>(*this)),
    command_controller(
        config, self->mutex, state, window_controller,
        workspace_manager, mode_observer_registrar,
        std::make_unique<MirRunnerCommandControllerInterface>(runner), scratchpad_, output_manager.get()),
    drag_and_drop_service(command_controller, config, output_manager.get()),
    i3_command_executor(command_controller, output_manager.get(), workspace_manager, compositor_state, external_client_launcher, window_controller),
    ipc(std::make_shared<Ipc>(runner, command_controller, i3_command_executor, config))
{
    workspace_observer_registrar.register_interest(ipc);
    workspace_observer_registrar.register_interest(self);
    mode_observer_registrar.register_interest(ipc);
    window_tools_accessor->set_tools(tools);
    animator_loop->start();

    auto output = output_manager->create(
        "default",
        -1,
        mir::geometry::Rectangle(mir::geometry::Point(0, 0), mir::geometry::Size(1920, 1280)));
    output->set_defunct();
    workspace_manager.request_first_available_workspace(output);
}

Policy::~Policy()
{
    ipc->on_shutdown();
    animator_loop->stop();
    workspace_observer_registrar.unregister_interest(ipc.get());
    workspace_observer_registrar.unregister_interest(self.get());
    mode_observer_registrar.unregister_interest(ipc.get());
}

bool Policy::handle_keyboard_event(MirKeyboardEvent const* event)
{
    auto const action = miral::toolkit::mir_keyboard_event_action(event);
    auto const scan_code = miral::toolkit::mir_keyboard_event_scan_code(event);
    auto const modifiers = miral::toolkit::mir_keyboard_event_modifiers(event) & MODIFIER_MASK;
    state.modifiers = modifiers;

    auto custom_key_command = config->matches_custom_key_command(action, scan_code, modifiers);
    if (custom_key_command != nullptr)
    {
        external_client_launcher.launch({ custom_key_command->command });
        return true;
    }

    return config->matches_key_command(action, scan_code, modifiers, [&](DefaultKeyCommand key_command)
    {
        if (key_command == DefaultKeyCommand::MAX)
            return false;

        switch (key_command)
        {
        case DefaultKeyCommand::Terminal:
        {
            auto terminal_command = config->get_terminal_command();
            if (terminal_command)
                external_client_launcher.launch({ terminal_command.value() });
            return true;
        }
        case DefaultKeyCommand::RequestVertical:
            return command_controller.try_request_vertical();
        case DefaultKeyCommand::RequestHorizontal:
            return command_controller.try_request_horizontal();
        case DefaultKeyCommand::ToggleResize:
            command_controller.try_toggle_resize_mode();
            return true;
        case DefaultKeyCommand::ResizeUp:
            return state.mode() != WindowManagerMode::normal && command_controller.try_resize(Direction::up, config->get_resize_jump());
        case DefaultKeyCommand::ResizeDown:
            return state.mode() != WindowManagerMode::normal && command_controller.try_resize(Direction::down, config->get_resize_jump());
        case DefaultKeyCommand::ResizeLeft:
            return state.mode() != WindowManagerMode::normal && command_controller.try_resize(Direction::left, config->get_resize_jump());
        case DefaultKeyCommand::ResizeRight:
            return state.mode() != WindowManagerMode::normal && command_controller.try_resize(Direction::right, config->get_resize_jump());
        case DefaultKeyCommand::MoveUp:
            return command_controller.try_move(Direction::up);
        case DefaultKeyCommand::MoveDown:
            return command_controller.try_move(Direction::down);
        case DefaultKeyCommand::MoveLeft:
            return command_controller.try_move(Direction::left);
        case DefaultKeyCommand::MoveRight:
            return command_controller.try_move(Direction::right);
        case DefaultKeyCommand::SelectUp:
            return command_controller.try_select(Direction::up);
        case DefaultKeyCommand::SelectDown:
            return command_controller.try_select(Direction::down);
        case DefaultKeyCommand::SelectLeft:
            return command_controller.try_select(Direction::left);
        case DefaultKeyCommand::SelectRight:
            return command_controller.try_select(Direction::right);
        case DefaultKeyCommand::QuitActiveWindow:
            return command_controller.try_close_window();
        case DefaultKeyCommand::QuitCompositor:
            return command_controller.quit();
        case DefaultKeyCommand::Fullscreen:
            return command_controller.try_toggle_fullscreen();
        case DefaultKeyCommand::SelectWorkspace1:
            return command_controller.select_workspace(1);
        case DefaultKeyCommand::SelectWorkspace2:
            return command_controller.select_workspace(2);
        case DefaultKeyCommand::SelectWorkspace3:
            return command_controller.select_workspace(3);
        case DefaultKeyCommand::SelectWorkspace4:
            return command_controller.select_workspace(4);
        case DefaultKeyCommand::SelectWorkspace5:
            return command_controller.select_workspace(5);
        case DefaultKeyCommand::SelectWorkspace6:
            return command_controller.select_workspace(6);
        case DefaultKeyCommand::SelectWorkspace7:
            return command_controller.select_workspace(7);
        case DefaultKeyCommand::SelectWorkspace8:
            return command_controller.select_workspace(8);
        case DefaultKeyCommand::SelectWorkspace9:
            return command_controller.select_workspace(9);
        case DefaultKeyCommand::SelectWorkspace0:
            return command_controller.select_workspace(0);
        case DefaultKeyCommand::MoveToWorkspace1:
            return command_controller.move_active_to_workspace(1);
        case DefaultKeyCommand::MoveToWorkspace2:
            return command_controller.move_active_to_workspace(2);
        case DefaultKeyCommand::MoveToWorkspace3:
            return command_controller.move_active_to_workspace(3);
        case DefaultKeyCommand::MoveToWorkspace4:
            return command_controller.move_active_to_workspace(4);
        case DefaultKeyCommand::MoveToWorkspace5:
            return command_controller.move_active_to_workspace(5);
        case DefaultKeyCommand::MoveToWorkspace6:
            return command_controller.move_active_to_workspace(6);
        case DefaultKeyCommand::MoveToWorkspace7:
            return command_controller.move_active_to_workspace(7);
        case DefaultKeyCommand::MoveToWorkspace8:
            return command_controller.move_active_to_workspace(8);
        case DefaultKeyCommand::MoveToWorkspace9:
            return command_controller.move_active_to_workspace(9);
        case DefaultKeyCommand::MoveToWorkspace0:
            return command_controller.move_active_to_workspace(0);
        case DefaultKeyCommand::ToggleFloating:
            return command_controller.toggle_floating();
        case DefaultKeyCommand::TogglePinnedToWorkspace:
            return command_controller.toggle_pinned_to_workspace();
        case DefaultKeyCommand::ToggleTabbing:
            return command_controller.toggle_tabbing();
        case DefaultKeyCommand::ToggleStacking:
            return command_controller.toggle_stacking();
        default:
            std::cerr << "Unknown key_command: " << static_cast<int>(key_command) << std::endl;
            break;
        }
        return false;
    });
}

bool Policy::handle_pointer_event(MirPointerEvent const* event)
{
    std::lock_guard lock(self->mutex);
    auto x = miral::toolkit::mir_pointer_event_axis_value(event, MirPointerAxis::mir_pointer_axis_x);
    auto y = miral::toolkit::mir_pointer_event_axis_value(event, MirPointerAxis::mir_pointer_axis_y);
    auto action = miral::toolkit::mir_pointer_event_action(event);
    auto const modifiers = miral::toolkit::mir_pointer_event_modifiers(event) & MODIFIER_MASK;
    state.cursor_position = { x, y };

    // Select the output first
    for (auto const& output : output_manager->outputs())
    {
        if (output->point_is_in_output(static_cast<int>(x), static_cast<int>(y)))
        {
            if (output_manager->focused() != output.get())
            {
                if (output_manager->focused())
                    output_manager->unfocus(output_manager->focused()->id());
                output_manager->focus(output->id());
                if (auto active = output->active())
                    workspace_manager.request_focus(active->id());
            }
            break;
        }
    }

    if (drag_and_drop_service.handle_pointer_event(state, x, y, action, modifiers))
        return true;

    if (output_manager->focused() && state.mode() != WindowManagerMode::resizing)
    {
        if (MIRACLE_FEATURE_FLAG_MULTI_SELECT && action == mir_pointer_action_button_down)
        {
            if (state.modifiers == config->get_primary_modifier())
            {
                // We clicked while holding the modifier, so we're probably in the middle of a multi-selection.
                if (state.mode() != WindowManagerMode::selecting)
                {
                    command_controller.set_mode(WindowManagerMode::selecting);
                    group_selection = std::make_shared<ContainerGroupContainer>(state);
                    state.add(group_selection);
                }
            }
            else if (state.mode() == WindowManagerMode::selecting)
            {
                // We clicked while we were in selection mode, so let's stop being in selection mode
                // TODO: Would it be better to check what we clicked in case it's in the group? Then we wouldn't
                //  exit selection mode in this case.
                command_controller.set_mode(WindowManagerMode::normal);
            }
        }

        // Get Container intersection. Depending on the state, do something with that Container
        std::shared_ptr<Container> intersected = output_manager->focused()->intersect(x, y);
        switch (state.mode())
        {
        case WindowManagerMode::normal:
        {
            if (intersected)
            {
                if (auto window = intersected->window().value())
                {
                    if (state.focused_container() != intersected)
                        window_controller.select_active_window(window);
                }
            }

            if (state.has_clicked_floating_window || (state.focused_container() && state.focused_container()->get_type() == ContainerType::floating_window))
            {
                if (action == mir_pointer_action_button_down)
                    state.has_clicked_floating_window = true;
                else if (action == mir_pointer_action_button_up)
                    state.has_clicked_floating_window = false;
                return floating_window_manager->handle_pointer_event(event);
            }

            return false;
        }
        case WindowManagerMode::selecting:
        {
            if (intersected && action == mir_pointer_action_button_down)
                group_selection->add(intersected);
            return true;
        }
        default:
            return false;
        }
    }

    return false;
}

auto Policy::place_new_window(
    const miral::ApplicationInfo& app_info,
    const miral::WindowSpecification& requested_specification) -> miral::WindowSpecification
{
    std::lock_guard lock(self->mutex);
    if (!output_manager->focused())
    {
        mir::log_warning("place_new_window: no output available");
        return requested_specification;
    }

    auto new_spec = requested_specification;
    pending_allocation = output_manager->focused()->allocate_position(app_info, new_spec, {});
    return new_spec;
}

void Policy::advise_new_window(miral::WindowInfo const& window_info)
{
    std::lock_guard lock(self->mutex);
    if (!output_manager->focused())
        mir::fatal_error("create_container: an output should always be available");

    auto container = output_manager->focused()->create_container(window_info, pending_allocation);
    container->animation_handle(animator->register_animateable());
    container->on_open();
    state.add(container);

    pending_allocation.container_type = ContainerType::none;
}

void Policy::handle_window_ready(miral::WindowInfo& window_info)
{
    std::lock_guard lock(self->mutex);
    auto container = window_controller.get_container(window_info.window());
    if (!container)
    {
        mir::log_error("handle_window_ready: container is not provided");
        return;
    }

    container->handle_ready();
}

mir::geometry::Rectangle
Policy::confirm_placement_on_display(
    miral::WindowInfo const& window_info,
    MirWindowState new_state,
    mir::geometry::Rectangle const& new_placement)
{
    std::lock_guard lock(self->mutex);
    auto container = window_controller.get_container(window_info.window());
    if (!container)
    {
        mir::log_warning("confirm_placement_on_display: window lacks container");
        return new_placement;
    }

    return container->confirm_placement(new_state, new_placement);
}

void Policy::advise_focus_gained(const miral::WindowInfo& window_info)
{
    std::lock_guard lock(self->mutex);
    auto container = window_controller.get_container(window_info.window());
    if (!container)
    {
        mir::log_error("advise_focus_gained: container is not provided");
        return;
    }

    switch (state.mode())
    {
    case WindowManagerMode::selecting:
        group_selection->add(container);
        container->on_focus_gained();
        break;
    default:
    {
        auto* workspace = container->get_workspace();
        state.focus_container(container);
        container->on_focus_gained();

        // TODO: This logic was put in place to navigate to the focused
        //  workspace.
        // if (workspace && workspace != output_manager->focused()->active())
        //    workspace_manager.request_focus(workspace->id());

        if (workspace)
            workspace->advise_focus_gained(container);
        break;
    }
    }
}

void Policy::advise_focus_lost(const miral::WindowInfo& window_info)
{
    std::lock_guard lock(self->mutex);
    auto container = window_controller.get_container(window_info.window());
    if (!container)
    {
        mir::log_error("advise_focus_lost: container is not provided");
        return;
    }

    if (state.mode() == WindowManagerMode::dragging)
    {
        command_controller.set_mode(WindowManagerMode::normal);
        if (state.focused_container())
            state.focused_container()->drag_stop();
    }

    state.unfocus_container(container);
    container->on_focus_lost();
}

void Policy::advise_delete_window(const miral::WindowInfo& window_info)
{
    std::lock_guard lock(self->mutex);
    auto container = window_controller.get_container(window_info.window());
    if (!container)
    {
        mir::log_error("delete_container: container is not provided");
        return;
    }

    if (auto output = container->get_output())
        output->delete_container(container);
    else
        scratchpad_.remove(container);

    animator->remove_by_animation_handle(container->animation_handle());
    if (container == state.focused_container())
        state.unfocus_container(container);

    state.remove(container);
}

void Policy::advise_move_to(miral::WindowInfo const& window_info, geom::Point top_left)
{
    std::lock_guard lock(self->mutex);
    auto container = window_controller.get_container(window_info.window());
    if (!container)
    {
        mir::log_error("advise_move_to: container is not provided: %s", window_info.application_id().c_str());
        return;
    }

    container->on_move_to(top_left);
}

void Policy::advise_output_create(miral::Output const& output)
{
    std::lock_guard lock(self->mutex);
    auto prev_size = output_manager->outputs().size();
    auto created = output_manager->create(output.name(), output.id(), output.extents());
    if (prev_size < output_manager->outputs().size())
        workspace_manager.request_first_available_workspace(created);
}

void Policy::advise_output_update(miral::Output const& updated, miral::Output const& original)
{
    std::lock_guard lock(self->mutex);
    output_manager->update(updated.id(), updated.extents());
}

void Policy::advise_output_delete(miral::Output const& output)
{
    std::lock_guard lock(self->mutex);
    output_manager->remove(output.id());
}

void Policy::handle_modify_window(
    miral::WindowInfo& window_info,
    const miral::WindowSpecification& modifications)
{
    std::lock_guard lock(self->mutex);
    auto container = window_controller.get_container(window_info.window());
    if (!container)
    {
        mir::log_error("handle_modify_window: container is not provided");
        return;
    }

    auto const* workspace = container->get_workspace();
    if (workspace)
    {
        if (workspace != output_manager->focused()->active())
            return;
    }
    else if (scratchpad_.contains(container) && !scratchpad_.is_showing(container))
        return;

    container->handle_modify(modifications);
}

void Policy::handle_raise_window(miral::WindowInfo& window_info)
{
    std::lock_guard lock(self->mutex);
    auto container = window_controller.get_container(window_info.window());
    if (!container)
    {
        mir::log_error("handle_raise_window: container is not provided");
        return;
    }

    container->handle_raise();
}

bool Policy::handle_touch_event(const MirTouchEvent* event)
{
    return false;
}

void Policy::handle_request_move(miral::WindowInfo& window_info, const MirInputEvent* input_event)
{
    std::lock_guard lock(self->mutex);
    auto container = window_controller.get_container(window_info.window());
    if (!container)
    {
        mir::log_error("handle_request_move: window lacks container");
        return;
    }

    container->handle_request_move(input_event);
}

void Policy::handle_request_resize(
    miral::WindowInfo& window_info,
    const MirInputEvent* input_event,
    MirResizeEdge edge)
{
    std::lock_guard lock(self->mutex);
    auto container = window_controller.get_container(window_info.window());
    if (!container)
    {
        mir::log_error("handle_request_resize: window lacks container");
        return;
    }

    container->handle_request_resize(input_event, edge);
}

void Policy::handle_animation(
    AnimationStepResult const& asr,
    std::weak_ptr<Container> const& container)
{
    std::lock_guard lock(self->mutex);
    auto sh_container = container.lock();
    if (!sh_container)
    {
        mir::log_error("handle_animation: container is invalid");
        return;
    }

    window_controller.process_animation(asr, sh_container);
}

mir::geometry::Rectangle Policy::confirm_inherited_move(
    const miral::WindowInfo& window_info,
    mir::geometry::Displacement movement)
{
    return { window_info.window().top_left() + movement, window_info.window().size() };
}

void Policy::advise_application_zone_create(miral::Zone const& application_zone)
{
    std::lock_guard lock(self->mutex);
    for (auto const& output : output_manager->outputs())
    {
        output->advise_application_zone_create(application_zone);
    }
}

void Policy::advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original)
{
    std::lock_guard lock(self->mutex);
    for (auto const& output : output_manager->outputs())
    {
        output->advise_application_zone_update(updated, original);
    }
}

void Policy::advise_application_zone_delete(miral::Zone const& application_zone)
{
    std::lock_guard lock(self->mutex);
    for (auto const& output : output_manager->outputs())
    {
        output->advise_application_zone_delete(application_zone);
    }
}

void Policy::advise_end()
{
    if (is_starting_)
    {
        is_starting_ = false;
        for (auto const& app : config->get_startup_apps())
        {
            external_client_launcher.launch(app);
        }
    }
}

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

#include "i3_command_executor.h"
#include "auto_restarting_launcher.h"
#include "direction.h"
#include "i3_command.h"
#include "leaf_container.h"
#include "parent_container.h"
#include "policy.h"
#include "utility_general.h"
#include "window_controller.h"
#include "window_helpers.h"

#define MIR_LOG_COMPONENT "miracle"
#include <mir/log.h>
#include <miral/application_info.h>

using namespace miracle;

I3CommandExecutor::I3CommandExecutor(
    miracle::Policy& policy,
    WorkspaceManager& workspace_manager,
    miral::WindowManagerTools const& tools,
    AutoRestartingLauncher& launcher,
    WindowController& window_controller) :
    policy { policy },
    workspace_manager { workspace_manager },
    tools { tools },
    launcher { launcher },
    window_controller { window_controller }
{
}

void I3CommandExecutor::process(miracle::I3ScopedCommandList const& command_list)
{
    for (auto const& command : command_list.commands)
    {
        switch (command.type)
        {
        case I3CommandType::exec:
            process_exec(command, command_list);
            break;
        case I3CommandType::split:
            process_split(command, command_list);
            break;
        case I3CommandType::focus:
            process_focus(command, command_list);
            break;
        case I3CommandType::move:
            process_move(command, command_list);
            break;
        case I3CommandType::sticky:
            process_sticky(command, command_list);
            break;
        case I3CommandType::exit:
            policy.quit();
            break;
        case I3CommandType::input:
            process_input(command, command_list);
            break;
        case I3CommandType::workspace:
            process_workspace(command, command_list);
            break;
        case I3CommandType::layout:
            process_layout(command, command_list);
            break;
        default:
            break;
        }
    }
}

miral::Window I3CommandExecutor::get_window_meeting_criteria(I3ScopedCommandList const& command_list)
{
    miral::Window result;
    tools.find_application([&](miral::ApplicationInfo const& info)
    {
        for (auto const& window : info.windows())
        {
            if (command_list.meets_criteria(window, window_controller))
            {
                result = window;
                return true;
            }
        }

        return false;
    });
    return result;
}

void I3CommandExecutor::process_exec(miracle::I3Command const& command, miracle::I3ScopedCommandList const& command_list)
{
    if (command.arguments.empty())
    {
        mir::log_warning("process_exec: no arguments were supplied");
        return;
    }

    bool no_startup_id = false;
    if (!command.options.empty() && command.options[0] == "--no-startup-id")
        no_startup_id = true;

    if (command.arguments.empty())
    {
        mir::log_warning("process_exec: argument does not have a command to run");
        return;
    }

    std::string exec_cmd;
    for (auto const& arg : command.arguments)
    {
        exec_cmd += arg + " ";
    }

    StartupApp app { exec_cmd, false, no_startup_id };
    launcher.launch(app);
}

void I3CommandExecutor::process_split(miracle::I3Command const& command, miracle::I3ScopedCommandList const& command_list)
{
    if (command.arguments.empty())
    {
        mir::log_warning("process_split: no arguments were supplied");
        return;
    }

    if (command.arguments.front() == "vertical")
    {
        policy.try_request_vertical();
    }
    else if (command.arguments.front() == "horizontal")
    {
        policy.try_request_horizontal();
    }
    else if (command.arguments.front() == "toggle")
    {
        policy.try_toggle_layout(false);
    }
    else
    {
        mir::log_warning("process_split: unknown argument %s", command.arguments.front().c_str());
        return;
    }
}

void I3CommandExecutor::process_focus(I3Command const& command, I3ScopedCommandList const& command_list)
{
    // https://i3wm.org/docs/userguide.html#_focusing_moving_containers
    if (command.arguments.empty())
    {
        if (command_list.scope.empty())
        {
            mir::log_warning("Focus command expected scope but none was provided");
            return;
        }

        auto window = get_window_meeting_criteria(command_list);
        if (window)
            window_controller.select_active_window(window);

        return;
    }

    auto const& arg = command.arguments.front();
    if (arg == "workspace")
    {
        if (command_list.scope.empty())
        {
            mir::log_warning("Focus 'workspace' command expected scope but none was provided");
            return;
        }

        auto window = get_window_meeting_criteria(command_list);
        auto container = window_controller.get_container(window);
        if (container)
            workspace_manager.request_focus(container->get_workspace()->get_workspace());
    }
    else if (arg == "left")
        policy.try_select(Direction::left);
    else if (arg == "right")
        policy.try_select(Direction::right);
    else if (arg == "up")
        policy.try_select(Direction::up);
    else if (arg == "down")
        policy.try_select(Direction::down);
    else if (arg == "parent")
        mir::log_warning("'focus parent' is not supported, see https://github.com/mattkae/miracle-wm/issues/117"); // TODO
    else if (arg == "child")
        mir::log_warning("'focus child' is not supported, see https://github.com/mattkae/miracle-wm/issues/117"); // TODO
    else if (arg == "prev")
    {
        auto active_window = tools.active_window();
        if (!active_window)
            return;

        auto container = window_controller.get_container(active_window);
        if (!container)
            return;

        if (container->get_type() != ContainerType::leaf)
        {
            mir::log_warning("Cannot focus prev when a tiling window is not selected");
            return;
        }

        if (auto parent = Container::as_parent(container->get_parent().lock()))
        {
            auto index = parent->get_index_of_node(container);
            if (index != 0)
            {
                auto node_to_select = parent->get_nth_window(index - 1);
                window_controller.select_active_window(node_to_select->window().value());
            }
        }
    }
    else if (arg == "next")
    {
        auto active_window = tools.active_window();
        if (!active_window)
            return;

        auto container = window_controller.get_container(active_window);
        if (!container)
            return;

        if (container->get_type() != ContainerType::leaf)
        {
            mir::log_warning("Cannot focus prev when a tiling window is not selected");
            return;
        }

        if (auto parent = Container::as_parent(container->get_parent().lock()))
        {
            auto index = parent->get_index_of_node(container);
            if (index != parent->num_nodes() - 1)
            {
                auto node_to_select = parent->get_nth_window(index + 1);
                window_controller.select_active_window(node_to_select->window().value());
            }
        }
    }
    else if (arg == "floating")
        mir::log_warning("'focus floating' is not supported, see https://github.com/mattkae/miracle-wm/issues/117"); // TODO
    else if (arg == "tiling")
        mir::log_warning("'focus tiling' is not supported, see https://github.com/mattkae/miracle-wm/issues/117"); // TODO
    else if (arg == "mode_toggle")
        mir::log_warning("'focus mode_toggle' is not supported, see https://github.com/mattkae/miracle-wm/issues/117"); // TODO
    else if (arg == "output")
        mir::log_warning("'focus output' is not supported, see https://github.com/canonical/mir/issues/3357"); // TODO
}

namespace
{
bool parse_move_distance(std::vector<std::string> const& arguments, int& index, int total_size, int& out)
{
    auto size = arguments.size() - index;
    if (size <= 1)
        return false;

    try
    {
        out = std::stoi(arguments[index]);
        if (size == 2)
        {
            // We default to assuming the value is in pixels
            if (arguments[index + 1] == "ppt")
            {
                float ppt = static_cast<float>(out) / 100.f;
                out = (float)total_size * ppt;
            }
        }

        return true;
    }
    catch (std::invalid_argument const& e)
    {
        mir::log_error("Invalid argument: %s", arguments[index].c_str());
        return false;
    }
}
}

void I3CommandExecutor::process_move(I3Command const& command, I3ScopedCommandList const& command_list)
{
    auto active_output = policy.get_active_output();
    if (!active_output)
    {
        mir::log_warning("process_move: output is not set");
        return;
    }

    // https://i3wm.org/docs/userguide.html#_focusing_moving_containers
    if (command.arguments.empty())
    {
        mir::log_warning("process_move: move command expects arguments");
        return;
    }

    int index = 0;
    auto const& arg0 = command.arguments[index++];
    Direction direction = Direction::MAX;
    int total_size = 0;
    if (arg0 == "left")
    {
        direction = Direction::left;
        total_size = active_output->get_area().size.width.as_int();
    }
    else if (arg0 == "right")
    {
        direction = Direction::right;
        total_size = active_output->get_area().size.width.as_int();
    }
    else if (arg0 == "up")
    {
        direction = Direction::up;
        total_size = active_output->get_area().size.height.as_int();
    }
    else if (arg0 == "down")
    {
        direction = Direction::down;
        total_size = active_output->get_area().size.height.as_int();
    }
    else if (arg0 == "position")
    {
        if (command.arguments.size() < 2)
        {
            mir::log_error("process_move: move position expected a third argument");
            return;
        }

        auto const& arg1 = command.arguments[index++];
        if (arg1 == "center")
        {
            auto active = policy.get_state().active.get();
            auto area = active_output->get_area();
            float x = (float)area.size.width.as_int() / 2.f - (float)active->get_visible_area().size.width.as_int() / 2.f;
            float y = (float)area.size.height.as_int() / 2.f - (float)active->get_visible_area().size.height.as_int() / 2.f;
            policy.try_move_to((int)x, (int)y);
        }
        else if (arg1 == "mouse")
        {
            auto const& position = policy.get_cursor_position();
            policy.try_move_to((int)position.x.as_int(), (int)position.y.as_int());
        }
        else
        {
            int move_distance_x;
            int move_distance_y;

            if (!parse_move_distance(command.arguments, index, total_size, move_distance_x))
            {
                mir::log_error("process_move: move position <x> <y>: unable to parse x");
                return;
            }

            if (!parse_move_distance(command.arguments, index, total_size, move_distance_y))
            {
                mir::log_error("process_move: move position <x> <y>: unable to parse y");
                return;
            }

            policy.try_move_to(move_distance_x, move_distance_y);
        }
        return;
    }
    else if (arg0 == "absolute")
    {
        auto const& arg1 = command.arguments[index++];
        auto const& arg2 = command.arguments[index++];
        if (arg1 != "position")
        {
            mir::log_error("process_move: move [absolute] ... expected 'position' as the third argument");
            return;
        }

        if (arg2 != "center")
        {
            mir::log_error("process_move: move absolute position ... expected 'center' as the third argument");
            return;
        }

        float x = 0, y = 0;
        for (auto const& output : policy.get_output_list())
        {
            auto area = output->get_area();
            float end_x = (float)area.size.width.as_int() + (float)area.top_left.x.as_int();
            float end_y = (float)area.size.height.as_int() + (float)area.top_left.y.as_int();
            if (end_x > x)
                x = end_x;
            if (end_y > y)
                y = end_y;
        }

        auto active = policy.get_state().active;
        float x_pos = x / 2.f - (float)active->get_visible_area().size.width.as_int() / 2.f;
        float y_pos = y / 2.f - (float)active->get_visible_area().size.height.as_int() / 2.f;
        policy.try_move_to((int)x_pos, (int)y_pos);
        return;
    }
    else if (arg0 == "window" || arg0 == "container")
    {
        auto const back_and_forth = std::find(command.options.begin(), command.options.end(), "--no-auto-back-and-forth") == command.options.end();
        auto const& arg1 = command.arguments[index++];
        if (arg1 != "to")
        {
            mir::log_error("process_move: expected 'to' after 'move window/container ...'");
            return;
        }

        auto const& arg2 = command.arguments[index++];
        if (arg2 != "workspace")
        {
            mir::log_error("process_move: expected 'workspace' after 'move window/container to...'");
            return;
        }

        auto const& arg3 = command.arguments[index++];
        int number = -1;
        if (try_get_number(arg3, number))
        {
            // TODO: Do we need to care about the name here?
            policy.move_active_to_workspace(number, back_and_forth);
            return;
        }
        else if (arg3 == "next")
        {
            policy.move_active_to_next();
            return;
        }
        else if (arg3 == "prev")
        {
            policy.move_active_to_prev();
            return;
        }
        else if (arg3 == "current")
        {
            // TODO: Support window selection
        }
        else if (arg3 == "back_and_forth")
        {
            policy.move_active_to_back_and_forth();
            return;
        }
        else
        {
            policy.move_active_to_workspace_named(arg3, back_and_forth);
            return;
        }
    }

    if (direction < Direction::MAX)
    {
        int move_distance;
        if (parse_move_distance(command.arguments, index, total_size, move_distance))
            policy.try_move_by(direction, move_distance);
        else
            policy.try_move(direction);
    }
}

void I3CommandExecutor::process_sticky(I3Command const& command, I3ScopedCommandList const& command_list)
{
    if (command.arguments.empty())
    {
        mir::log_warning("process_sticky: expects arguments");
        return;
    }

    auto const& arg0 = command.arguments[0];
    if (arg0 == "enable")
        policy.set_is_pinned(true);
    else if (arg0 == "disable")
        policy.set_is_pinned(false);
    else if (arg0 == "toggle")
        policy.toggle_pinned_to_workspace();
    else
        mir::log_warning("process_sticky: unknown arguments: %s", arg0.c_str());
}

// This command will be
void I3CommandExecutor::process_input(I3Command const& command, I3ScopedCommandList const& command_list)
{
    // Payloads appear in the following format:
    //    [type:X, xkb_Y, Z]
    // where X is something like "keyboard", Y is the variable that we want to change
    // and Z is the value of that variable. Z may not be included at all, in which
    // case the variable is set to the default.
    if (command.arguments.size() < 2)
    {
        mir::log_warning("process_input: expects at least 2 arguments");
        return;
    }

    const char* const TYPE_PREFIX = "type:";
    const size_t TYPE_PREFIX_LEN = strlen(TYPE_PREFIX);
    std::string_view type_str = command.arguments[0];
    if (!type_str.starts_with("type:"))
    {
        mir::log_warning("process_input: 'type' string is misformatted: %s", command.arguments[0].c_str());
        return;
    }

    std::string_view type = type_str.substr(TYPE_PREFIX_LEN);
    assert(type == "keyboard");

    std::string_view xkb_str = command.arguments[1];
    const char* const XKB_PREFIX = "xkb_";
    const size_t XKB_PREFIX_LEN = strlen(XKB_PREFIX);
    if (!xkb_str.starts_with(XKB_PREFIX))
    {
        mir::log_warning("process_input: 'xkb' string is misformatted: %s", command.arguments[1].c_str());
        return;
    }

    std::string_view xkb_variable_name = xkb_str.substr(XKB_PREFIX_LEN);
    assert(xkb_variable_name == "model"
        || xkb_variable_name == "layout"
        || xkb_variable_name == "variant"
        || xkb_variable_name == "options");

    mir::log_info("Processing input from locale1: type=%s, xkb_variable=%s", type.data(), xkb_variable_name.data());

    // TODO: This is where we need to process the request
    if (command.arguments.size() == 3)
    {
    }
    else if (command.arguments.size() < 3)
    {
        // TODO: Set to the default
    }
    else
    {
        mir::log_warning("process_input: > 3 arguments were provided but only <= 3 are expected");
        return;
    }
}

void I3CommandExecutor::process_workspace(I3Command const& command, I3ScopedCommandList const& command_list)
{
    if (command.arguments.empty())
    {
        mir::log_error("process_workspace: no arguments provided");
        return;
    }

    std::string const& arg0 = command.arguments[0];
    if (arg0 == "next")
        policy.next_workspace();
    else if (arg0 == "prev")
        policy.prev_workspace();
    else if (arg0 == "next_on_output")
    {
        if (auto const* output = policy.get_active_output())
            policy.next_workspace_on_output(*output);
        else
            mir::log_error("process_workspace: next_on_output has no output to go next on");
    }
    else if (arg0 == "prev_on_output")
    {
        if (auto const* output = policy.get_active_output())
            policy.prev_workspace_on_output(*output);
        else
            mir::log_error("process_workspace: prev_on_output has no output to go prev on");
    }
    else if (arg0 == "back_and_forth")
    {
        policy.back_and_forth_workspace();
    }
    else
    {
        std::string const* arg1 = &arg0;
        auto const back_and_forth = std::find(command.options.begin(), command.options.end(), "--no-auto-back-and-forth") == command.options.end();

        int number = -1;
        if (try_get_number(*arg1, number))
        {
            // Check if we just have "workspace number"
            if (command.arguments.size() < 3)
            {
                policy.select_workspace(number, back_and_forth);
                return;
            }

            // We have "workspace number <name>"
            arg1 = &command.arguments[2];
            policy.select_workspace(*arg1, back_and_forth);
        }
        else
        {
            // We have "workspace <name>"
            policy.select_workspace(*arg1, back_and_forth);
        }
    }
}

void I3CommandExecutor::process_layout(I3Command const& command, I3ScopedCommandList const& command_list)
{
    // https://i3wm.org/docs/userguide.html#manipulating_layout
    std::string const& arg0 = command.arguments[0];
    if (arg0 == "default")
        policy.set_layout_default();
    else if (arg0 == "tabbed")
        policy.set_layout(LayoutScheme::tabbing);
    else if (arg0 == "stacking")
        policy.set_layout(LayoutScheme::stacking);
    else if (arg0 == "splitv")
        policy.set_layout(LayoutScheme::vertical);
    else if (arg0 == "splith")
        policy.set_layout(LayoutScheme::horizontal);
    else if (arg0 == "toggle")
    {
        if (command.arguments.size() == 1)
        {
            mir::log_error("process_layout: expected argument after 'layout toggle ...'");
            return;
        }

        if (command.arguments.size() == 2)
        {
            auto const& arg1 = command.arguments[1];
            if (arg1 == "split")
                policy.try_toggle_layout(false);
            else if (arg1 == "all")
                policy.try_toggle_layout(true);
            else
                mir::log_error("process_layout: expected split/all after 'layout toggle X'");

            return;
        }
        else
        {
            auto const& container = policy.get_state().active;
            if (!container)
            {
                mir::log_error("process_layout: container unavailable");
                return;
            }

            auto current_type = container->get_layout();
            size_t index = 0;
            for (size_t i = 1; i < command.arguments.size(); i++)
            {
                auto const& argn = command.arguments[i];
                if (argn == "split")
                {
                    if (current_type == LayoutScheme::horizontal || current_type == LayoutScheme::vertical)
                    {
                        index = i;
                        break;
                    }
                }
                else if (argn == "tabbed")
                {
                    if (current_type == LayoutScheme::tabbing)
                    {
                        index = i;
                        break;
                    }
                }
                else if (argn == "stacking")
                {
                    if (current_type == LayoutScheme::stacking)
                    {
                        index = i;
                        break;
                    }
                }
                else if (argn == "splitv")
                {
                    if (current_type == LayoutScheme::vertical)
                    {
                        index = i;
                        break;
                    }
                }
                else if (argn == "splith")
                {
                    if (current_type == LayoutScheme::horizontal)
                    {
                        index = i;
                        break;
                    }
                }
            }

            index++;
            if (index == command.arguments.size())
                index = 1;

            auto const& target = command.arguments[index];
            if (target == "split")
                policy.try_toggle_layout(false);
            else if (target == "tabbed")
                policy.set_layout(LayoutScheme::tabbing);
            else if (target == "stacking")
                policy.set_layout(LayoutScheme::stacking);
            else if (target == "splitv")
                policy.set_layout(LayoutScheme::vertical);
            else if (target == "splith")
                policy.set_layout(LayoutScheme::horizontal);
        }
    }
}
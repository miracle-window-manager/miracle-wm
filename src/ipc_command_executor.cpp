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

#include "ipc_command_executor.h"
#include "auto_restarting_launcher.h"
#include "direction.h"
#include "ipc_command.h"
#include "leaf_container.h"
#include "parent_container.h"
#include "policy.h"
#include "utility_general.h"
#include "window_controller.h"
#include "window_helpers.h"

#define MIR_LOG_COMPONENT "miracle"
#include <format>
#include <mir/log.h>
#include <miral/application_info.h>

using namespace miracle;

namespace
{
class ArgumentsIndexer
{
public:
    explicit ArgumentsIndexer(IpcCommand const& command) :
        command { command }
    {
    }

    bool next()
    {
        index++;
        return index < command.arguments.size();
    }

    bool prev()
    {
        index--;
        return index < command.arguments.size();
    }

    [[nodiscard]] std::string const& current() const
    {
        return command.arguments[index];
    }

    bool parse_move_distance(int available_area, int& out)
    {
        if (!next())
            return false;

        try
        {
            out = std::stoi(current());
            if (next())
            {
                // We default to assuming the value is in pixels
                if (current() == "ppt")
                {
                    float ppt = static_cast<float>(out) / 100.f;
                    out = static_cast<float>(available_area) * ppt;
                    return true;
                }
                else if (current() == "px")
                {
                    return true;
                }
            }

            // The 'next' item wasn't ppt or px, so let's pop out of it.
            prev();
            return true;
        }
        catch (std::invalid_argument const& e)
        {
            mir::log_error("Invalid argument: %s", command.arguments[index].c_str());
            return false;
        }
    }

protected:
    IpcCommand const& command;
    size_t index = 0;
};
}

IpcCommandExecutor::IpcCommandExecutor(
    CommandController& policy,
    WorkspaceManager& workspace_manager,
    CompositorState const& state,
    AutoRestartingLauncher& launcher,
    WindowController& window_controller) :
    policy { policy },
    workspace_manager { workspace_manager },
    state { state },
    launcher { launcher },
    window_controller { window_controller }
{
}

IpcValidationResult IpcCommandExecutor::process(miracle::IpcParseResult const& command_list)
{
    IpcValidationResult result;
    for (auto const& command : command_list.commands)
    {
        switch (command.type)
        {
        case IpcCommandType::exec:
            result = process_exec(command, command_list);
            break;
        case IpcCommandType::split:
            result = process_split(command, command_list);
            break;
        case IpcCommandType::focus:
            result = process_focus(command, command_list);
            break;
        case IpcCommandType::move:
            result = process_move(command, command_list);
            break;
        case IpcCommandType::sticky:
            result = process_sticky(command, command_list);
            break;
        case IpcCommandType::exit:
            policy.quit();
            result = {};
            break;
        case IpcCommandType::input:
            result = process_input(command, command_list);
            break;
        case IpcCommandType::workspace:
            result = process_workspace(command, command_list);
            break;
        case IpcCommandType::layout:
            result = process_layout(command, command_list);
            break;
        case IpcCommandType::scratchpad:
            result = process_scratchpad(command, command_list);
            break;
        case IpcCommandType::resize:
            result = process_resize(command, command_list);
            break;
        case IpcCommandType::reload:
            result = process_reload(command, command_list);
            break;
        default:
            result = parse_error(std::format("Unsupported command type: {}", (int)command.type));
            break;
        }

        if (!result.success)
            return result;
    }

    return {};
}

miral::Window IpcCommandExecutor::get_window_meeting_criteria(IpcParseResult const& command_list)
{
    for (auto const& container : state.containers())
    {
        if (container.expired())
            continue;

        auto window = container.lock()->window();
        if (auto const& w = window.value())
        {
            // if (command_list.meets_criteria(w, window_controller))
            return window.value();
        }
    }

    return miral::Window {};
}

IpcValidationResult IpcCommandExecutor::parse_error(std::string error)
{
    mir::log_error("Parse Error: %s", error.c_str());
    return {
        .success = false,
        .parse_error = true,
        .error = std::move(error)
    };
}

IpcValidationResult IpcCommandExecutor::process_exec(miracle::IpcCommand const& command, miracle::IpcParseResult const& command_list)
{
    if (command.arguments.empty())
        return parse_error("process_exec: no arguments were supplied");

    bool no_startup_id = false;
    if (!command.options.empty() && command.options[0] == "--no-startup-id")
        no_startup_id = true;

    if (command.arguments.empty())
        return parse_error("process_exec: argument does not have a command to run");

    std::string exec_cmd;
    for (auto const& arg : command.arguments)
    {
        exec_cmd += arg + " ";
    }

    StartupApp app { exec_cmd, false, no_startup_id };
    launcher.launch(app);
    return {};
}

IpcValidationResult IpcCommandExecutor::process_split(miracle::IpcCommand const& command, miracle::IpcParseResult const& command_list)
{
    if (command.arguments.empty())
        return parse_error("process_split: no arguments were supplied");

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
        return parse_error(std::format("process_split: unknown argument {}", command.arguments.front().c_str()));
    }

    return {};
}

IpcValidationResult IpcCommandExecutor::process_focus(IpcCommand const& command, IpcParseResult const& command_list)
{
    // https://i3wm.org/docs/userguide.html#_focusing_moving_containers
    if (command.arguments.empty())
    {
        if (command_list.scope.empty())
        {
            return parse_error("Focus command expected scope but none was provided");
        }

        auto window = get_window_meeting_criteria(command_list);
        if (window)
            window_controller.select_active_window(window);

        return {};
    }

    auto const& arg = command.arguments.front();
    if (arg == "workspace")
    {
        if (command_list.scope.empty())
        {
            return parse_error("Focus 'workspace' command expected scope but none was provided");
        }

        auto window = get_window_meeting_criteria(command_list);
        auto container = window_controller.get_container(window);
        if (container)
            workspace_manager.request_focus(container->get_workspace()->id());
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
        policy.try_select_parent();
    else if (arg == "child")
        policy.try_select_child();
    else if (arg == "prev")
    {
        auto container = state.focused_container();
        if (!container)
            return parse_error("Active container does nto exist");

        if (container->get_type() != ContainerType::leaf)
            return parse_error("Cannot focus prev when a tiling window is not selected");

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
        auto container = state.focused_container();
        if (!container)
            return parse_error("No container is selected");

        if (container->get_type() != ContainerType::leaf)
            return parse_error("Cannot focus prev when a tiling window is not selected");

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
        policy.try_select_floating();
    else if (arg == "tiling")
        policy.try_select_tiling();
    else if (arg == "mode_toggle")
        policy.try_select_toggle();
    else if (arg == "output")
    {
        if (command.arguments.size() < 2)
            return parse_error("process_focus: 'focus output' must have more than two arguments");

        auto const& arg1 = command.arguments[1];
        if (arg1 == "next")
            policy.try_select_next_output();
        else if (arg1 == "prev")
            policy.try_select_prev_output();
        else if (arg1 == "left")
            policy.try_select_output(Direction::left);
        else if (arg1 == "right")
            policy.try_select_output(Direction::right);
        else if (arg1 == "up")
            policy.try_select_output(Direction::up);
        else if (arg1 == "down")
            policy.try_select_output(Direction::down);
        else
        {
            auto names = std::vector<std::string>(command.arguments.begin() + 1, command.arguments.end());
            policy.try_select_output(names);
        }
    }

    return {};
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

IpcValidationResult IpcCommandExecutor::process_move(IpcCommand const& command, IpcParseResult const& command_list)
{
    auto const& active_output = state.focused_output();
    if (!active_output)
        return parse_error("process_move: output is not set");

    // https://i3wm.org/docs/userguide.html#_focusing_moving_containers
    if (command.arguments.empty())
        return parse_error("process_move: move command expects arguments");

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
            return parse_error("process_move: move position expected a third argument");

        auto const& arg1 = command.arguments[index++];
        if (arg1 == "center")
        {
            auto active = state.focused_container().get();
            auto area = active_output->get_area();
            float x = (float)area.size.width.as_int() / 2.f - (float)active->get_visible_area().size.width.as_int() / 2.f;
            float y = (float)area.size.height.as_int() / 2.f - (float)active->get_visible_area().size.height.as_int() / 2.f;
            policy.try_move_to((int)x, (int)y);
        }
        else if (arg1 == "mouse")
        {
            auto const& position = state.cursor_position;
            policy.try_move_to((int)position.x.as_int(), (int)position.y.as_int());
        }
        else
        {
            int move_distance_x;
            int move_distance_y;

            if (!parse_move_distance(command.arguments, index, total_size, move_distance_x))
                return parse_error("process_move: move position <x> <y>: unable to parse x");

            if (!parse_move_distance(command.arguments, index, total_size, move_distance_y))
                return parse_error("process_move: move position <x> <y>: unable to parse y");

            policy.try_move_to(move_distance_x, move_distance_y);
        }

        return {};
    }
    else if (arg0 == "absolute")
    {
        auto const& arg1 = command.arguments[index++];
        auto const& arg2 = command.arguments[index++];
        if (arg1 != "position")
            return parse_error("process_move: move [absolute] ... expected 'position' as the third argument");

        if (arg2 != "center")
            return parse_error("process_move: move absolute position ... expected 'center' as the third argument");

        float x = 0, y = 0;
        for (auto const& output : state.output_list)
        {
            auto area = output->get_area();
            float end_x = (float)area.size.width.as_int() + (float)area.top_left.x.as_int();
            float end_y = (float)area.size.height.as_int() + (float)area.top_left.y.as_int();
            if (end_x > x)
                x = end_x;
            if (end_y > y)
                y = end_y;
        }

        auto active = state.focused_container();
        float x_pos = x / 2.f - (float)active->get_visible_area().size.width.as_int() / 2.f;
        float y_pos = y / 2.f - (float)active->get_visible_area().size.height.as_int() / 2.f;
        policy.try_move_to((int)x_pos, (int)y_pos);
        return {};
    }
    else if (arg0 == "window" || arg0 == "container")
    {
        auto const back_and_forth = std::find(command.options.begin(), command.options.end(), "--no-auto-back-and-forth") == command.options.end();
        auto const& arg1 = command.arguments[index++];
        if (arg1 != "to")
            return parse_error("process_move: expected 'to' after 'move window/container ...'");

        auto const& arg2 = command.arguments[index++];
        if (arg2 == "workspace")
        {
            if (command.arguments.size() <= 3)
                return parse_error("process_move: expected another argument after 'move container/window to output...'");

            auto const& arg3 = command.arguments[index++];
            int number = -1;
            if (try_get_number(arg3, number))
            {
                // TODO: Do we need to care about the name here?
                policy.move_active_to_workspace(number, back_and_forth);
                return {};
            }
            else if (arg3 == "next")
            {
                policy.move_active_to_next_workspace();
                return {};
            }
            else if (arg3 == "prev")
            {
                policy.move_active_to_prev_workspace();
                return {};
            }
            else if (arg3 == "current")
            {
                // TODO: Support window selection
            }
            else if (arg3 == "back_and_forth")
            {
                policy.move_active_to_back_and_forth();
                return {};
            }
            else
            {
                policy.move_active_to_workspace_named(arg3, back_and_forth);
                return {};
            }
        }
        else if (arg2 == "output")
        {
            if (command.arguments.size() <= 3)
            {
                mir::log_error("process_move: expected another argument after 'move container/window to output...'");
                return {};
            }

            auto const& arg3 = command.arguments[index++];
            if (arg3 == "left")
                policy.try_move_active_to_output(Direction::left);
            else if (arg3 == "right")
                policy.try_move_active_to_output(Direction::right);
            else if (arg3 == "down")
                policy.try_move_active_to_output(Direction::down);
            else if (arg3 == "up")
                policy.try_move_active_to_output(Direction::up);
            else if (arg3 == "current")
                policy.try_move_active_to_current();
            else if (arg3 == "primary")
                policy.try_move_active_to_primary();
            else if (arg3 == "nonprimary")
                policy.try_move_active_to_nonprimary();
            else if (arg3 == "next")
                policy.try_move_active_to_next();
            else
            {
                auto names = std::vector<std::string>(command.arguments.begin() + index - 1, command.arguments.end());
                policy.try_move_active(names);
            }
        }
    }
    else if (arg0 == "scratchpad")
    {
        policy.move_to_scratchpad();
        return {};
    }

    if (direction < Direction::MAX)
    {
        int move_distance;
        if (parse_move_distance(command.arguments, index, total_size, move_distance))
            policy.try_move_by(direction, move_distance);
        else
            policy.try_move(direction);
    }

    return {};
}

IpcValidationResult IpcCommandExecutor::process_sticky(IpcCommand const& command, IpcParseResult const& command_list)
{
    if (command.arguments.empty())
        return parse_error("process_sticky: expects arguments");

    auto const& arg0 = command.arguments[0];
    if (arg0 == "enable")
        policy.set_is_pinned(true);
    else if (arg0 == "disable")
        policy.set_is_pinned(false);
    else if (arg0 == "toggle")
        policy.toggle_pinned_to_workspace();
    else
        mir::log_warning("process_sticky: unknown arguments: %s", arg0.c_str());

    return {};
}

IpcValidationResult IpcCommandExecutor::process_input(IpcCommand const& command, IpcParseResult const& command_list)
{
    // Payloads appear in the following format:
    //    [type:X, xkb_Y, Z]
    // where X is something like "keyboard", Y is the variable that we want to change
    // and Z is the value of that variable. Z may not be included at all, in which
    // case the variable is set to the default.
    if (command.arguments.size() < 2)
        return parse_error("process_input: expects at least 2 arguments");

    const char* const TYPE_PREFIX = "type:";
    const size_t TYPE_PREFIX_LEN = strlen(TYPE_PREFIX);
    std::string_view type_str = command.arguments[0];
    if (!type_str.starts_with("type:"))
        return parse_error(std::format("process_input: 'type' string is misformatted: {}", command.arguments[0].c_str()));

    std::string_view type = type_str.substr(TYPE_PREFIX_LEN);
    assert(type == "keyboard");

    std::string_view xkb_str = command.arguments[1];
    const char* const XKB_PREFIX = "xkb_";
    const size_t XKB_PREFIX_LEN = strlen(XKB_PREFIX);
    if (!xkb_str.starts_with(XKB_PREFIX))
        return parse_error(std::format("process_input: 'xkb' string is misformatted: {}", command.arguments[1].c_str()));

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
        return parse_error("process_input: > 3 arguments were provided but only <= 3 are expected");
    }

    return {};
}

IpcValidationResult IpcCommandExecutor::process_workspace(IpcCommand const& command, IpcParseResult const& command_list)
{
    if (command.arguments.empty())
        return parse_error("process_workspace: no arguments provided");

    std::string const& arg0 = command.arguments[0];
    if (arg0 == "next")
        policy.next_workspace();
    else if (arg0 == "prev")
        policy.prev_workspace();
    else if (arg0 == "next_on_output")
    {
        if (auto const& output = state.focused_output())
            policy.next_workspace_on_output(*output);
        else
            mir::log_error("process_workspace: next_on_output has no output to go next on");
    }
    else if (arg0 == "prev_on_output")
    {
        if (auto const& output = state.focused_output())
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
                return {};
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

    return {};
}

IpcValidationResult IpcCommandExecutor::process_layout(IpcCommand const& command, IpcParseResult const& command_list)
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
            return parse_error("process_layout: expected argument after 'layout toggle ...'");

        if (command.arguments.size() == 2)
        {
            auto const& arg1 = command.arguments[1];
            if (arg1 == "split")
                policy.try_toggle_layout(false);
            else if (arg1 == "all")
                policy.try_toggle_layout(true);
            else
                return parse_error("process_layout: expected split/all after 'layout toggle X'");

            return {};
        }
        else
        {
            auto container = state.focused_container();
            if (!container)
                return parse_error("process_layout: container unavailable");

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

    return {};
}

IpcValidationResult IpcCommandExecutor::process_scratchpad(IpcCommand const& command, IpcParseResult const& command_list)
{
    if (command.arguments.empty())
        return parse_error("process_scratchpad: no arguments provided");

    std::string const& arg0 = command.arguments[0];
    if (arg0 != "show")
        return parse_error("process_scratchpad: all scratchpad commands must be 'scratchpad show'");

    policy.show_scratchpad();
    return {};
}

namespace
{
struct ResizeAdjust
{
    bool success = true;
    std::string error;
    Direction direction = Direction::MAX;
    int first = 0;
    int second = 0;
};

ResizeAdjust parse_resize(CompositorState const& state, ArgumentsIndexer& indexer, int multiplier)
{
    if (!indexer.next())
        return { .success = false, .error = "process_resize: expected argument after 'resize grow'" };

    auto const& container = state.focused_container();
    if (!container)
        return { .success = false, .error = "No container is selcted" };

    ResizeAdjust result;
    if (indexer.current() == "width" || indexer.current() == "horizontal")
    {
        result.direction = Direction::right;
    }
    else if (indexer.current() == "height" || indexer.current() == "vertical")
    {
        result.direction = Direction::down;
    }
    else if (indexer.current() == "up")
    {
        result.direction = Direction::up;
    }
    else if (indexer.current() == "down")
    {
        result.direction = Direction::down;
    }
    else if (indexer.current() == "left")
    {
        result.direction = Direction::left;
    }
    else if (indexer.current() == "right")
    {
        result.direction = Direction::right;
    }
    else
    {
        return { .success = false, .error = std::format("Unknown direction value: {}", indexer.current().c_str()) };
    }

    int available_space = 0;
    switch (result.direction)
    {
    case Direction::up:
    case Direction::down:
        available_space = container->get_output()->get_area().size.height.as_value();
        break;
    default:
        available_space = container->get_output()->get_area().size.width.as_value();
        break;
    }

    int first = 0;
    if (!indexer.parse_move_distance(available_space, first))
        return { .success = false, .error = "cannot parse the first value" };

    if (indexer.next())
    {
        if (indexer.current() != "or")
            return { .success = false, .error = "expected 'or' after first value" };
    }

    int second = 0;
    indexer.parse_move_distance(available_space, second);
    result.first = first * multiplier;
    result.second = second * multiplier;
    return result;
}

struct SetResizeResult
{
    bool success = true;
    std::string error;
    std::optional<int> width;
    std::optional<int> height;
};

SetResizeResult parse_set_resize(CompositorState const& state, ArgumentsIndexer& indexer)
{
    auto const& container = state.focused_container();
    if (!container)
        return { .success = false, .error = "Container is not selected" };

    SetResizeResult result;
    int width = 0, height = 0;
    if (!indexer.parse_move_distance(container->get_output()->get_area().size.width.as_value(), width))
        return { .success = false, .error = "invalid width" };

    if (!indexer.parse_move_distance(container->get_output()->get_area().size.height.as_value(), height))
        return { .success = false, .error = "invalid height" };

    if (width != 0)
        result.width = width;
    if (height != 0)
        result.height = height;

    return result;
}
}

IpcValidationResult IpcCommandExecutor::process_resize(IpcCommand const& command, IpcParseResult const& command_list)
{
    if (command.arguments.empty())
        return parse_error("process_resize: no arguments provided");

    ArgumentsIndexer indexer(command);
    auto const& arg0 = indexer.current();
    if (arg0 == "grow")
    {
        auto adjust = parse_resize(state, indexer, 1);
        if (!adjust.success)
            return parse_error(adjust.error);

        policy.try_resize(adjust.direction, adjust.first);
    }
    else if (arg0 == "shrink")
    {
        auto adjust = parse_resize(state, indexer, -1);
        if (!adjust.success)
            return parse_error(adjust.error);

        policy.try_resize(adjust.direction, adjust.first);
    }
    else if (arg0 == "set")
    {
        auto result = parse_set_resize(state, indexer);
        if (!result.success)
            return parse_error(result.error);

        policy.try_set_size(result.width, result.height);
    }
    else
        return parse_error(std::format("process_resize: unexpected argument: {}", arg0.c_str()));

    return {};
}

IpcValidationResult IpcCommandExecutor::process_reload(IpcCommand const& command, IpcParseResult const&)
{
    if (!command.arguments.empty())
        return parse_error("'reload' command expects no arguments");

    policy.reload_config();
    return {};
}

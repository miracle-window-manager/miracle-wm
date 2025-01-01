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

#ifndef MIRACLEWM_I3_COMMAND_H
#define MIRACLEWM_I3_COMMAND_H

#include <miral/window.h>
#include <miral/window_manager_tools.h>
#include <optional>
#include <string>
#include <vector>

namespace miracle
{
class WindowController;

enum class IpcCommandType
{
    none,
    exec,
    split,
    layout,
    focus,
    move,
    swap,
    sticky,
    workspace,
    mark,
    title_format,
    title_window_icon,
    border,
    shm_log,
    debug_log,
    restart,
    reload,
    exit,
    scratchpad,
    nop,
    i3_bar,
    gaps,
    input,
    resize
};

// https://i3wm.org/docs/userguide.html#command_criteria
enum class IpcScopeType
{
    none,
    all,
    machine,
    title,
    urgent,
    workspace,
    con_mark,
    con_id,
    floating,
    floating_from,
    tiling,
    tiling_from,

    /// TODO: X11-only
    class_,
    /// TODO: X11-only
    instance,
    /// TODO: X11-only
    window_role,
    /// TODO: X11-only
    window_type,
    // TODO: X11-only
    id,
};

struct IpcScope
{
    IpcScopeType type;
    std::string value;
};

struct IpcCommand
{
    IpcCommandType type;
    std::vector<std::string> options;
    std::vector<std::string> arguments;
};

struct IpcParseResult
{
    std::vector<IpcScope> scope;
    std::vector<IpcCommand> commands;
};

class IpcCommandParser
{
public:
    explicit IpcCommandParser(const char*);
    IpcParseResult parse();

private:
    enum class ParseState
    {
        root,
        scope_key,
        scope_value,
        literal,
        command,
        option,
        argument
    };

    std::string data;
    std::vector<ParseState> stack = { ParseState::root };
    size_t index = 0;
    bool has_parsed_command = false;
    bool can_parse_options = true;
};
}

#endif // MIRACLEWM_I3_COMMAND_H

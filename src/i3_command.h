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

enum class I3CommandType
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
    input
};

enum class I3ScopeType
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

struct I3Scope
{
    I3ScopeType type = I3ScopeType::none;
    std::optional<std::string> regex;

    /// Assumes that the provided string_view is in [] brackets
    static std::vector<I3Scope> parse(std::string_view const&, int& ptr);
};

struct I3Command
{
    I3CommandType type = I3CommandType::none;
    std::vector<std::string> arguments;
    /// Anything in the command that starts with "--"
    std::vector<std::string> options;
};

struct I3ScopedCommandList
{
    std::vector<I3Command> commands;
    std::vector<I3Scope> scope;

    bool meets_criteria(miral::Window const&, WindowController&) const;

    static std::vector<I3ScopedCommandList> parse(std::string_view const&);
};
}

#endif // MIRACLEWM_I3_COMMAND_H

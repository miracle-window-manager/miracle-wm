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

#include "i3_command.h"
#include "jpcre2.h"
#include "string_extensions.h"
#include "window_controller.h"
#include "window_helpers.h"

#include <cstring>
#include <ranges>
#define MIR_LOG_COMPONENT "miracle::i3_command"
#include <mir/log.h>

using namespace miracle;

namespace
{
const char* CLASS_STRING = "class";
const char* INSTANCE_STRING = "instance";
const char* WINDOW_ROLE_STRING = "window_role";
const char* MACHINE_STRING = "machine";
const char* ID_STRING = "id";
const char* TITLE_STRING = "title";
const char* URGENT_STRING = "urgent";
const char* WORKSPACE_STRING = "workspace";
const char* ALL_STRING = "all";
const char* FLOATING_STRING = "floating";
const char* TILING_STRING = "tiling";

inline bool try_parse_i3_scope(
    std::string_view const& view,
    int& ptr,
    const char* v,
    bool has_value)
{
    auto const v_len = strlen(v);
    bool starts_with = view.compare(ptr, v_len, v) == 0;
    if (!starts_with)
        return false;

    int possible_new_ptr = ptr + v_len;
    if (has_value)
    {
        if (view[possible_new_ptr] != '=')
            return false;

        ptr = possible_new_ptr;
        return true;
    }
    else
    {
        if (view[possible_new_ptr] == ']' || view[possible_new_ptr == ' '])
        {
            ptr = possible_new_ptr;
            return true;
        }

        return false;
    }
}
}

// https://i3wm.org/docs/userguide.html#command_criteria
std::vector<I3Scope> I3Scope::parse(std::string_view const& view, int& ptr)
{
    if (view[0] != '[')
    {
        ptr = 0;
        return {};
    }

    std::vector<I3Scope> result;
    ptr = 1; // Start past the opening bracket

    while (view[ptr] != ']') // End when we encounter the closing bracket
    {
        I3Scope next;
        if (try_parse_i3_scope(view, ptr, CLASS_STRING, true))
            next.type = I3ScopeType::class_;
        else if (try_parse_i3_scope(view, ptr, WINDOW_ROLE_STRING, true))
            next.type = I3ScopeType::window_role;
        else if (try_parse_i3_scope(view, ptr, MACHINE_STRING, true))
            next.type = I3ScopeType::machine;
        else if (try_parse_i3_scope(view, ptr, ID_STRING, true))
            next.type = I3ScopeType::id;
        else if (try_parse_i3_scope(view, ptr, INSTANCE_STRING, true))
            next.type = I3ScopeType::instance;
        else if (try_parse_i3_scope(view, ptr, TITLE_STRING, true))
            next.type = I3ScopeType::title;
        else if (try_parse_i3_scope(view, ptr, URGENT_STRING, true))
            next.type = I3ScopeType::urgent;
        else if (try_parse_i3_scope(view, ptr, WORKSPACE_STRING, true))
            next.type = I3ScopeType::workspace;
        else if (try_parse_i3_scope(view, ptr, ALL_STRING, false))
        {
            next.type = I3ScopeType::all;
            result.push_back(next);
            continue;
        }
        else if (try_parse_i3_scope(view, ptr, FLOATING_STRING, false))
        {
            next.type = I3ScopeType::floating;
            result.push_back(next);
            continue;
        }
        else if (try_parse_i3_scope(view, ptr, TILING_STRING, false))
        {
            next.type = I3ScopeType::tiling;
            result.push_back(next);
            continue;
        }
        else
        {
            ptr++;
            continue;
        }

        // If we get here, it is assumed that we need to also parse a regex
        ptr++;
        if (view[ptr] != '"')
            continue;

        ptr++;
        auto start = ptr;
        for (; ptr < view.size(); ptr++)
        {
            if (view[ptr] == '"')
                break;
        }

        if (ptr == view.size())
        {
            // TODO: ERROR Haven't encountered a closing quote
            break;
        }

        ptr++;
        next.regex = view.substr(start, ptr - start - 1);

        // TODO: Verify if we have a valid value here or not
        result.push_back(next);
    }

    return result;
}

bool I3ScopedCommandList::meets_criteria(miral::Window const& window, WindowController& window_controller) const
{
    typedef jpcre2::select<char> jp;
    auto container = window_controller.get_container(window);
    if (!container)
        return false;

    for (auto const& criteria : scope)
    {
        switch (criteria.type)
        {
        case I3ScopeType::all:
            break;
        case I3ScopeType::title:
        {
            auto& info = window_controller.info_for(window);
            jp::Regex re(criteria.regex.value());
            auto const& name = info.name();
            return re.match(name);
        }
        default:
            break;
        }
    }

    return true;
}

namespace
{
bool equals(std::string_view const& s, const char* v)
{
    // TODO: Perhaps this is a bit naive, as it is basically a "startswith"
    return strncmp(s.data(), v, strlen(v)) == 0;
}
}

std::vector<I3ScopedCommandList> I3ScopedCommandList::parse(std::string_view const& view)
{
    std::vector<I3ScopedCommandList> list;

    // First, split the view by semicolons, as that will denote different possible scopes
    for (auto const& scope : ((view) | std::ranges::views::split(';')))
    {
        I3ScopedCommandList result;

        // Next, parse the scope
        int ptr = 0;
        auto sub_view = std::string_view(scope.data());
        result.scope = I3Scope::parse(sub_view, ptr);

        // Next, split the sub_view by commas to get the list of commands with the scope
        for (auto const& command_view : (std::string_view(&sub_view[ptr]) | std::ranges::views::split(',')))
        {
            bool is_parsing_options = true;
            I3Command next_command = { I3CommandType::none };

            // Next, we can now read the command tokens space-by-space
            for (auto const& command_token : ((trim_left(command_view)) | std::ranges::views::split(' ')))
            {
                if (next_command.type == I3CommandType::none)
                {
                    if (equals(command_token.data(), "exec"))
                        next_command.type = I3CommandType::exec;
                    else if (equals(command_token.data(), "split"))
                        next_command.type = I3CommandType::split;
                    else if (equals(command_token.data(), "layout"))
                        next_command.type = I3CommandType::layout;
                    else if (equals(command_token.data(), "focus"))
                        next_command.type = I3CommandType::focus;
                    else if (equals(command_token.data(), "move"))
                        next_command.type = I3CommandType::move;
                    else if (equals(command_token.data(), "swap"))
                        next_command.type = I3CommandType::swap;
                    else if (equals(command_token.data(), "sticky"))
                        next_command.type = I3CommandType::sticky;
                    else if (equals(command_token.data(), "workspace"))
                        next_command.type = I3CommandType::workspace;
                    else if (equals(command_token.data(), "mark"))
                        next_command.type = I3CommandType::mark;
                    else if (equals(command_token.data(), "title_format"))
                        next_command.type = I3CommandType::title_format;
                    else if (equals(command_token.data(), "title_window_icon"))
                        next_command.type = I3CommandType::title_window_icon;
                    else if (equals(command_token.data(), "border"))
                        next_command.type = I3CommandType::border;
                    else if (equals(command_token.data(), "shm_log"))
                        next_command.type = I3CommandType::shm_log;
                    else if (equals(command_token.data(), "debug_log"))
                        next_command.type = I3CommandType::debug_log;
                    else if (equals(command_token.data(), "restart"))
                        next_command.type = I3CommandType::restart;
                    else if (equals(command_token.data(), "reload"))
                        next_command.type = I3CommandType::reload;
                    else if (equals(command_token.data(), "exit"))
                        next_command.type = I3CommandType::exit;
                    else if (equals(command_token.data(), "scratchpad"))
                        next_command.type = I3CommandType::scratchpad;
                    else if (equals(command_token.data(), "nop"))
                        next_command.type = I3CommandType::nop;
                    else if (equals(command_token.data(), "i3_bar"))
                        next_command.type = I3CommandType::i3_bar;
                    else if (equals(command_token.data(), "gaps"))
                        next_command.type = I3CommandType::gaps;
                    else if (equals(command_token.data(), "input"))
                        next_command.type = I3CommandType::input;
                    else
                    {
                        mir::log_error("Invalid i3 command type: %s", command_token.data());
                        continue;
                    }
                }
                else
                {
                    auto s = std::string(command_token.data(), command_token.size());
                    if (s.starts_with("--") && is_parsing_options)
                        next_command.options.emplace_back(s);
                    else
                    {
                        next_command.arguments.emplace_back(s);
                        is_parsing_options = false;
                    }
                }
            }

            result.commands.push_back(next_command);
        }

        list.push_back(result);
    }

    return list;
}

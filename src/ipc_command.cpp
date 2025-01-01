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

#include <cctype>
#include <sstream>
#define MIR_LOG_COMPONENT "miracle::i3_command"

#include "ipc_command.h"

#include "ipc.h"
#include "jpcre2.h"
#include "string_extensions.h"
#include "window_controller.h"
#include "window_helpers.h"

#include <cstring>
#include <mir/log.h>
#include <miral/application.h>
#include <miral/application_info.h>
#include <ranges>

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

constexpr char COMMAND_DELIM = ' ';
constexpr char INTER_COMMAND_DELIM = ';';
constexpr char SCOPE_OPEN = '[';
constexpr char SCOPE_CLOSE = ']';
constexpr char SCOPE_EQUALS = '=';
constexpr char SCOPE_DELIM = ' ';
constexpr char LITERAL_OPEN = '"';
constexpr char LITERAL_CLOSE = '"';

IpcScopeType scope_from_string(const std::string& s)
{
    if (s == CLASS_STRING)
        return IpcScopeType::class_;
    else if (s == INSTANCE_STRING)
        return IpcScopeType::instance;
    else if (s == WINDOW_ROLE_STRING)
        return IpcScopeType::window_role;
    else if (s == MACHINE_STRING)
        return IpcScopeType::machine;
    else if (s == ID_STRING)
        return IpcScopeType::id;
    else if (s == TITLE_STRING)
        return IpcScopeType::title;
    else if (s == URGENT_STRING)
        return IpcScopeType::urgent;
    else if (s == WORKSPACE_STRING)
        return IpcScopeType::workspace;
    else if (s == ALL_STRING)
        return IpcScopeType::all;
    else if (s == FLOATING_STRING)
        return IpcScopeType::floating;
    else if (s == TILING_STRING)
        return IpcScopeType::tiling;
    else
        return IpcScopeType::all;
}

IpcCommandType command_from_string(const std::string& str)
{
    if (str == "exec")
        return IpcCommandType::exec;
    else if (str == "split")
        return IpcCommandType::split;
    else if (str == "layout")
        return IpcCommandType::layout;
    else if (str == "focus")
        return IpcCommandType::focus;
    else if (str == "move")
        return IpcCommandType::move;
    else if (str == "swap")
        return IpcCommandType::swap;
    else if (str == "sticky")
        return IpcCommandType::sticky;
    else if (str == "workspace")
        return IpcCommandType::workspace;
    else if (str == "mark")
        return IpcCommandType::mark;
    else if (str == "title_format")
        return IpcCommandType::title_format;
    else if (str == "title_window_icon")
        return IpcCommandType::title_window_icon;
    else if (str == "border")
        return IpcCommandType::border;
    else if (str == "shm_log")
        return IpcCommandType::shm_log;
    else if (str == "debug_log")
        return IpcCommandType::debug_log;
    else if (str == "restart")
        return IpcCommandType::restart;
    else if (str == "reload")
        return IpcCommandType::reload;
    else if (str == "exit")
        return IpcCommandType::exit;
    else if (str == "scratchpad")
        return IpcCommandType::scratchpad;
    else if (str == "nop")
        return IpcCommandType::nop;
    else if (str == "i3_bar")
        return IpcCommandType::i3_bar;
    else if (str == "gaps")
        return IpcCommandType::gaps;
    else if (str == "input")
        return IpcCommandType::input;
    else if (str == "resize")
        return IpcCommandType::resize;
    else
    {
        mir::log_error("Invalid i3 command type: %s", str.c_str());
        return IpcCommandType::none;
    }
}
}

IpcCommandParser::IpcCommandParser(const char* data) :
    data { data }
{
}

IpcParseResult IpcCommandParser::parse()
{
    IpcParseResult retval;
    std::stringstream ss;
    for (; index < data.size(); index++)
    {
        char c = data[index];
        switch (stack.back())
        {
        case ParseState::root:
        {
            if (c == SCOPE_OPEN)
            {
                stack.push_back(ParseState::scope_key);
            }
            else
            {
                assert(ss.str().empty());
                if (c == SCOPE_DELIM)
                    break;

                if (!has_parsed_command)
                {
                    stack.push_back(ParseState::command);
                }
                else
                {
                    // We're reading an option or an argument.
                    if (can_parse_options
                        && index + 2 < data.size()
                        && data[index] == '-'
                        && data[index + 1] == '-')
                    {
                        stack.push_back(ParseState::option);
                    }
                    else
                    {
                        can_parse_options = false;
                        stack.push_back(ParseState::argument);
                    }
                }

                if (c == LITERAL_OPEN)
                    stack.push_back(ParseState::literal);
                else
                    ss << c;
            }
            break;
        }
        case ParseState::scope_key:
        {
            if (c == SCOPE_CLOSE)
            {
                if (ss.str().empty())
                {
                    stack.pop_back();
                    break;
                }

                retval.scope.push_back({ scope_from_string(ss.str()) });
                ss = std::stringstream();
                stack.pop_back();
            }
            else if (c == LITERAL_OPEN)
            {
                assert(ss.str().empty());
                stack.push_back(ParseState::literal);
            }
            else if (c == SCOPE_EQUALS)
            {
                if (ss.str().empty())
                {
                    stack.pop_back();
                    break;
                }

                retval.scope.push_back({ scope_from_string(ss.str()) });
                ss = std::stringstream();
                stack.pop_back();
                stack.push_back(ParseState::scope_value);
            }
            else if (c == SCOPE_DELIM)
            {
                break;
            }
            else
            {
                ss << c;
            }
            break;
        }
        case ParseState::scope_value:
        {
            if (c == SCOPE_CLOSE || c == SCOPE_DELIM)
            {
                assert(!retval.scope.empty());
                retval.scope.back().value = ss.str();
                ss = std::stringstream();
                stack.pop_back();

                if (c == SCOPE_DELIM)
                    stack.push_back(ParseState::scope_key);
            }
            else if (c == LITERAL_OPEN)
            {
                assert(ss.str().empty());
                stack.push_back(ParseState::literal);
            }
            else
            {
                ss << c;
            }
            break;
        }
        case ParseState::literal:
        {
            if (c == LITERAL_CLOSE)
                stack.pop_back();
            else
                ss << c;
            break;
        }
        case ParseState::command:
        {
            if (c == COMMAND_DELIM || c == INTER_COMMAND_DELIM)
            {
                // In case we are encountering random whitespace before
                // we've parsed anything, then we ignore the delim.
                if (ss.str().empty())
                    break;

                retval.commands.push_back({ command_from_string(ss.str()) });
                ss = std::stringstream();
                stack.pop_back();
                can_parse_options = true;
                has_parsed_command = c != INTER_COMMAND_DELIM;
                break;
            }

            ss << c;
            break;
        }
        case ParseState::option:
        {
            if (c == COMMAND_DELIM || c == INTER_COMMAND_DELIM)
            {
                // In case we are encountering random whitespace before
                // we've parsed anything, then we ignore the delim.
                if (c == COMMAND_DELIM && ss.str().empty())
                    break;

                retval.commands.back().options.push_back(ss.str());
                ss = std::stringstream();
                stack.pop_back();
                has_parsed_command = c != INTER_COMMAND_DELIM;
                break;
            }

            ss << c;
            break;
        }
        case ParseState::argument:
        {
            if (c == COMMAND_DELIM || c == INTER_COMMAND_DELIM)
            {
                // In case we are encountering random whitespace before
                // we've parsed anything, then we ignore the delim.
                if (ss.str().empty())
                    break;

                retval.commands.back().arguments.push_back(ss.str());
                ss = std::stringstream();
                stack.pop_back();
                has_parsed_command = c != INTER_COMMAND_DELIM;
                break;
            }

            ss << c;
            break;
        }
        }
    }

    if (!ss.str().empty())
    {
        switch (stack.back())
        {
        case ParseState::option:
            retval.commands.back().options.push_back(ss.str());
            break;
        case ParseState::argument:
            retval.commands.back().arguments.push_back(ss.str());
            break;
        case ParseState::command:
            retval.commands.push_back({ command_from_string(ss.str()) });
            break;
        case ParseState::scope_key:
            retval.scope.push_back({ scope_from_string(ss.str()) });
            break;
        case ParseState::scope_value:
            retval.scope.back().value = ss.str();
            break;
        default:
            break;
        }
    }

    return retval;
}

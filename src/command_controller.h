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

#ifndef MIRACLE_WM_COMMAND_CONTROLLER_H
#define MIRACLE_WM_COMMAND_CONTROLLER_H

#include "direction.h"
#include "output.h"
#include <optional>
#include <string>

namespace miracle
{

/// Abstract interface used to send commands to miracle.
class CommandController
{
public:
    virtual bool try_request_horizontal() = 0;
    virtual bool try_request_vertical() = 0;
    virtual bool try_toggle_layout(bool cycle_through_all) = 0;
    virtual void try_toggle_resize_mode() = 0;
    virtual bool try_resize(Direction direction, int pixels) = 0;
    virtual bool try_set_size(std::optional<int> const& width, std::optional<int> const& height) = 0;
    virtual bool try_move(Direction direction) = 0;
    virtual bool try_move_by(Direction direction, int pixels) = 0;
    virtual bool try_move_to(int x, int y) = 0;
    virtual bool try_select(Direction direction) = 0;
    virtual bool try_select_parent() = 0;
    virtual bool try_select_child() = 0;
    virtual bool try_select_floating() = 0;
    virtual bool try_select_tiling() = 0;
    virtual bool try_select_toggle() = 0;
    virtual bool try_close_window() = 0;
    virtual bool quit() = 0;
    virtual bool try_toggle_fullscreen() = 0;
    virtual bool select_workspace(int number, bool back_and_forth = true) = 0;
    virtual bool select_workspace(std::string const& name, bool back_and_forth) = 0;
    virtual bool next_workspace() = 0;
    virtual bool prev_workspace() = 0;
    virtual bool back_and_forth_workspace() = 0;
    virtual bool next_workspace_on_output(Output const&) = 0;
    virtual bool prev_workspace_on_output(Output const&) = 0;
    virtual bool move_active_to_workspace(int number, bool back_and_forth = true) = 0;
    virtual bool move_active_to_workspace_named(std::string const&, bool back_and_forth) = 0;
    virtual bool move_active_to_next_workspace() = 0;
    virtual bool move_active_to_prev_workspace() = 0;
    virtual bool move_active_to_back_and_forth() = 0;
    virtual bool move_to_scratchpad() = 0;
    virtual bool show_scratchpad() = 0;
    virtual bool toggle_floating() = 0;
    virtual bool toggle_pinned_to_workspace() = 0;
    virtual bool set_is_pinned(bool) = 0;
    virtual bool toggle_tabbing() = 0;
    virtual bool toggle_stacking() = 0;
    virtual bool set_layout(LayoutScheme scheme) = 0;
    virtual bool set_layout_default() = 0;
    virtual void move_cursor_to_output(Output const&) = 0;
    virtual bool try_select_next_output() = 0;
    virtual bool try_select_prev_output() = 0;
    virtual bool try_select_output(Direction direction) = 0;
    virtual bool try_select_output(std::vector<std::string> const& names) = 0;
    virtual bool try_move_active_to_output(Direction direction) = 0;
    virtual bool try_move_active_to_current() = 0;
    virtual bool try_move_active_to_primary() = 0;
    virtual bool try_move_active_to_nonprimary() = 0;
    virtual bool try_move_active_to_next() = 0;
    virtual bool try_move_active(std::vector<std::string> const& names) = 0;
};
}

#endif // MIRACLE_WM_COMMAND_CONTROLLER_H

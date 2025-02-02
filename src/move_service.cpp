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

#define MIR_LOG_COMPONENT "move_service"
#include "move_service.h"
#include "command_controller.h"
#include "compositor_state.h"
#include "config.h"
#include "output.h"
#include "output_manager.h"
#include <mir/log.h>

using namespace miracle;

MoveService::MoveService(
    CommandController& command_controller,
    std::shared_ptr<Config> const& config,
    OutputManager* output_manager) :
    command_controller(command_controller),
    config(config),
    output_manager(output_manager)
{
}

bool MoveService::handle_pointer_event(
    CompositorState& state,
    float x,
    float y,
    MirPointerAction action,
    uint modifiers)
{
    if (state.mode() == WindowManagerMode::moving)
    {
        if (action == mir_pointer_action_button_up)
        {
            command_controller.set_mode(WindowManagerMode::normal);
            return true;
        }

        if (!state.focused_container())
        {
            mir::log_warning("handle_pointer_event: focused container no longer exists while dragging");
            return false;
        }

        /// If we haven't moved since last time, there's nothing to do
        if (cursor_x == x && cursor_y == y)
            return false;

        auto dx = x - cursor_x;
        auto dy = y - cursor_y;
        state.focused_container()->move_by(dx, dy);
        cursor_x = x;
        cursor_y = y;
        return true;
    }
    else if (action == mir_pointer_action_button_down)
    {
        uint move_modifier = config->process_modifier(config->move_modifier());
        if (move_modifier != modifiers)
            return false;

        if (state.mode() != WindowManagerMode::normal)
        {
            mir::log_warning("Must be in normal mode before we can start moving");
            return false;
        }

        if (output_manager->focused() == nullptr)
            return false;

        std::shared_ptr<Container> intersected = output_manager->focused()->intersect(x, y);
        if (!intersected)
            return false;

        command_controller.set_mode(WindowManagerMode::moving);
        command_controller.select_container(intersected);
        cursor_x = x;
        cursor_y = y;
        return true;
    }

    return false;
}
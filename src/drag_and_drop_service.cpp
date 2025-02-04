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

#define MIR_LOG_COMPONENT "drag_and_drop_service"

#include "drag_and_drop_service.h"
#include "command_controller.h"
#include "compositor_state.h"
#include "config.h"
#include "constants.h"
#include "feature_flags.h"
#include "output_manager.h"

#include <mir/log.h>
#include <miral/toolkit_event.h>

using namespace miracle;

DragAndDropService::DragAndDropService(
    std::shared_ptr<CommandController> const& command_controller,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<OutputManager> const& output_manager) :
    command_controller { command_controller },
    config { config },
    output_manager { output_manager }
{
}

bool DragAndDropService::handle_pointer_event(CompositorState& state, float x, float y, MirPointerAction action, uint modifiers)
{
    if (!MIRACLE_FEATURE_FLAG_DRAG_AND_DROP || !config->drag_and_drop().enabled)
        return false;

    if (state.mode() == WindowManagerMode::dragging)
    {
        if (action == mir_pointer_action_button_up)
        {
            command_controller->set_mode(WindowManagerMode::normal);
            if (state.focused_container())
                state.focused_container()->drag_stop();
            last_intersected.reset();
            return true;
        }

        if (!state.focused_container())
        {
            mir::log_warning("handle_drag_and_drop_pointer_event: focused container no longer exists while dragging");
            return false;
        }

        /// If we haven't moved since last time, there's nothing to do
        if (current_x == x && current_y == y)
            return false;

        current_x = x;
        current_y = y;

        // Drag the container to the new position
        float const diff_x = x - cursor_start_x;
        float const diff_y = y - cursor_start_y;
        state.focused_container()->drag(
            static_cast<int>(container_start_x + diff_x),
            static_cast<int>(container_start_y + diff_y));

        if (output_manager->focused()->active()->is_empty())
        {
            drag_to(state.focused_container(), output_manager->focused()->active());
            return true;
        }

        // Get the intersection and try to move ourselves there. We only care if we're intersecting
        // a leaf container, as those would be the only one in the grid.
        std::shared_ptr<Container> intersected = output_manager->focused()->intersect_leaf(x, y, true);
        if (!intersected)
        {
            last_intersected.reset();
            return true;
        }

        if (last_intersected.lock() == intersected)
            return true;

        last_intersected = intersected;
        drag_to(state.focused_container(), intersected);
        return true;
    }
    else if (action == mir_pointer_action_button_down)
    {
        uint command_modifiers = config->process_modifier(config->drag_and_drop().modifiers);
        if (command_modifiers != modifiers)
            return false;

        if (state.mode() != WindowManagerMode::normal)
        {
            mir::log_warning("Must be in normal mode before we can start dragging");
            return false;
        }

        if (output_manager->focused() == nullptr)
            return false;

        std::shared_ptr<Container> intersected = output_manager->focused()->intersect(x, y);
        if (!intersected)
            return false;

        if (!intersected->drag_start())
        {
            mir::log_warning("Cannot drag container of type %d", (int)intersected->get_type());
            return false;
        }

        command_controller->set_mode(WindowManagerMode::dragging);
        command_controller->select_container(intersected);
        cursor_start_x = x;
        cursor_start_y = y;
        container_start_x = static_cast<float>(intersected->get_visible_area().top_left.x.as_int());
        container_start_y = static_cast<float>(intersected->get_visible_area().top_left.y.as_int());
        current_x = x;
        current_y = y;
        return true;
    }

    return false;
}

void DragAndDropService::drag_to(
    std::shared_ptr<Container> const& dragging,
    std::shared_ptr<Container> const& to)
{
    if (dragging == to)
        return;

    // TODO: Convert dragging to a leaf beforehand
    if (!to->is_leaf() || !dragging->is_leaf())
        return;

    dragging->move_to(*to);
}

void DragAndDropService::drag_to(
    std::shared_ptr<Container> const& dragging,
    WorkspaceInterface* workspace)
{
    if (dragging->get_workspace() == workspace)
        return;

    // TODO: Convert dragging to a leaf beforehand
    if (!dragging->is_leaf())
        return;

    workspace->add_to_root(*dragging);
}
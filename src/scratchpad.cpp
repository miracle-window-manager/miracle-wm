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

#define MIR_LOG_COMPONENT "scratchpad"

#include "scratchpad.h"
#include "container.h"
#include "output_interface.h"
#include "output_manager.h"
#include "scratchpad_state.h"

#include <mir/log.h>

using namespace miracle;

Scratchpad::Scratchpad(std::shared_ptr<WindowController> const& window_controller, std::shared_ptr<OutputManager> const& output_manager) :
    window_controller { window_controller },
    output_manager { output_manager }
{
}

bool Scratchpad::move_to(std::shared_ptr<Container> const& container)
{
    if (container->get_type() != ContainerType::leaf)
    {
        mir::log_error("move_to_scratchpad: cannot move window to scratchpad: %d", static_cast<int>(container->get_type()));
        return false;
    }

    // Remove it from its current workspace since it is no longer wanted there
    if (auto workspace = container->get_workspace())
        workspace->delete_container(container);

    items.push_back({ container, false });
    container->scratchpad_state(ScratchpadState::fresh);
    container->set_workspace(nullptr);
    container->hide();
    return true;
}

bool Scratchpad::remove(std::shared_ptr<Container> const& container)
{
    return items.erase(std::remove_if(items.begin(), items.end(), [&](auto const& item)
    {
        return item.container == container;
    }),
               items.end())
        != items.end();
}

void Scratchpad::toggle(ScratchpadItem& other)
{
    other.is_showing = !other.is_showing;
    other.container->scratchpad_state(ScratchpadState::changed);
    if (other.is_showing)
    {
        auto window = other.container->window().value();
        auto output_extents = output_manager->focused()->get_area();
        other.container->show();
        miral::WindowSpecification spec;
        spec.depth_layer() = mir_depth_layer_above;
        spec.top_left() = {
            output_extents.top_left.x.as_int() + (output_extents.size.width.as_int() - window.size().width.as_int()) / 2.f,
            output_extents.top_left.y.as_int() + (output_extents.size.height.as_int() - window.size().height.as_int()) / 2.f,
        };
        window_controller->modify(window, spec);
        window_controller->noclip(window);
    }
    else
        other.container->hide();
}

bool Scratchpad::toggle_show(std::shared_ptr<Container> const& container)
{
    for (auto& other : items)
    {
        if (other.container == container)
        {
            toggle(other);
            return true;
        }
    }

    return false;
}

bool Scratchpad::toggle_show_all()
{
    for (auto& item : items)
        toggle(item);

    return true;
}

bool Scratchpad::contains(std::shared_ptr<Container> const& container)
{
    for (auto const& other : items)
    {
        if (other.container == container)
            return true;
    }

    return false;
}

bool Scratchpad::is_showing(std::shared_ptr<Container> const& container)
{
    for (auto const& other : items)
    {
        if (other.container == container)
            return other.is_showing;
    }

    return false;
}
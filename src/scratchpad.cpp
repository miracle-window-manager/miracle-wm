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
#include "floating_window_container.h"

#include <mir/log.h>

using namespace miracle;

Scratchpad::Scratchpad(WindowController& window_controller) :
    window_controller { window_controller }
{
}

bool Scratchpad::move_to(std::shared_ptr<Container> const& container)
{
    auto floating = Container::as_floating(container);
    floating->set_workspace(nullptr);
    floating->hide();
    containers.push_back(floating);
    return true;
}

bool Scratchpad::remove(std::shared_ptr<Container> const& container)
{
    assert(container->get_type() == ContainerType::floating_window);
    auto const& floating = Container::as_floating(container);
    return containers.erase(std::remove(containers.begin(), containers.end(), floating), containers.end())
        != containers.end();
}

bool Scratchpad::toggle_show(std::shared_ptr<Container>& container)
{
    for (auto const& other : containers)
    {
        if (other == container)
            return other->toggle_shown();
    }

    return false;
}

bool Scratchpad::toggle_show_all()
{
    for (auto const& container : containers)
        container->toggle_shown();

    return true;
}

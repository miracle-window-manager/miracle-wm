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

#include "compositor_state.h"

using namespace miracle;

[[nodiscard]] std::shared_ptr<Container> CompositorState::active() const
{
    if (!focused.expired())
        return focused.lock();

    return nullptr;
}

void CompositorState::focus(std::shared_ptr<Container> const& container, bool is_anonymous)
{
    if (is_anonymous)
    {
        focused = container;
        return;
    }

    auto it = std::find_if(focus_order.begin(), focus_order.end(), [&](auto const& element)
    {
        return !element.expired() && element.lock() == container;
    });

    if (it != focus_order.end())
    {
        std::rotate(focus_order.begin(), it, it + 1);
        focused = container;
    }
}

void CompositorState::unfocus(std::shared_ptr<Container> const& container)
{
    if (!focused.expired())
    {
        if (focused.lock() == container)
            focused.reset();
    }
}

void CompositorState::add(std::shared_ptr<Container> const& container)
{
    CompositorState::focus_order.push_back(container);
}

void CompositorState::remove(std::shared_ptr<Container> const& container)
{
    focus_order.erase(std::remove_if(focus_order.begin(), focus_order.end(), [&](auto const& element)
    {
        return !element.expired() && element.lock() == container;
    }));
}

std::shared_ptr<Container> CompositorState::get_first_with_type(ContainerType type) const
{
    for (auto const& container : focus_order)
    {
        if (!container.expired() && container.lock()->get_type() == type)
            return container.lock();
    }

    return nullptr;
}
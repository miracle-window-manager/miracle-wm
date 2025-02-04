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

#define MIR_LOG_COMPONENT "compositor_state"

#include "compositor_state.h"
#include <mir/log.h>

using namespace miracle;

CompositorState::CompositorState() :
    render_data_manager_(std::make_unique<RenderDataManager>())
{
}

std::shared_ptr<Container> CompositorState::focused_container() const
{
    if (!focused.expired())
        return focused.lock();

    return nullptr;
}

void CompositorState::focus_container(std::shared_ptr<Container> const& container, bool is_anonymous)
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

void CompositorState::unfocus_container(std::shared_ptr<Container> const& container)
{
    if (!focused.expired())
    {
        if (focused.lock() == container)
            focused.reset();
    }
}

void CompositorState::add(std::shared_ptr<Container> const& container)
{
    focus_order.push_back(container);
    mir::log_debug("add: there are now %zu surfaces in the focus order", focus_order.size());
}

void CompositorState::remove(std::shared_ptr<Container> const& container)
{
    focus_order.erase(std::remove_if(focus_order.begin(), focus_order.end(), [&](auto const& element)
    {
        return !element.expired() && element.lock() == container;
    }));
    mir::log_debug("remove: there are now %zu surfaces in the focus order", focus_order.size());
}

std::shared_ptr<Container> CompositorState::first_floating() const
{
    for (auto const& container : focus_order)
    {
        if (!container.expired() && !container.lock()->anchored())
            return container.lock();
    }

    return nullptr;
}

std::shared_ptr<Container> CompositorState::first_tiling() const
{
    for (auto const& container : focus_order)
    {
        if (!container.expired() && container.lock()->anchored())
            return container.lock();
    }

    return nullptr;
}

WindowManagerMode CompositorState::mode() const
{
    return mode_;
}

void CompositorState::mode(WindowManagerMode next)
{
    mode_ = next;
}

RenderDataManager* CompositorState::render_data_manager() const
{
    return render_data_manager_.get();
}
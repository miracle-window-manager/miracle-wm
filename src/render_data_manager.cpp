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

#include "render_data_manager.h"
#include "container.h"
#include <algorithm>
#include <mir/scene/surface.h>

using namespace miracle;

namespace
{
inline bool needs_outline(Container const& container)
{
    auto const surface = container.window().value().operator std::shared_ptr<mir::scene::Surface>();
    container.window().value();
    return (container.get_type() == ContainerType::leaf)
        && (surface == nullptr || !surface->parent());
}

inline glm::mat4 workspace_transform(Container const& container)
{
    return container.get_output_transform() * container.get_workspace_transform();
}
}

RenderDataManager::RenderDataManager()
{
    render_data.reserve(48);
}

void RenderDataManager::add(Container const& container)
{
    if (container.window() == std::nullopt)
        return;

    render_data.emplace_back(RenderData {
        .surface = container.window()->operator std::shared_ptr<mir::scene::Surface>().get(),
        .needs_outline = needs_outline(container),
        .is_focused = container.is_focused(),
        .transform = container.get_transform(),
        .workspace_transform = workspace_transform(container) });
}

void RenderDataManager::transform_change(Container const& container)
{
    for (auto& data : render_data)
    {
        if (data.surface == container.window()->operator std::shared_ptr<mir::scene::Surface>().get())
        {
            data.transform = container.get_transform();
            return;
        }
    }
}

void RenderDataManager::workspace_transform_change(Container const& container)
{
    for (auto& data : render_data)
    {
        if (data.surface == container.window()->operator std::shared_ptr<mir::scene::Surface>().get())
        {
            data.workspace_transform = workspace_transform(container);
            return;
        }
    }
}

void RenderDataManager::focus_change(Container const& container)
{
    for (auto& data : render_data)
    {
        if (data.surface == container.window()->operator std::shared_ptr<mir::scene::Surface>().get())
        {
            data.is_focused = container.is_focused();
            return;
        }
    }
}

void RenderDataManager::remove(Container const& container)
{
    render_data.erase(std::remove_if(render_data.begin(), render_data.end(), [&](RenderData const& data)
    {
        return data.surface == container.window()->operator std::shared_ptr<mir::scene::Surface>().get();
    }),
        render_data.end());
}

std::vector<RenderData> const& RenderDataManager::get()
{
    std::lock_guard lock(mutex);
    copy_for_renderer.clear();
    if (render_data.capacity() > copy_for_renderer.capacity())
        copy_for_renderer.reserve(render_data.capacity());
    std::copy(
        render_data.begin(),
        render_data.end(),
        std::back_inserter(copy_for_renderer));
    return copy_for_renderer;
}
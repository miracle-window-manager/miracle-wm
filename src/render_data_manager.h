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

#ifndef MIRACLEWM_SURFACE_TRACKER_H
#define MIRACLEWM_SURFACE_TRACKER_H

#include <glm/glm.hpp>
#include <mir/scene/surface.h>
#include <vector>

namespace miracle
{

class Container;

struct RenderData
{
    mir::scene::Surface* surface;
    bool needs_outline = false;
    bool is_focused = false;
    glm::mat4 transform = glm::mat4(1.f);
    glm::mat4 workspace_transform = glm::mat4(1.f);
};

class RenderDataManager
{
public:
    RenderDataManager();
    void add(Container const&);
    void remove(Container const&);
    void transform_change(Container const&);
    void workspace_transform_change(Container const&);
    void focus_change(Container const&);
    std::vector<RenderData> const& get();

private:
    std::mutex mutex;
    std::vector<RenderData> render_data;
    std::vector<RenderData> copy_for_renderer;
};

} // miracle

#endif // MIRACLEWM_SURFACE_TRACKER_H

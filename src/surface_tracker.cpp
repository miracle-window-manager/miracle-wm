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

#include "surface_tracker.h"

using namespace miracle;

void SurfaceTracker::add(miral::Window const& window)
{
    std::lock_guard lock(mutex);
    auto surface = window.operator std::shared_ptr<mir::scene::Surface>();
    map.insert(std::pair(surface.get(), window));
}

void SurfaceTracker::remove(miral::Window const& window)
{
    std::lock_guard lock(mutex);
    auto surface = window.operator std::shared_ptr<mir::scene::Surface>();
    auto it = map.find(surface.get());
    if (it != map.end())
        map.erase(it);
}

miral::Window SurfaceTracker::get(mir::scene::Surface const* surface)
{
    std::lock_guard lock(mutex);
    auto it = map.find(surface);
    if (it == map.end())
        return {};

    return it->second;
}
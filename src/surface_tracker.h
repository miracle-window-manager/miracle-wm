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

#include <map>
#include <miral/window.h>

namespace miracle
{

class SurfaceTracker
{
public:
    void add(miral::Window const&);
    void remove(miral::Window const&);
    miral::Window get(mir::scene::Surface const*);

private:
    std::mutex mutex;
    std::map<mir::scene::Surface const*, miral::Window> map;
};

} // miracle

#endif // MIRACLEWM_SURFACE_TRACKER_H

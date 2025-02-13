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

#ifndef MIRACLEWM_WINDOW_HELPERS_H
#define MIRACLEWM_WINDOW_HELPERS_H

#include "container.h"
#include <miral/window_info.h>
#include <miral/window_manager_tools.h>

namespace miracle
{
class LeafContainer;

namespace window_helpers
{
    miral::WindowSpecification copy_from(miral::WindowInfo const&);
}
}

#endif // MIRACLEWM_WINDOW_HELPERS_H

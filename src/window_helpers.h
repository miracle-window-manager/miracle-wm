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
class TilingWindowTree;

namespace window_helpers
{
    bool is_window_fullscreen(MirWindowState state);

    template <typename T>
    ContainerType get_ideal_type(T const& requested_specification)
    {
        auto has_exclusive_rect = requested_specification.exclusive_rect().is_set();
        auto is_attached = requested_specification.attached_edges().is_set();
        if (has_exclusive_rect || is_attached)
            return ContainerType::shell;

        auto t = requested_specification.type();
        if (t == mir_window_type_normal || t == mir_window_type_freestyle)
            return ContainerType::none;

        return ContainerType::floating_window;
    }

    miral::WindowSpecification copy_from(miral::WindowInfo const&);
}
}

#endif // MIRACLEWM_WINDOW_HELPERS_H

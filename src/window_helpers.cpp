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

#define MIR_LOG_COMPONENT "window_helpers"

#include "window_helpers.h"
#include "leaf_container.h"

miral::WindowSpecification miracle::window_helpers::copy_from(miral::WindowInfo const& info)
{
    miral::WindowSpecification spec;
    spec.name() = info.name();
    spec.state() = info.state();
    spec.type() = info.type();
    spec.parent() = info.parent();
    spec.min_width() = info.min_width();
    spec.max_width() = info.max_width();
    spec.min_height() = info.min_height();
    spec.max_height() = info.max_height();
    spec.width_inc() = info.width_inc();
    spec.height_inc() = info.height_inc();
    spec.min_aspect() = info.min_aspect();
    spec.max_aspect() = info.max_aspect();
    spec.preferred_orientation() = info.preferred_orientation();
    spec.confine_pointer() = info.confine_pointer();
    spec.shell_chrome() = info.shell_chrome();
    spec.userdata() = info.userdata();
    spec.attached_edges() = info.attached_edges();
    if (info.exclusive_rect().is_set())
    {
        spec.exclusive_rect() = mir::optional_value<mir::optional_value<geom::Rectangle>>(info.exclusive_rect());
    }
    spec.application_id() = info.application_id();
    spec.focus_mode() = info.focus_mode();
    spec.visible_on_lock_screen() = info.visible_on_lock_screen();
    return spec;
}
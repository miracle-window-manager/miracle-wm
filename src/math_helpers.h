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

#ifndef MIRACLE_WM_MATH_HELPERS_H
#define MIRACLE_WM_MATH_HELPERS_H

#include <mir/geometry/point.h>
#include <mir/geometry/rectangle.h>
#include <cmath>

namespace geom = mir::geometry;

namespace miracle
{
geom::Point get_center(geom::Rectangle const& r) {
    return {
    r.top_left.x.as_int() + static_cast<int>(((float)r.size.width.as_int()) / 2.f),
    r.top_left.x.as_int() + static_cast<int>(((float)r.size.height.as_int()) / 2.f)
    };
}

float get_distance(geom::Point const& p1, geom::Point const& p2) {
    auto const y_diff = p2.x.as_int() - p1.x.as_int();
    auto const x_diff = p2.y.as_int() - p1.y.as_int();
    return sqrtf(x_diff * x_diff + y_diff * y_diff);
}
}

#endif //MIRACLE_WM_MATH_HELPERS_H

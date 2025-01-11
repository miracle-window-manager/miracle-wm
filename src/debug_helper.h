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

#ifndef MIRACLE_WM_DEBUG_HELPER_H
#define MIRACLE_WM_DEBUG_HELPER_H

#define MIR_LOG_COMPONENT "debug_helper"
#include "mir/log.h"
#include <miral/window_specification.h>
#include <sstream>
#include <string>

namespace miracle
{
std::string point_to_string(mir::optional_value<mir::geometry::Point> const& point)
{
    if (!point)
        return "(unset)";

    std::stringstream ss;
    ss << "(";
    ss << point.value().x.as_int();
    ss << ", ";
    ss << point.value().y.as_int();
    ss << ")";
    return ss.str();
}

std::string size_to_string(mir::optional_value<mir::geometry::Size> const& size)
{
    if (!size)
        return "(unset)";

    std::stringstream ss;
    ss << "(";
    ss << size.value().width.as_int();
    ss << ", ";
    ss << size.value().height.as_int();
    ss << ")";
    return ss.str();
}

void print_specification(std::string const& label, mir::geometry::Rectangle const& spec)
{
    std::stringstream ss;
    ss << label << ": \n";
    ss << "  top_left(): " << point_to_string(spec.top_left) << "\n";
    ss << "  size(): " << size_to_string(spec.size) << "\n";
    mir::log_info(ss.str());
}
}

#endif // MIRACLE_WM_DEBUG_HELPER_H

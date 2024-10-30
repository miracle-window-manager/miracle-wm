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

#include "layout_scheme.h"
#include <stdexcept>

const char* miracle::to_string(miracle::LayoutScheme scheme)
{
    switch (scheme)
    {
    case LayoutScheme::horizontal:
        return "splith";
    case LayoutScheme::vertical:
        return "splitv";
    case LayoutScheme::stacking:
        return "stacking";
    case LayoutScheme::tabbing:
        return "tabbed";
    default:
    {
        std::string error = "Encountered unexpected scheme in to_string: " + std::to_string((int)scheme);
        throw std::logic_error(error);
    }
    }
}

miracle::LayoutScheme miracle::get_next_layout(miracle::LayoutScheme scheme)
{
    auto next = static_cast<LayoutScheme>((int)scheme + 1);
    if (next == LayoutScheme::none)
        next = static_cast<LayoutScheme>(0);
    return next;
}
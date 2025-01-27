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

#ifndef MIRACLE_WM_OUTPUT_FACTORY_H
#define MIRACLE_WM_OUTPUT_FACTORY_H

#include <memory>
#include <mir/geometry/rectangle.h>

namespace miracle
{
class Output;
class OutputManager;

class OutputFactory
{
public:
    virtual ~OutputFactory() = default;
    virtual std::unique_ptr<Output> create(
        std::string name,
        int id,
        mir::geometry::Rectangle area,
        OutputManager* output_manager)
        = 0;
};
}

#endif // MIRACLE_WM_OUTPUT_FACTORY_H

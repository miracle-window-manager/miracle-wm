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

#ifndef MIRACLE_WM_OUTPUT_MANAGER_H
#define MIRACLE_WM_OUTPUT_MANAGER_H

#include "output_factory.h"

#include <memory>
#include <mir/geometry/rectangle.h>
#include <vector>

namespace miracle
{
class Output;
class WorkspaceManager;

class OutputManager
{
public:
    explicit OutputManager(
        std::unique_ptr<OutputFactory> output_factory);

    Output* create(
        std::string name,
        int id,
        mir::geometry::Rectangle area,
        WorkspaceManager& workspace_manager);
    void update(int id, mir::geometry::Rectangle area);
    bool remove(int id, WorkspaceManager& workspace_manager);
    [[nodiscard]] std::vector<std::unique_ptr<Output>> const& outputs() const;
    bool focus(int id);
    bool unfocus(int id);
    Output* focused();

private:
    std::unique_ptr<OutputFactory> output_factory;
    std::vector<std::unique_ptr<Output>> outputs_;
    Output* focused_ = nullptr;
};

}

#endif // MIRACLE_WM_OUTPUT_MANAGER_H

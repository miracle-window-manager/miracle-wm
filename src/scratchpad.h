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

#ifndef SCRATCHPAD_MIRACLE_WM_H
#define SCRATCHPAD_MIRACLE_WM_H

#include "window_controller.h"

#include <memory>
#include <vector>

namespace miracle
{
class Container;
class OutputManager;

struct ScratchpadItem
{
    std::shared_ptr<Container> container;
    bool is_showing = false;
};

class Scratchpad
{
public:
    Scratchpad(WindowController&, OutputManager* output_manager);
    ~Scratchpad() = default;
    Scratchpad(Scratchpad&&) = delete;
    Scratchpad(Scratchpad const&) = delete;

    bool move_to(std::shared_ptr<Container> const&);
    bool remove(std::shared_ptr<Container> const&);
    bool toggle_show(std::shared_ptr<Container>&);
    bool toggle_show_all();
    bool contains(std::shared_ptr<Container> const&);
    bool is_showing(std::shared_ptr<Container> const&);

private:
    void toggle(ScratchpadItem& item);

    WindowController& window_controller;
    OutputManager* output_manager;
    std::vector<ScratchpadItem> items;
};
}

#endif // SCRATCHPAD_MIRACLE_WM_H

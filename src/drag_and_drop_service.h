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

#ifndef MIRACLE_WM_DRAG_AND_DROP_SERVICE_H
#define MIRACLE_WM_DRAG_AND_DROP_SERVICE_H

#include <memory>
#include <mir_toolkit/event.h>

namespace miracle
{

class Container;
class Config;
class CompositorState;
class CommandController;
class OutputManager;
class TilingWindowTree;

class DragAndDropService
{
public:
    DragAndDropService(CommandController& command_controller, std::shared_ptr<Config> const& config, OutputManager* output_manager);
    bool handle_pointer_event(CompositorState& state, float x, float y, MirPointerAction action, uint modifiers);

private:
    CommandController& command_controller;
    std::shared_ptr<Config> config;
    OutputManager* output_manager;

    float cursor_start_x = 0;
    float cursor_start_y = 0;
    float container_start_x = 0;
    float container_start_y = 0;
    float current_x = 0;
    float current_y = 0;
    std::weak_ptr<Container> last_intersected;

    void drag_to(
        std::shared_ptr<Container> const& dragging,
        std::shared_ptr<Container> const& to);

    void drag_to(
        std::shared_ptr<Container> const& dragging,
        TilingWindowTree* tree);
};

} // miracle

#endif // MIRACLE_WM_DRAG_AND_DROP_SERVICE_H

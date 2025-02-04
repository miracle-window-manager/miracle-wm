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

#ifndef MIRACLE_WM_MOVE_SERVICE_H
#define MIRACLE_WM_MOVE_SERVICE_H

#include <memory>
#include <mir_toolkit/event.h>

namespace miracle
{
class CommandController;
class Config;
class CompositorState;
class OutputManager;

class MoveService
{
public:
    MoveService(CommandController&, std::shared_ptr<Config> const&, OutputManager*);
    bool handle_pointer_event(CompositorState& state, float x, float y, MirPointerAction action, uint modifiers);

private:
    CommandController& command_controller;
    std::shared_ptr<Config> config;
    OutputManager* output_manager;

    float cursor_x = 0;
    float cursor_y = 0;
};

} // miracle

#endif // MIRACLE_WM_MOVE_SERVICE_H

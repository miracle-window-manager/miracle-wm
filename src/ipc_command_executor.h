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

#ifndef MIRACLEWM_I_3_COMMAND_EXECUTOR_H
#define MIRACLEWM_I_3_COMMAND_EXECUTOR_H

#include "compositor_state.h"
#include "ipc_command.h"
#include <mir/glib_main_loop.h>

namespace miracle
{

class CommandController;
class WorkspaceManager;
class AutoRestartingLauncher;
class WindowController;

struct IpcValidationResult
{
    bool success = true;
    bool parse_error = false;
    std::string error;
};

/// Processes all commands coming from i3 IPC. This class is mostly for organizational
/// purposes, as a lot of logic is associated with processing these operations.
class IpcCommandExecutor
{
public:
    IpcCommandExecutor(
        CommandController&,
        WorkspaceManager&,
        CompositorState const&,
        AutoRestartingLauncher&,
        WindowController&);
    IpcValidationResult process(IpcParseResult const&);

private:
    CommandController& policy;
    WorkspaceManager& workspace_manager;
    CompositorState const& state;
    AutoRestartingLauncher& launcher;
    WindowController& window_controller;

    miral::Window get_window_meeting_criteria(IpcParseResult const&);
    IpcValidationResult process_exec(IpcCommand const&, IpcParseResult const&);
    IpcValidationResult process_split(IpcCommand const&, IpcParseResult const&);
    IpcValidationResult process_focus(IpcCommand const&, IpcParseResult const&);
    IpcValidationResult process_move(IpcCommand const&, IpcParseResult const&);
    IpcValidationResult process_sticky(IpcCommand const&, IpcParseResult const&);
    IpcValidationResult process_input(IpcCommand const&, IpcParseResult const&);
    IpcValidationResult process_workspace(IpcCommand const&, IpcParseResult const&);
    IpcValidationResult process_layout(IpcCommand const&, IpcParseResult const&);
    IpcValidationResult process_scratchpad(IpcCommand const&, IpcParseResult const&);
    IpcValidationResult process_resize(IpcCommand const&, IpcParseResult const&);

    IpcValidationResult parse_error(std::string error);
};

} // miracle

#endif // MIRACLEWM_I_3_COMMAND_EXECUTOR_H

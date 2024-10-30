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

#include "i3_command.h"
#include <mir/glib_main_loop.h>

namespace miracle
{

class Policy;
class WorkspaceManager;
class AutoRestartingLauncher;
class WindowController;

/// Processes all commands coming from i3 IPC. This class is mostly for organizational
/// purposes, as a lot of logic is associated with processing these operations.
class I3CommandExecutor
{
public:
    I3CommandExecutor(
        Policy&,
        WorkspaceManager&,
        miral::WindowManagerTools const&,
        AutoRestartingLauncher&,
        WindowController&);
    void process(I3ScopedCommandList const&);

private:
    Policy& policy;
    WorkspaceManager& workspace_manager;
    miral::WindowManagerTools tools;
    AutoRestartingLauncher& launcher;
    WindowController& window_controller;

    miral::Window get_window_meeting_criteria(I3ScopedCommandList const&);
    void process_exec(I3Command const&, I3ScopedCommandList const&);
    void process_split(I3Command const&, I3ScopedCommandList const&);
    void process_focus(I3Command const&, I3ScopedCommandList const&);
    void process_move(I3Command const&, I3ScopedCommandList const&);
    void process_sticky(I3Command const&, I3ScopedCommandList const&);
    void process_input(I3Command const&, I3ScopedCommandList const&);
    void process_workspace(I3Command const&, I3ScopedCommandList const&);
    void process_layout(I3Command const&, I3ScopedCommandList const&);
};

} // miracle

#endif // MIRACLEWM_I_3_COMMAND_EXECUTOR_H

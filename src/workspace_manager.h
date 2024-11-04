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

#ifndef WORKSPACE_MANAGER_H
#define WORKSPACE_MANAGER_H

#include "workspace_observer.h"

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <miral/window_manager_tools.h>
#include <vector>

namespace miracle
{

class Output;

/// A central place to request operations on workspaces.
/// [Workspace] objects are held in their [Output] containers.
class WorkspaceManager
{
public:
    WorkspaceManager(
        miral::WindowManagerTools const& tools,
        WorkspaceObserverRegistrar& registry,
        std::function<Output const*()> const& get_active,
        std::function<std::vector<std::shared_ptr<Output>>()> const& get_outputs);
    WorkspaceManager(WorkspaceManager const&) = delete;
    virtual ~WorkspaceManager() = default;

    /// Request workspace by number. If it does not yet exist, then one
    /// is created on the provided output. If it does exist, we navigate
    /// to the screen containing that workspace and show it if it
    /// isn't already shown.
    /// \param output_hint output that we want to show on if creating a new workspace
    /// \param key workspace number that we want to create
    /// \param back_and_forth
    /// \returns true if we focused a new workspace, otherwise false
    bool request_workspace(
        Output* output_hint,
        int key,
        bool back_and_forth = true);

    /// Request the workspace by name.
    bool request_workspace(
        Output* output_hint,
        std::string const& name,
        bool back_and_forth = true);

    /// Returns any available workspace with the lowest numerical value starting with 1.
    int request_first_available_workspace(Output* output);

    /// Selects the next workspace after the current selected one.
    bool request_next(std::shared_ptr<Output> const& output);

    /// Selects the workspace before the current selected one
    bool request_prev(std::shared_ptr<Output> const& output);

    bool request_back_and_forth();

    bool request_next_on_output(Output const&);

    bool request_prev_on_output(Output const&);

    bool delete_workspace(uint32_t id);

    /// Focuses the workspace with the provided id
    bool request_focus(uint32_t id);

    /// Returns the workspace with the provided [id], if any.
    Workspace* workspace(uint32_t id) const;

    /// Builds and returns a sorted array of all active workspaces.
    std::vector<Workspace const*> workspaces() const;

private:
    bool focus_existing(Workspace const*, bool back_and_forth);

    /// The number of default workspaces
    const int NUM_DEFAULT_WORKSPACES = 10;
    uint32_t next_id = 0;

    Workspace* workspace(int num) const;
    Workspace* workspace(std::string const& name) const;

    miral::WindowManagerTools tools_;
    WorkspaceObserverRegistrar& registry;
    std::function<Output const*()> get_active;
    std::function<std::vector<std::shared_ptr<Output>>()> get_outputs;
    std::optional<Workspace*> last_selected;
};
}

#endif

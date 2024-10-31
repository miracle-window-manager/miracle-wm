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

#define MIR_LOG_COMPONENT "workspace_manager"
#include "workspace_manager.h"
#include "output.h"
#include "window_helpers.h"
#include <mir/log.h>

using namespace mir::geometry;
using namespace miracle;

WorkspaceManager::WorkspaceManager(
    WindowManagerTools const& tools,
    WorkspaceObserverRegistrar& registry,
    std::function<Output const*()> const& get_active_screen) :
    tools_ { tools },
    registry { registry },
    get_active_screen { get_active_screen }
{
}

std::shared_ptr<Output> WorkspaceManager::request_workspace(std::shared_ptr<Output> const& screen, int key, bool back_and_forth)
{
    if (output_to_workspace_mapping[key] != nullptr)
    {
        auto output = output_to_workspace_mapping[key];
        auto active_workspace = output->get_active_workspace_num();
        if (active_workspace == key)
        {
            if (last_selected)
            {
                // If we reselect the same workspace and we have a previously selected
                // workspace, let's reselect it.
                auto last_output = last_selected->output.lock();
                if (!last_output)
                {
                    last_selected.reset();
                    mir::log_warning("request_workspace: last_selected output unavailable");
                    return nullptr;
                }

                /// If back_and_forth isn't true and we have a last_output, don't visit it.
                if (!back_and_forth)
                    return output;

                return request_workspace(last_output, last_selected->number, back_and_forth);
            }
            else
            {
                mir::log_warning("Same workspace selected twice in a row");
                return output;
            }
        }

        request_focus(key);
        return output;
    }

    output_to_workspace_mapping[key] = screen;
    screen->advise_new_workspace(key);

    request_focus(key);
    registry.advise_created(*output_to_workspace_mapping[key].get(), key);
    return screen;
}

int WorkspaceManager::request_first_available_workspace(std::shared_ptr<Output> const& screen)
{
    for (int i = 1; i < NUM_WORKSPACES; i++)
    {
        if (output_to_workspace_mapping[i] == nullptr)
        {
            request_workspace(screen, i, true);
            return i;
        }
    }

    if (output_to_workspace_mapping[0] == nullptr)
    {
        request_workspace(screen, 0, true);
        return 0;
    }

    return -1;
}

bool WorkspaceManager::request_workspace(std::string const& name, bool back_and_forth)
{
    for (auto const& output : output_to_workspace_mapping)
    {
        if (output == nullptr)
            continue;

        for (auto const& workspace : output->get_workspaces())
        {
            if (workspace->get_name() == name)
            {
                request_workspace(output, workspace->get_workspace(), back_and_forth);
                return true;
            }
        }
    }

    return false;
}

bool WorkspaceManager::request_next(std::shared_ptr<Output> const& output)
{
    int start = output->get_active_workspace_num() + 1;
    if (start == NUM_WORKSPACES)
        start = 0;

    int i = start;
    while (i != output->get_active_workspace_num())
    {
        if (output_to_workspace_mapping[i] != nullptr)
        {
            request_workspace(output_to_workspace_mapping[i], i, true);
            return true;
        }

        i++;

        if (i == NUM_WORKSPACES)
            i = 0;
    }

    return false;
}

bool WorkspaceManager::request_prev(std::shared_ptr<Output> const& output)
{
    int start = output->get_active_workspace_num() - 1;
    if (start == NUM_WORKSPACES)
        start = 0;

    int i = start;
    while (i != output->get_active_workspace_num())
    {
        if (output_to_workspace_mapping[i] != nullptr)
        {
            request_workspace(output_to_workspace_mapping[i], i, true);
            return true;
        }

        i--;

        if (i < 0)
            i = NUM_WORKSPACES - 1;
    }

    return false;
}

bool WorkspaceManager::request_back_and_forth()
{
    if (last_selected)
    {
        if (auto output = last_selected->output.lock())
        {
            request_workspace(output, last_selected->number);
            return true;
        }
    }

    return false;
}

bool WorkspaceManager::request_next_on_output(Output const& output)
{
    int start = output.get_active_workspace_num() + 1;
    if (start == NUM_WORKSPACES)
        start = 0;

    int i = start;
    while (i != output.get_active_workspace_num())
    {
        if (output_to_workspace_mapping[i].get() == &output)
        {
            request_workspace(output_to_workspace_mapping[i], i, true);
            return true;
        }

        i++;

        if (i == NUM_WORKSPACES)
            i = 0;
    }

    return false;
}

bool WorkspaceManager::request_prev_on_output(Output const& output)
{
    int start = output.get_active_workspace_num() - 1;
    if (start == NUM_WORKSPACES)
        start = 0;

    int i = start;
    while (i != output.get_active_workspace_num())
    {
        if (output_to_workspace_mapping[i].get() == &output)
        {
            request_workspace(output_to_workspace_mapping[i], i, true);
            return true;
        }

        i--;

        if (i < 0)
            i = NUM_WORKSPACES - 1;
    }

    return false;
}

bool WorkspaceManager::delete_workspace(int key)
{
    if (output_to_workspace_mapping[key])
    {
        registry.advise_removed(*output_to_workspace_mapping[key].get(), key);
        output_to_workspace_mapping[key]->advise_workspace_deleted(key);
        output_to_workspace_mapping[key] = nullptr;
        return true;
    }

    return false;
}

std::shared_ptr<Output> WorkspaceManager::request_focus(int key)
{
    if (!output_to_workspace_mapping[key])
        return nullptr;

    auto active_screen = get_active_screen();
    int previous_workspace = -1;
    if (active_screen)
    {
        previous_workspace = active_screen->get_active_workspace_num();
        last_selected = { previous_workspace, output_to_workspace_mapping[previous_workspace] };
    }
    output_to_workspace_mapping[key]->advise_workspace_active(key);

    if (active_screen != nullptr)
    {
        registry.advise_focused(output_to_workspace_mapping[previous_workspace].get(), previous_workspace, output_to_workspace_mapping[key].get(), key);
    }
    else
        registry.advise_focused(nullptr, -1, output_to_workspace_mapping[key].get(), key);

    return output_to_workspace_mapping[key];
}

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
#include "vector_helpers.h"
#include <mir/log.h>

using namespace mir::geometry;
using namespace miracle;

WorkspaceManager::WorkspaceManager(
    miral::WindowManagerTools const& tools,
    WorkspaceObserverRegistrar& registry,
    std::function<Output const*()> const& get_active,
    std::function<std::vector<std::shared_ptr<Output>>()> const& get_outputs) :
    tools_ { tools },
    registry { registry },
    get_active { get_active },
    get_outputs { get_outputs }
{
}

bool WorkspaceManager::focus_existing(Workspace const* existing, bool back_and_forth)
{
    auto const& active_workspace = get_active()->active();
    if (active_workspace == existing)
    {
        if (last_selected)
        {
            // If we reselect the same workspace and we have a previously selected
            // workspace, let's reselect it.
            auto last_output = last_selected.value()->get_output();

            /// If back_and_forth isn't true and we have a last_output, don't visit it.
            if (!back_and_forth)
                return false;

            return request_focus(last_selected.value()->id());
        }
        else
            return false;
    }

    request_focus(existing->id());
    return true;
}

bool WorkspaceManager::request_workspace(
    Output* output_hint,
    int num,
    bool back_and_forth)
{
    if (auto const& existing = workspace(num))
        return focus_existing(existing, back_and_forth);

    uint32_t id = next_id++;
    output_hint->advise_new_workspace({ .id = id,
        .num = num });
    request_focus(id);
    registry.advise_created(id);
    return true;
}

bool WorkspaceManager::request_workspace(
    Output* output_hint,
    std::string const& name,
    bool back_and_forth)
{
    if (auto const& existing = workspace(name))
        return focus_existing(existing, back_and_forth);

    uint32_t id = next_id++;
    output_hint->advise_new_workspace({ .id = id,
        .name = name });
    request_focus(id);
    registry.advise_created(id);
    return true;
}

int WorkspaceManager::request_first_available_workspace(Output* output)
{
    for (int i = 1; i < NUM_DEFAULT_WORKSPACES; i++)
    {
        if (auto const& w = workspace(i))
            continue;

        request_workspace(output, i, true);
        return i;
    }

    if (workspace(0) == nullptr)
    {
        request_workspace(output, 0, true);
        return 0;
    }

    return -1;
}

bool WorkspaceManager::request_next(std::shared_ptr<Output> const& output)
{
    auto const& active = output->active();
    if (!active)
        return false;

    auto const l = workspaces();
    for (auto it = l.begin(); it != l.end(); it++)
    {
        if (*it == active)
        {
            it++;
            if (it == l.end())
                it = l.begin();

            return focus_existing(*it, false);
        }
    }

    return false;
}

bool WorkspaceManager::request_prev(std::shared_ptr<Output> const& output)
{
    auto const& active = output->active();
    if (!active)
        return false;

    auto const l = workspaces();
    for (auto it = l.rbegin(); it != l.rend(); it++)
    {
        if (*it == active)
        {
            it++;
            if (it == l.rend())
                it = l.rbegin();

            return focus_existing(*it, false);
        }
    }

    return false;
}

bool WorkspaceManager::request_back_and_forth()
{
    if (last_selected)
    {
        request_focus(last_selected.value()->id());
        return true;
    }

    return false;
}

bool WorkspaceManager::request_next_on_output(Output const& output)
{
    auto const& active = output.active();
    if (!active)
        return false;

    auto const& workspaces = output.get_workspaces();
    for (auto it = workspaces.begin(); it != workspaces.end(); it++)
    {
        if (it->get() == active)
        {
            it++;
            if (it == workspaces.end())
                it = workspaces.begin();

            return focus_existing(it->get(), false);
        }
    }

    return false;
}

bool WorkspaceManager::request_prev_on_output(Output const& output)
{
    auto const& active = output.active();
    if (!active)
        return false;

    auto const& workspaces = output.get_workspaces();
    for (auto it = workspaces.rbegin(); it != workspaces.rend(); it++)
    {
        if (it->get() == active)
        {
            it++;
            if (it == workspaces.rend())
                it = workspaces.rbegin();

            return focus_existing(it->get(), false);
        }
    }

    return false;
}

bool WorkspaceManager::delete_workspace(uint32_t id)
{
    auto const* w = workspace(id);
    if (!w)
        return false;

    auto* output = w->get_output();
    output->advise_workspace_deleted(id);
    registry.advise_removed(id);
    return true;
}

bool WorkspaceManager::request_focus(uint32_t id)
{
    auto const& existing = workspace(id);
    if (!existing)
        return false;

    auto active_screen = get_active();
    if (active_screen)
        last_selected = active_screen->active();
    else
        last_selected = nullptr;

    existing->get_output()->advise_workspace_active(id);

    if (active_screen != nullptr)
        registry.advise_focused(last_selected.value()->id(), id);
    else
        registry.advise_focused(std::nullopt, id);

    return true;
}

Workspace* WorkspaceManager::workspace(int num) const
{
    for (auto const& output : get_outputs())
    {
        for (auto const& workspace : output->get_workspaces())
        {
            if (workspace->num() == num)
                return workspace.get();
        }
    }

    return nullptr;
}

Workspace* WorkspaceManager::workspace(uint32_t id) const
{
    for (auto const& output : get_outputs())
    {
        for (auto const& workspace : output->get_workspaces())
        {
            if (workspace->id() == id)
                return workspace.get();
        }
    }

    return nullptr;
}

Workspace* WorkspaceManager::workspace(std::string const& name) const
{
    for (auto const& output : get_outputs())
    {
        for (auto const& workspace : output->get_workspaces())
        {
            if (workspace->name() == name)
                return workspace.get();
        }
    }

    return nullptr;
}

std::vector<Workspace const*> WorkspaceManager::workspaces() const
{
    std::vector<Workspace const*> result;
    for (auto const& output : get_outputs())
    {
        for (auto const& w : output->get_workspaces())
        {
            auto const* ptr = w.get();
            insert_sorted(result, ptr, [](Workspace const* a, Workspace const* b)
            {
                if (a->num() && b->num())
                    return a->num().value() < b->num().value();
                else if (a->num())
                    return true;
                else if (b->num())
                    return false;
                else
                    return true;
            });
        }
    }
    return result;
}

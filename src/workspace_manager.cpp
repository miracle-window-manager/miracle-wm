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
#include "config.h"
#include "output_interface.h"
#include "output_manager.h"
#include "vector_helpers.h"
#include <mir/log.h>

using namespace mir::geometry;
using namespace miracle;

WorkspaceManager::WorkspaceManager(
    std::shared_ptr<WorkspaceObserverRegistrar> const& registry,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<OutputManager> const& output_manager) :
    registry { registry },
    config { config },
    output_manager { output_manager }
{
}

bool WorkspaceManager::focus_existing(WorkspaceInterface const* existing, bool back_and_forth)
{
    auto const& active_workspace = output_manager->focused()->active();
    if (active_workspace.get() == existing)
    {
        if (!last_selected.expired())
        {
            /// If back_and_forth isn't true and we have a last_output, don't visit it.
            if (!back_and_forth)
                return false;

            return request_focus(last_selected.lock()->id());
        }
        else
            return false;
    }

    request_focus(existing->id());
    return true;
}

bool WorkspaceManager::request_workspace(
    OutputInterface* output_hint,
    int num,
    bool back_and_forth)
{
    if (auto const& existing = workspace(num))
        return focus_existing(existing, back_and_forth);

    uint32_t id = next_id++;
    auto const& workspace_config = config->get_workspace_config(num, std::nullopt);
    output_hint->advise_new_workspace({ .id = id,
        .num = num,
        .name = workspace_config.name });
    registry->advise_created(id);
    request_focus(id);
    return true;
}

bool WorkspaceManager::request_workspace(
    OutputInterface* output_hint,
    std::string const& name,
    bool back_and_forth)
{
    if (auto const& existing = workspace(name))
        return focus_existing(existing, back_and_forth);

    uint32_t id = next_id++;
    output_hint->advise_new_workspace({ .id = id,
        .name = name });
    request_focus(id);
    registry->advise_created(id);
    return true;
}

int WorkspaceManager::request_first_available_workspace(OutputInterface* output)
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

bool WorkspaceManager::request_next(OutputInterface* output)
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

            return focus_existing(it->get(), false);
        }
    }

    return false;
}

bool WorkspaceManager::request_prev(OutputInterface* output)
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

            return focus_existing(it->get(), false);
        }
    }

    return false;
}

bool WorkspaceManager::request_back_and_forth()
{
    if (!last_selected.expired())
    {
        request_focus(last_selected.lock()->id());
        return true;
    }

    return false;
}

bool WorkspaceManager::request_next_on_output(OutputInterface const& output)
{
    auto const& active = output.active();
    if (!active)
        return false;

    auto const& workspaces = output.get_workspaces();
    for (auto it = workspaces.begin(); it != workspaces.end(); it++)
    {
        if (*it == active)
        {
            it++;
            if (it == workspaces.end())
                it = workspaces.begin();

            return focus_existing(it->get(), false);
        }
    }

    return false;
}

bool WorkspaceManager::request_prev_on_output(OutputInterface const& output)
{
    auto const& active = output.active();
    if (!active)
        return false;

    auto const& workspaces = output.get_workspaces();
    for (auto it = workspaces.rbegin(); it != workspaces.rend(); it++)
    {
        if (*it == active)
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

    registry->advise_removed(id);
    auto* output = w->get_output();
    output->advise_workspace_deleted(*this, id);
    return true;
}

bool WorkspaceManager::request_focus(uint32_t id)
{
    auto const& existing = workspace(id);
    if (!existing)
        return false;

    auto active_screen = output_manager->focused();
    if (active_screen)
    {
        if (auto current = active_screen->active())
            last_selected = current;
        else
            last_selected.reset();
    }
    else
        last_selected.reset();

    // Note: it is important that this is sent before the workspace
    // is activated because 'advise_workspace-active' might remove
    // the workspace if it is empty
    if (active_screen != nullptr && !last_selected.expired())
        registry->advise_focused(last_selected.lock()->id(), id);
    else
        registry->advise_focused(std::nullopt, id);

    existing->get_output()->advise_workspace_active(*this, id);
    existing->select_first_window();
    return true;
}

WorkspaceInterface* WorkspaceManager::workspace(int num) const
{
    for (auto const& output : output_manager->outputs())
    {
        for (auto const& workspace : output->get_workspaces())
        {
            if (workspace->num() == num)
                return workspace.get();
        }
    }

    return nullptr;
}

WorkspaceInterface* WorkspaceManager::workspace(uint32_t id) const
{
    for (auto const& output : output_manager->outputs())
    {
        for (auto const& workspace : output->get_workspaces())
        {
            if (workspace->id() == id)
                return workspace.get();
        }
    }

    return nullptr;
}

WorkspaceInterface* WorkspaceManager::workspace(std::string const& name) const
{
    for (auto const& output : output_manager->outputs())
    {
        for (auto const& workspace : output->get_workspaces())
        {
            if (workspace->name() == name)
                return workspace.get();
        }
    }

    return nullptr;
}

std::vector<std::shared_ptr<WorkspaceInterface>> WorkspaceManager::workspaces() const
{
    std::vector<std::shared_ptr<WorkspaceInterface>> result;
    for (auto const& output : output_manager->outputs())
    {
        for (auto const& w : output->get_workspaces())
        {
            insert_sorted(result, w, [](
                std::shared_ptr<WorkspaceInterface> const& a,
                std::shared_ptr<WorkspaceInterface> const& b)
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

void WorkspaceManager::move_workspace_to_output(uint32_t id, OutputInterface* hint)
{
    auto w = workspace(id);
    if (!w)
    {
        mir::log_error("move_workspace_to_output: cannot find workspace with id %d", id);
        return;
    }

    hint->move_workspace_to(*this, w);
}
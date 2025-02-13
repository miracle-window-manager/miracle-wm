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

#include "output_manager.h"
#include "output_factory_interface.h"
#include "output_interface.h"
#include "workspace_manager.h"

using namespace miracle;

OutputManager::OutputManager(
    std::unique_ptr<OutputFactoryInterface> output_factory) :
    output_factory(std::move(output_factory))
{
}

OutputInterface* OutputManager::create(
    std::string name, int id, mir::geometry::Rectangle area, WorkspaceManager& workspace_manager)
{
    if (outputs_.size() == 1 && outputs_[0]->is_defunct())
    {
        outputs_[0]->unset_defunct();
        outputs_[0]->set_info(id, name);
        outputs_[0]->update_area(area);
    }
    else
    {
        outputs_.push_back(output_factory->create(name, id, area));
        workspace_manager.request_first_available_workspace(outputs_.back().get());
    }

    if (focused_ == nullptr)
        focus(outputs_.back()->id());

    return outputs_.back().get();
}

void OutputManager::update(int id, mir::geometry::Rectangle area)
{
    for (auto const& output : outputs_)
    {
        if (output->id() == id)
        {
            output->update_area(area);
            return;
        }
    }
}

bool OutputManager::remove(int id, WorkspaceManager& workspace_manager)
{
    for (auto it = outputs_.begin(); it != outputs_.end(); it++)
    {
        auto const& output = *it;
        if (output->id() != id)
            continue;

        if (output.get() == focused_)
            unfocus(id);

        if (outputs_.size() == 1)
        {
            outputs_[0]->set_defunct();
        }
        else
        {
            // Find the workspace ids
            std::vector<size_t> workspaces(output->get_workspaces().size());
            for (auto const& workspace : output->get_workspaces())
                workspaces.push_back(workspace->id());

            // Find the next available output
            auto next_it = it + 1;
            if (next_it == outputs_.end())
                next_it = outputs_.begin();

            // Move workspaces to the next available output
            for (auto workspace_id : workspaces)
                workspace_manager.move_workspace_to_output(workspace_id, next_it->get());

            focus(next_it->get()->id());
            outputs_.erase(it);
        }
        return true;
    }

    return false;
}

std::vector<std::unique_ptr<OutputInterface>> const& OutputManager::outputs() const
{
    return outputs_;
}

bool OutputManager::focus(int id)
{
    for (auto const& output : outputs_)
    {
        if (output->id() == id)
        {
            focused_ = output.get();
            return true;
        }
    }

    return false;
}

bool OutputManager::unfocus(int id)
{
    if (focused_ == nullptr)
        return false;

    if (focused_->id() != id)
        return false;

    focused_ = nullptr;
    return true;
}

OutputInterface* OutputManager::focused()
{
    return focused_;
}
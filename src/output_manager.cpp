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
#include "output.h"
#include "output_factory.h"

using namespace miracle;

OutputManager::OutputManager(
    std::unique_ptr<OutputFactory> output_factory) :
    output_factory(std::move(output_factory))
{
}

Output* OutputManager::create(
    std::string name, int id, mir::geometry::Rectangle area)
{
    if (outputs_.size() == 1 && outputs_[0]->is_defunct())
    {
        outputs_[0]->unset_defunct();
        outputs_[0]->set_info(id, name);
        outputs_[0]->update_area(area);
    }
    else
    {
        outputs_.push_back(output_factory->create(name, id, area, this));
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

bool OutputManager::remove(int id)
{
    for (auto it = outputs_.begin(); it != outputs_.end(); it++)
    {
        auto const& other_output = *it;
        if (other_output->id() == id)
        {
            if (other_output.get() == focused_)
                unfocus(id);

            if (outputs_.size() == 1)
            {
                outputs_[0]->set_defunct();
            }
            else
            {
                // TODO: !
            }
            return true;
        }
    }

    return false;
}

std::vector<std::unique_ptr<Output>> const& OutputManager::outputs() const
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

Output* OutputManager::focused()
{
    return focused_;
}
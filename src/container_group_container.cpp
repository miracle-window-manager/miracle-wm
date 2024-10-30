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

#include "container_group_container.h"
#include "compositor_state.h"
#include "output.h"
#include "workspace.h"

namespace miracle
{
ContainerGroupContainer::ContainerGroupContainer(CompositorState& state) :
    state { state }
{
}

void ContainerGroupContainer::add(std::shared_ptr<Container> const& container)
{
    containers.push_back(container);
}

void ContainerGroupContainer::remove(std::shared_ptr<Container> const& container)
{
    containers.erase(std::remove_if(
                         containers.begin(),
                         containers.end(),
                         [&](std::weak_ptr<Container> const& weak_container)
    {
        return weak_container.expired() || weak_container.lock() == container;
    }),
        containers.end());
}

bool ContainerGroupContainer::contains(std::shared_ptr<Container const> const& container) const
{
    for (auto const& other : containers)
    {
        if (!other.expired() && other.lock() == container)
            return true;
    }

    return false;
}

ContainerType ContainerGroupContainer::get_type() const
{
    return ContainerType::group;
}

void ContainerGroupContainer::show()
{
    for (auto const& container : containers)
    {
        if (auto c = container.lock())
            c->show();
    }
}

void ContainerGroupContainer::hide()
{
    for (auto const& container : containers)
    {
        if (auto c = container.lock())
            c->hide();
    }
}

void ContainerGroupContainer::commit_changes()
{
    for (auto const& container : containers)
    {
        if (auto c = container.lock())
            c->commit_changes();
    }
}

mir::geometry::Rectangle ContainerGroupContainer::get_logical_area() const
{
    return {};
}

void ContainerGroupContainer::set_logical_area(mir::geometry::Rectangle const& rectangle)
{
}

mir::geometry::Rectangle ContainerGroupContainer::get_visible_area() const
{
    return {};
}

void ContainerGroupContainer::constrain()
{
    for (auto const& container : containers)
    {
        if (auto c = container.lock())
            c->constrain();
    }
}

std::weak_ptr<ParentContainer> ContainerGroupContainer::get_parent() const
{
    return {};
}

void ContainerGroupContainer::set_parent(std::shared_ptr<ParentContainer> const& ptr)
{
    throw std::logic_error("Cannot set-parent on ContainerGroup");
}

size_t ContainerGroupContainer::get_min_height() const
{
    return 0;
}

size_t ContainerGroupContainer::get_min_width() const
{
    return 0;
}

void ContainerGroupContainer::handle_ready()
{
    throw std::logic_error("handle_ready should not be called on a ContainerGroup");
}

void ContainerGroupContainer::handle_modify(miral::WindowSpecification const& specification)
{
    for (auto const& container : containers)
    {
        if (auto c = container.lock())
            c->handle_modify(specification);
    }
}

void ContainerGroupContainer::handle_request_move(MirInputEvent const* input_event)
{
    for (auto const& container : containers)
    {
        if (auto c = container.lock())
            c->handle_request_move(input_event);
    }
}

void ContainerGroupContainer::handle_request_resize(MirInputEvent const* input_event, MirResizeEdge edge)
{
    for (auto const& container : containers)
    {
        if (auto c = container.lock())
            c->handle_request_resize(input_event, edge);
    }
}

void ContainerGroupContainer::handle_raise()
{
    for (auto const& container : containers)
    {
        if (auto c = container.lock())
            c->handle_raise();
    }
}

bool ContainerGroupContainer::resize(Direction direction)
{
    bool result = true;
    for (auto const& container : containers)
    {
        if (auto c = container.lock())
            result &= c->resize(direction);
    }
    return result;
}

bool ContainerGroupContainer::toggle_fullscreen()
{
    bool result = true;
    for (auto const& container : containers)
    {
        if (auto c = container.lock())
            result &= c->toggle_fullscreen();
    }
    return result;
}

void ContainerGroupContainer::request_horizontal_layout()
{
    for (auto const& container : containers)
    {
        if (auto c = container.lock())
            c->request_horizontal_layout();
    }
}

void ContainerGroupContainer::request_vertical_layout()
{
    for (auto const& container : containers)
    {
        if (auto c = container.lock())
            c->request_vertical_layout();
    }
}

void ContainerGroupContainer::toggle_layout(bool cycle_thru_all)
{
    for (auto const& container : containers)
    {
        if (auto c = container.lock())
            c->toggle_layout(cycle_thru_all);
    }
}

void ContainerGroupContainer::on_open()
{
}

void ContainerGroupContainer::on_focus_gained()
{
}

void ContainerGroupContainer::on_focus_lost()
{
}

void ContainerGroupContainer::on_move_to(mir::geometry::Point const& top_left)
{
}

mir::geometry::Rectangle
ContainerGroupContainer::confirm_placement(MirWindowState state, mir::geometry::Rectangle const& rectangle)
{
    return {};
}

Workspace* ContainerGroupContainer::get_workspace() const
{
    return state.active_output->get_active_workspace().get();
}

Output* ContainerGroupContainer::get_output() const
{
    return state.active_output.get();
}

glm::mat4 ContainerGroupContainer::get_transform() const
{
    return glm::mat4(1.f);
}

void ContainerGroupContainer::set_transform(glm::mat4 transform)
{
}

glm::mat4 ContainerGroupContainer::get_workspace_transform() const
{
    return glm::mat4(1.f);
}

glm::mat4 ContainerGroupContainer::get_output_transform() const
{
    return glm::mat4(1.f);
}

uint32_t ContainerGroupContainer::animation_handle() const
{
    return 0;
}

void ContainerGroupContainer::animation_handle(uint32_t uint_32)
{
}

bool ContainerGroupContainer::is_focused() const
{
    return state.active.get() == this;
}

bool ContainerGroupContainer::is_fullscreen() const
{
    return false;
}

std::optional<miral::Window> ContainerGroupContainer::window() const
{
    return std::nullopt;
}

bool ContainerGroupContainer::select_next(Direction direction)
{
    return false;
}

bool ContainerGroupContainer::pinned() const
{
    return false;
}

bool ContainerGroupContainer::pinned(bool b)
{
    return false;
}

bool ContainerGroupContainer::move(Direction direction)
{
    bool result = true;
    for (auto const& container : containers)
    {
        if (auto c = container.lock())
            result &= c->move(direction);
    }
    return result;
}

bool ContainerGroupContainer::move_by(Direction direction, int pixels)
{
    bool result = true;
    for (auto const& container : containers)
    {
        if (auto c = container.lock())
            result &= c->move_by(direction, pixels);
    }
    return result;
}

bool ContainerGroupContainer::move_to(int x, int y)
{
    bool result = true;
    for (auto const& container : containers)
    {
        if (auto c = container.lock())
            result &= c->move_to(x, y);
    }
    return result;
}
} // miracle
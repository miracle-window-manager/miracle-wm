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

#ifndef MIRACLE_WM_WORKSPACE_INTERFACE_H
#define MIRACLE_WM_WORKSPACE_INTERFACE_H

#include "container.h"
#include "direction.h"

#include <glm/glm.hpp>
#include <memory>
#include <miral/window_manager_tools.h>

namespace miracle
{
class OutputInterface;
class Container;
class ParentContainer;

struct AllocationHint
{
    ContainerType container_type = ContainerType::none;
    std::optional<std::shared_ptr<ParentContainer>> parent;
};

class WorkspaceInterface
{
public:
    virtual ~WorkspaceInterface() = default;

    virtual void set_area(mir::geometry::Rectangle const&) = 0;
    virtual void recalculate_area() = 0;

    virtual AllocationHint allocate_position(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification& requested_specification,
        AllocationHint const& hint)
        = 0;

    virtual std::shared_ptr<Container> create_container(
        miral::WindowInfo const& window_info, AllocationHint const& type)
        = 0;

    virtual void delete_container(std::shared_ptr<Container> const& container) = 0;
    virtual bool move_container(Direction direction, Container&) = 0;
    virtual bool add_to_root(Container& to_move) = 0;
    virtual void show() = 0;
    virtual void hide() = 0;

    virtual void transfer_pinned_windows_to(std::shared_ptr<WorkspaceInterface> const& other) = 0;

    /// Iterates all containers on this workspace that represent a window until the predicate is satisfied.
    /// Returns true if the predicate returned true.
    virtual bool for_each_window(std::function<bool(std::shared_ptr<Container>)> const&) const = 0;

    /// Creates a new floating tree on this workspace. The tree is empty by default
    /// and must be filled in by subsequent calls, lest it become a zombie tree with
    /// zero sub containers.
    virtual std::shared_ptr<ParentContainer> create_floating_tree(mir::geometry::Rectangle const& area) = 0;

    virtual void advise_focus_gained(std::shared_ptr<Container> const& container) = 0;

    virtual void select_first_window() = 0;

    [[nodiscard]] virtual OutputInterface* get_output() const = 0;

    virtual void set_output(OutputInterface*) = 0;

    [[deprecated("Do not use unless you have a very good reason to do so!")]]
    virtual void workspace_transform_change_hack()
        = 0;

    [[nodiscard]] virtual bool is_empty() const = 0;
    virtual void graft(std::shared_ptr<Container> const&) = 0;

    [[nodiscard]] virtual uint32_t id() const = 0;
    [[nodiscard]] virtual std::optional<int> num() const = 0;
    [[nodiscard]] virtual nlohmann::json to_json(bool is_output_focused) const = 0;
    [[nodiscard]] virtual std::optional<std::string> const& name() const = 0;
    [[nodiscard]] virtual std::string display_name() const = 0;
    [[nodiscard]] virtual std::shared_ptr<ParentContainer> get_root() const = 0;
};
}

#endif // MIRACLE_WM_WORKSPACE_INTERFACE_H

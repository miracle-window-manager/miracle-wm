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

#ifndef MIRACLE_SCREEN_H
#define MIRACLE_SCREEN_H

#include "animator.h"
#include "direction.h"
#include "miral/window.h"
#include "tiling_window_tree.h"

#include "minimal_window_manager.h"
#include "workspace.h"
#include <memory>
#include <miral/output.h>
#include <nlohmann/json.hpp>

namespace miracle
{
class WorkspaceManager;
class Config;
class WindowManagerToolsWindowController;
class CompositorState;
class Animator;

struct WorkspaceCreationData
{
    uint32_t id;
    std::optional<int> num;
    std::optional<std::string> name;
};

class Output
{
public:
    virtual ~Output() = default;

    virtual std::shared_ptr<Container> intersect(float x, float y) = 0;
    /// Ignores all other windows and checks for intersections within the tiling grid. If
    /// [ignore_selected] is true, then the active window will not be intersected.
    virtual std::shared_ptr<Container> intersect_leaf(float x, float y, bool ignore_selected) = 0;
    virtual AllocationHint allocate_position(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification& requested_specification,
        AllocationHint hint = AllocationHint())
        = 0;
    [[nodiscard]] virtual std::shared_ptr<Container> create_container(
        miral::WindowInfo const& window_info, AllocationHint const& hint) const
        = 0;
    virtual void delete_container(std::shared_ptr<Container> const& container) = 0;
    virtual void advise_new_workspace(WorkspaceCreationData const&&) = 0;
    virtual void advise_workspace_deleted(WorkspaceManager& workspace_manager, uint32_t id) = 0;
    virtual bool advise_workspace_active(WorkspaceManager& workspace_manager, uint32_t id) = 0;
    virtual void advise_application_zone_create(miral::Zone const& application_zone) = 0;
    virtual void advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original) = 0;
    virtual void advise_application_zone_delete(miral::Zone const& application_zone) = 0;
    virtual void move_workspace_to(WorkspaceManager& workspace_manager, Workspace* workspace) = 0;
    virtual bool point_is_in_output(int x, int y) = 0;
    virtual void update_area(geom::Rectangle const& area) = 0;

    /// Immediately requests that the provided window be added to the output
    /// with the provided type. This is a deviation away from the typical
    /// window-adding flow where you first call 'place_new_window' followed
    /// by 'create_container'.
    virtual void add_immediately(miral::Window& window, AllocationHint hint = AllocationHint()) = 0;

    /// Takes an existing [Container] object and places it in an appropriate position
    /// on the active [Workspace].
    virtual void graft(std::shared_ptr<Container> const& container) = 0;
    virtual void set_transform(glm::mat4 const& in) = 0;
    virtual void set_position(glm::vec2 const&) = 0;

    /// Set the [id] and [name] associated with this output.
    virtual void set_info(int id, std::string name) = 0;

    /// A defunct output is one that has "technically" been removed, but in practice it is still waiting
    /// around to be reassociated with a "true" output.
    virtual void set_defunct() = 0;
    virtual void unset_defunct() = 0;

    // Getters
    [[nodiscard]] virtual std::vector<miral::Window> collect_all_windows() const = 0;
    [[nodiscard]] virtual Workspace* active() const = 0;
    [[nodiscard]] virtual std::vector<std::shared_ptr<Workspace>> const& get_workspaces() const = 0;
    [[nodiscard]] virtual geom::Rectangle const& get_area() const = 0;
    [[nodiscard]] virtual std::vector<miral::Zone> const& get_app_zones() const = 0;
    [[nodiscard]] virtual int id() const = 0;
    [[nodiscard]] virtual std::string const& name() const = 0;
    [[nodiscard]] virtual bool is_defunct() const = 0;
    [[nodiscard]] virtual glm::mat4 get_transform() const = 0;
    [[nodiscard]] virtual geom::Rectangle get_workspace_rectangle(size_t i) const = 0;
    [[nodiscard]] virtual Workspace const* workspace(uint32_t id) const = 0;
    [[nodiscard]] virtual nlohmann::json to_json() const = 0;
};

}

#endif

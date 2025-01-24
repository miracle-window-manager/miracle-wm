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
    virtual void advise_workspace_deleted(uint32_t id) = 0;
    virtual bool advise_workspace_active(uint32_t id) = 0;
    virtual void advise_application_zone_create(miral::Zone const& application_zone) = 0;
    virtual void advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original) = 0;
    virtual void advise_application_zone_delete(miral::Zone const& application_zone) = 0;
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

    // Getters
    [[nodiscard]] virtual std::vector<miral::Window> collect_all_windows() const = 0;
    [[nodiscard]] virtual Workspace* active() const = 0;
    [[nodiscard]] virtual std::vector<std::shared_ptr<Workspace>> const& get_workspaces() const = 0;
    [[nodiscard]] virtual geom::Rectangle const& get_area() const = 0;
    [[nodiscard]] virtual std::vector<miral::Zone> const& get_app_zones() const = 0;
    [[nodiscard]] virtual int id() const = 0;
    [[nodiscard]] virtual std::string const& name() const = 0;
    [[nodiscard]] virtual bool is_active() const = 0;
    [[nodiscard]] virtual glm::mat4 get_transform() const = 0;
    [[nodiscard]] virtual geom::Rectangle get_workspace_rectangle(size_t i) const = 0;
    [[nodiscard]] virtual Workspace const* workspace(uint32_t id) const = 0;
    [[nodiscard]] virtual nlohmann::json to_json() const = 0;
};

class MiralWrapperOutput : public Output
{
public:
    MiralWrapperOutput(
        std::string name,
        int id,
        WorkspaceManager& workspace_manager,
        geom::Rectangle const& area,
        std::shared_ptr<MinimalWindowManager> const& floating_window_manager,
        CompositorState& state,
        std::shared_ptr<Config> const& options,
        WindowController&,
        Animator&);
    ~MiralWrapperOutput();

    std::shared_ptr<Container> intersect(float x, float y) override;
    /// Ignores all other windows and checks for intersections within the tiling grid. If
    /// [ignore_selected] is true, then the active window will not be intersected.
    std::shared_ptr<Container> intersect_leaf(float x, float y, bool ignore_selected) override;
    AllocationHint allocate_position(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification& requested_specification,
        AllocationHint hint = AllocationHint()) override;
    [[nodiscard]] std::shared_ptr<Container> create_container(
        miral::WindowInfo const& window_info, AllocationHint const& hint) const override;
    void delete_container(std::shared_ptr<Container> const& container) override;
    void advise_new_workspace(WorkspaceCreationData const&&) override;
    void advise_workspace_deleted(uint32_t id) override;
    bool advise_workspace_active(uint32_t id) override;
    void advise_application_zone_create(miral::Zone const& application_zone) override;
    void advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original) override;
    void advise_application_zone_delete(miral::Zone const& application_zone) override;
    bool point_is_in_output(int x, int y) override;
    void update_area(geom::Rectangle const& area) override;
    void add_immediately(miral::Window& window, AllocationHint hint = AllocationHint()) override;
    void graft(std::shared_ptr<Container> const& container) override;
    void set_transform(glm::mat4 const& in) override;
    void set_position(glm::vec2 const&) override;

    // Getters

    [[nodiscard]] std::vector<miral::Window> collect_all_windows() const override;
    [[nodiscard]] Workspace* active() const override;
    [[nodiscard]] std::vector<std::shared_ptr<Workspace>> const& get_workspaces() const override { return workspaces; }
    [[nodiscard]] geom::Rectangle const& get_area() const override { return area; }
    [[nodiscard]] std::vector<miral::Zone> const& get_app_zones() const override { return application_zone_list; }
    [[nodiscard]] std::string const& name() const override { return name_; }
    [[nodiscard]] int id() const override { return id_; }
    [[nodiscard]] bool is_active() const override;
    [[nodiscard]] glm::mat4 get_transform() const override;
    /// Gets the relative position of the current rectangle (e.g. the active
    /// rectangle with be at position (0, 0))
    [[nodiscard]] geom::Rectangle get_workspace_rectangle(size_t i) const override;
    [[nodiscard]] Workspace const* workspace(uint32_t id) const override;
    [[nodiscard]] nlohmann::json to_json() const override;

private:
    class WorkspaceAnimation : public Animation
    {
    public:
        WorkspaceAnimation(
            AnimationHandle handle,
            AnimationDefinition definition,
            mir::geometry::Rectangle const& from,
            mir::geometry::Rectangle const& to,
            mir::geometry::Rectangle const& current,
            std::shared_ptr<Workspace> const& to_workspace,
            std::shared_ptr<Workspace> const& from_workspace,
            MiralWrapperOutput* output);

        void on_tick(AnimationStepResult const&) override;

    private:
        std::shared_ptr<Workspace> to_workspace;
        std::shared_ptr<Workspace> from_workspace;
        MiralWrapperOutput* output;
    };

    void on_workspace_animation(
        AnimationStepResult const& result,
        std::shared_ptr<Workspace> const& to,
        std::shared_ptr<Workspace> const& from);

    std::string name_;
    int id_;
    WorkspaceManager& workspace_manager;
    std::shared_ptr<MinimalWindowManager> floating_window_manager;
    CompositorState& state;
    geom::Rectangle area;
    std::shared_ptr<Config> config;
    WindowController& window_controller;
    Animator& animator;
    std::weak_ptr<Workspace> active_workspace;
    std::vector<std::shared_ptr<Workspace>> workspaces;
    std::vector<miral::Zone> application_zone_list;
    bool is_active_ = false;
    AnimationHandle handle;

    /// The position of the output for scrolling across workspaces
    glm::vec2 position_offset = glm::vec2(0.f);

    /// The transform applied to the workspace
    glm::mat4 transform = glm::mat4(1.f);

    /// A matrix resulting from combining position + transform
    glm::mat4 final_transform = glm::mat4(1.f);
};

}

#endif

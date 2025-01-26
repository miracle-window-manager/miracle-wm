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

#ifndef MIRACLE_WM_MIRAL_OUTPUT_H
#define MIRACLE_WM_MIRAL_OUTPUT_H

#include "output.h"

namespace miracle
{
class MiralWrapperOutput : public Output
{
public:
    MiralWrapperOutput(
        std::string name,
        int id,
        geom::Rectangle const& area,
        std::shared_ptr<MinimalWindowManager> const& floating_window_manager,
        CompositorState& state,
        OutputManager* output_manager,
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
    void advise_workspace_deleted(WorkspaceManager& workspace_manager, uint32_t id) override;
    bool advise_workspace_active(WorkspaceManager& workspace_manager, uint32_t id) override;
    void advise_application_zone_create(miral::Zone const& application_zone) override;
    void advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original) override;
    void advise_application_zone_delete(miral::Zone const& application_zone) override;
    bool point_is_in_output(int x, int y) override;
    void update_area(geom::Rectangle const& area) override;
    void add_immediately(miral::Window& window, AllocationHint hint = AllocationHint()) override;
    void graft(std::shared_ptr<Container> const& container) override;
    void set_transform(glm::mat4 const& in) override;
    void set_position(glm::vec2 const&) override;
    void set_info(int id, std::string name) override;
    void set_defunct() override;
    void unset_defunct() override;

    // Getters

    [[nodiscard]] std::vector<miral::Window> collect_all_windows() const override;
    [[nodiscard]] Workspace* active() const override;
    [[nodiscard]] std::vector<std::shared_ptr<Workspace>> const& get_workspaces() const override { return workspaces; }
    [[nodiscard]] geom::Rectangle const& get_area() const override { return area; }
    [[nodiscard]] std::vector<miral::Zone> const& get_app_zones() const override { return application_zone_list; }
    [[nodiscard]] std::string const& name() const override { return name_; }
    [[nodiscard]] bool is_defunct() const override { return is_defunct_; }
    [[nodiscard]] int id() const override { return id_; }
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
    std::shared_ptr<MinimalWindowManager> floating_window_manager;
    CompositorState& state;
    geom::Rectangle area;
    std::shared_ptr<Config> config;
    OutputManager* output_manager;
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

    bool is_defunct_ = false;
};
}

#endif // MIRACLE_WM_MIRAL_OUTPUT_H

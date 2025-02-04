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

#ifndef MIRACLEWM_WORKSPACE_CONTENT_H
#define MIRACLEWM_WORKSPACE_CONTENT_H

#include "workspace_interface.h"

#include <glm/glm.hpp>
#include <memory>
#include <miral/window_manager_tools.h>

namespace miracle
{
class OutputManager;
class WindowController;
class Config;
class CompositorState;

class Workspace : public WorkspaceInterface
{
public:
    Workspace(
        OutputInterface* output,
        uint32_t id,
        std::optional<int> num,
        std::optional<std::string> name,
        std::shared_ptr<Config> const& config,
        std::shared_ptr<WindowController> const& window_controller,
        std::shared_ptr<CompositorState> const& state);
    ~Workspace() override;

    void set_area(mir::geometry::Rectangle const&) override;
    void recalculate_area() override;

    AllocationHint allocate_position(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification& requested_specification,
        AllocationHint const& hint) override;
    std::shared_ptr<Container> create_container(
        miral::WindowInfo const& window_info, AllocationHint const& type) override;
    void delete_container(std::shared_ptr<Container> const& container) override;
    bool move_container(Direction direction, Container&) override;
    bool add_to_root(Container& to_move) override;
    void show() override;
    void hide() override;
    void transfer_pinned_windows_to(std::shared_ptr<WorkspaceInterface> const& other) override;
    bool for_each_window(std::function<bool(std::shared_ptr<Container>)> const&) const override;
    std::shared_ptr<ParentContainer> create_floating_tree(mir::geometry::Rectangle const& area) override;
    void advise_focus_gained(std::shared_ptr<Container> const& container) override;
    void select_first_window() override;
    OutputInterface* get_output() const override;
    void set_output(OutputInterface*) override;
    void workspace_transform_change_hack() override;
    [[nodiscard]] bool is_empty() const override;
    void graft(std::shared_ptr<Container> const&) override;
    [[nodiscard]] uint32_t id() const override { return id_; }
    [[nodiscard]] std::optional<int> num() const override { return num_; }
    [[nodiscard]] nlohmann::json to_json(bool is_output_focused) const override;
    [[nodiscard]] std::optional<std::string> const& name() const override { return name_; }
    [[nodiscard]] std::string display_name() const override;
    [[nodiscard]] std::shared_ptr<ParentContainer> get_root() const override { return root; }

private:
    struct MoveResult
    {
        enum
        {
            traversal_type_invalid,
            traversal_type_insert,
            traversal_type_prepend,
            traversal_type_append
        } traversal_type
            = traversal_type_invalid;
        std::shared_ptr<Container> node = nullptr;
    };

    OutputInterface* output;
    uint32_t id_;
    std::optional<int> num_;
    std::optional<std::string> name_;
    std::shared_ptr<ParentContainer> root;
    std::vector<std::shared_ptr<ParentContainer>> floating_trees;
    std::shared_ptr<WindowController> window_controller;
    std::shared_ptr<CompositorState> const& state;
    std::shared_ptr<Config> config;
    std::weak_ptr<Container> last_selected_container;
    int config_handle = 0;

    /// Retrieves the container that is currently being used for layout
    std::shared_ptr<ParentContainer> get_layout_container();

    /// From the provided node, find the next node in the provided direction.
    /// This method is guaranteed to return a Window node, not a Lane.
    MoveResult handle_move(Container& from, Direction direction);
};

} // miracle

#endif // MIRACLEWM_WORKSPACE_CONTENT_H

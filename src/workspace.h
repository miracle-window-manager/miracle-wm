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

#include "animator.h"
#include "container.h"
#include "direction.h"

#include "minimal_window_manager.h"
#include <glm/glm.hpp>
#include <memory>
#include <miral/window_manager_tools.h>

namespace miracle
{
class Output;
class Config;
class TilingWindowTree;
class WindowController;
class CompositorState;
class ParentContainer;
class FloatingWindowContainer;
class FloatingTreeContainer;

struct AllocationHint
{
    ContainerType container_type = ContainerType::none;
    TilingWindowTree* placement_tree = nullptr;
};

class Workspace
{
public:
    Workspace(
        Output* output,
        miral::WindowManagerTools const& tools,
        uint32_t id,
        std::optional<int> num,
        std::optional<std::string> name,
        std::shared_ptr<Config> const& config,
        WindowController& window_controller,
        CompositorState const& state,
        std::shared_ptr<MinimalWindowManager> const& floating_window_manager);

    void set_area(mir::geometry::Rectangle const&);
    void recalculate_area();

    AllocationHint allocate_position(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification& requested_specification,
        AllocationHint const& hint);
    std::shared_ptr<Container> create_container(
        miral::WindowInfo const& window_info, AllocationHint const& type);
    void handle_ready_hack(LeafContainer& container);
    void delete_container(std::shared_ptr<Container> const& container);
    void show();
    void hide();
    void transfer_pinned_windows_to(std::shared_ptr<Workspace> const& other);
    void for_each_window(std::function<void(std::shared_ptr<Container>)> const&) const;
    void toggle_floating(std::shared_ptr<Container> const&);
    bool has_floating_window(std::shared_ptr<Container> const&);
    std::shared_ptr<FloatingWindowContainer> add_floating_window(miral::Window const&);
    Output* get_output();
    void trigger_rerender();
    [[nodiscard]] bool is_empty() const;
    void graft(std::shared_ptr<Container> const&);
    /// Converts a workspace to its corresponding index in the workspace array.
    [[nodiscard]] uint32_t id() const { return id_; }
    [[nodiscard]] std::optional<int> num() const { return num_; }
    [[nodiscard]] nlohmann::json to_json() const;
    [[nodiscard]] TilingWindowTree const* get_tree() const { return tree.get(); }
    [[nodiscard]] std::optional<std::string> const& name() { return name_; }

private:
    Output* output;
    miral::WindowManagerTools tools;
    uint32_t id_;
    std::optional<int> num_;
    std::optional<std::string> name_;
    std::shared_ptr<TilingWindowTree> tree;
    std::vector<std::shared_ptr<FloatingWindowContainer>> floating_windows;
    std::vector<std::shared_ptr<FloatingTreeContainer>> floating_trees;
    WindowController& window_controller;
    CompositorState const& state;
    std::shared_ptr<Config> config;
    std::shared_ptr<MinimalWindowManager> floating_window_manager;

    /// Retrieves the container that is currently being used for layout
    std::shared_ptr<ParentContainer> get_layout_container();
};

} // miracle

#endif // MIRACLEWM_WORKSPACE_CONTENT_H

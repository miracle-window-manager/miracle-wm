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
class WindowController;
class CompositorState;
class ParentContainer;
class FloatingWindowContainer;
class FloatingTreeContainer;
class OutputManager;

struct AllocationHint
{
    ContainerType container_type = ContainerType::none;
    std::optional<std::shared_ptr<ParentContainer>> parent;
};

class Workspace
{
public:
    virtual ~Workspace() = default;

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

    virtual void handle_ready_hack(LeafContainer& container) = 0;
    virtual void delete_container(std::shared_ptr<Container> const& container) = 0;
    virtual bool move_container(Direction direction, Container&) = 0;
    virtual bool move_to_container_position(Container& to_move, Container& target) = 0;
    virtual bool add_to_root(Container& to_move) = 0;
    virtual void show() = 0;
    virtual void hide() = 0;

    virtual void transfer_pinned_windows_to(std::shared_ptr<Workspace> const& other) = 0;

    /// Iterates all containers on this workspace that represent a window until the predicate is satisfied.
    /// Returns true if the predicate returned true.
    virtual bool for_each_window(std::function<bool(std::shared_ptr<Container>)> const&) const = 0;

    /// Creates a new floating tree on this workspace. The tree is empty by default
    /// and must be filled in by subsequent calls, lest it become a zombie tree with
    /// zero sub containers.
    virtual std::shared_ptr<ParentContainer> create_floating_tree(mir::geometry::Rectangle const& area) = 0;

    virtual void advise_focus_gained(std::shared_ptr<Container> const& container) = 0;

    virtual void select_first_window() = 0;

    [[nodiscard]] virtual Output* get_output() const = 0;

    virtual void set_output(Output*) = 0;

    [[deprecated("Do not use unless you have a very good reason to do so!")]]
    virtual void workspace_transform_change_hack()
        = 0;

    [[nodiscard]] virtual bool is_empty() const = 0;
    virtual void graft(std::shared_ptr<Container> const&) = 0;

    [[nodiscard]] virtual uint32_t id() const = 0;
    [[nodiscard]] virtual std::optional<int> num() const = 0;
    [[nodiscard]] virtual nlohmann::json to_json() const = 0;
    [[nodiscard]] virtual std::optional<std::string> const& name() const = 0;
    [[nodiscard]] virtual std::string display_name() const = 0;
    [[nodiscard]] virtual std::shared_ptr<ParentContainer> get_root() const = 0;
};

class MiralWorkspace : public Workspace
{
public:
    MiralWorkspace(
        Output* output,
        uint32_t id,
        std::optional<int> num,
        std::optional<std::string> name,
        std::shared_ptr<Config> const& config,
        WindowController& window_controller,
        CompositorState const& state,
        std::shared_ptr<MinimalWindowManager> const& floating_window_manager,
        OutputManager* output_manager);
    ~MiralWorkspace();

    void set_area(mir::geometry::Rectangle const&) override;
    void recalculate_area() override;

    AllocationHint allocate_position(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification& requested_specification,
        AllocationHint const& hint) override;
    std::shared_ptr<Container> create_container(
        miral::WindowInfo const& window_info, AllocationHint const& type) override;
    void handle_ready_hack(LeafContainer& container) override;
    void delete_container(std::shared_ptr<Container> const& container) override;
    bool move_container(Direction direction, Container&) override;
    bool move_to_container_position(Container& to_move, Container& target) override;
    bool add_to_root(Container& to_move) override;
    void show() override;
    void hide() override;
    void transfer_pinned_windows_to(std::shared_ptr<Workspace> const& other) override;
    bool for_each_window(std::function<bool(std::shared_ptr<Container>)> const&) const override;
    std::shared_ptr<ParentContainer> create_floating_tree(mir::geometry::Rectangle const& area) override;
    void advise_focus_gained(std::shared_ptr<Container> const& container) override;
    void select_first_window() override;
    Output* get_output() const override;
    void set_output(Output*) override;
    void workspace_transform_change_hack() override;
    [[nodiscard]] bool is_empty() const override;
    void graft(std::shared_ptr<Container> const&) override;
    [[nodiscard]] uint32_t id() const override { return id_; }
    [[nodiscard]] std::optional<int> num() const override { return num_; }
    [[nodiscard]] nlohmann::json to_json() const override;
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

    Output* output;
    uint32_t id_;
    std::optional<int> num_;
    std::optional<std::string> name_;
    std::shared_ptr<ParentContainer> root;
    std::vector<std::shared_ptr<ParentContainer>> floating_trees;
    WindowController& window_controller;
    CompositorState const& state;
    std::shared_ptr<Config> config;
    OutputManager* output_manager;
    std::shared_ptr<MinimalWindowManager> floating_window_manager;
    std::weak_ptr<Container> last_selected_container;
    int config_handle = 0;

    /// Retrieves the container that is currently being used for layout
    std::shared_ptr<ParentContainer> get_layout_container();

    /// From the provided node, find the next node in the provided direction.
    /// This method is guaranteed to return a Window node, not a Lane.
    MoveResult handle_move(Container& from, Direction direction);

    // Transfer a node from its current parent to the parent of 'to'
    /// in a position right after 'to'.
    /// @returns The two parents who will need to have their changes committed
    std::tuple<std::shared_ptr<ParentContainer>, std::shared_ptr<ParentContainer>> transfer_node(
        std::shared_ptr<Container> const& node,
        std::shared_ptr<Container> const& to);
};

} // miracle

#endif // MIRACLEWM_WORKSPACE_CONTENT_H

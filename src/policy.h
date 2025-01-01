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

#ifndef MIRACLE_POLICY_H
#define MIRACLE_POLICY_H

#include "animator.h"
#include "auto_restarting_launcher.h"
#include "command_controller.h"
#include "compositor_state.h"
#include "config.h"
#include "ipc.h"
#include "ipc_command_executor.h"
#include "minimal_window_manager.h"
#include "mode_observer.h"
#include "output.h"
#include "scratchpad.h"
#include "surface_tracker.h"
#include "window_manager_tools_window_controller.h"
#include "workspace_manager.h"

#include <memory>
#include <miral/window_management_policy.h>
#include <miral/window_manager_tools.h>
#include <vector>

namespace miral
{
class MirRunner;
}

namespace miracle
{

class Container;
class ContainerGroupContainer;
class WindowToolsAccessor;
class AnimatorLoop;

class Policy : public miral::WindowManagementPolicy, public CommandController
{
public:
    Policy(
        miral::WindowManagerTools const&,
        AutoRestartingLauncher&,
        miral::MirRunner&,
        std::shared_ptr<Config> const&,
        std::shared_ptr<Animator> const&,
        SurfaceTracker&,
        mir::Server const&,
        CompositorState&,
        std::shared_ptr<WindowToolsAccessor> const&);
    ~Policy() override;

    // Interactions with the engine

    bool handle_keyboard_event(MirKeyboardEvent const* event) override;
    bool handle_pointer_event(MirPointerEvent const* event) override;
    auto place_new_window(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification const& requested_specification) -> miral::WindowSpecification override;
    void advise_new_window(miral::WindowInfo const& window_info) override;
    void handle_window_ready(miral::WindowInfo& window_info) override;
    void advise_focus_gained(miral::WindowInfo const& window_info) override;
    void advise_focus_lost(miral::WindowInfo const& window_info) override;
    void advise_delete_window(miral::WindowInfo const& window_info) override;
    void advise_move_to(miral::WindowInfo const& window_info, geom::Point top_left) override;
    void advise_output_create(miral::Output const& output) override;
    void advise_output_update(miral::Output const& updated, miral::Output const& original) override;
    void advise_output_delete(miral::Output const& output) override;
    void handle_modify_window(miral::WindowInfo& window_info, const miral::WindowSpecification& modifications) override;
    void handle_raise_window(miral::WindowInfo& window_info) override;
    auto confirm_placement_on_display(
        const miral::WindowInfo& window_info,
        MirWindowState new_state,
        const mir::geometry::Rectangle& new_placement) -> mir::geometry::Rectangle override;
    bool handle_touch_event(const MirTouchEvent* event) override;
    void handle_request_move(miral::WindowInfo& window_info, const MirInputEvent* input_event) override;
    void handle_request_resize(
        miral::WindowInfo& window_info,
        const MirInputEvent* input_event,
        MirResizeEdge edge) override;
    auto confirm_inherited_move(
        const miral::WindowInfo& window_info,
        mir::geometry::Displacement movement) -> mir::geometry::Rectangle override;
    void advise_application_zone_create(miral::Zone const& application_zone) override;
    void advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original) override;
    void advise_application_zone_delete(miral::Zone const& application_zone) override;
    void advise_end() override;

    // Requests

    bool try_request_horizontal() override;
    bool try_request_vertical() override;
    bool try_toggle_layout(bool cycle_through_all) override;
    void try_toggle_resize_mode() override;
    bool try_resize(Direction direction, int pixels) override;
    bool try_set_size(std::optional<int> const& width, std::optional<int> const& height) override;
    bool try_move(Direction direction) override;
    bool try_move_by(Direction direction, int pixels) override;
    bool try_move_to(int x, int y) override;
    bool try_select(Direction direction) override;
    bool try_select_parent() override;
    bool try_select_child() override;
    bool try_select_floating() override;
    bool try_select_tiling() override;
    bool try_select_toggle() override;
    bool try_close_window() override;
    bool quit() override;
    bool try_toggle_fullscreen() override;
    bool select_workspace(int number, bool back_and_forth = true) override;
    bool select_workspace(std::string const& name, bool back_and_forth) override;
    bool next_workspace() override;
    bool prev_workspace() override;
    bool back_and_forth_workspace() override;
    bool next_workspace_on_output(Output const&) override;
    bool prev_workspace_on_output(Output const&) override;
    bool move_active_to_workspace(int number, bool back_and_forth = true) override;
    bool move_active_to_workspace_named(std::string const&, bool back_and_forth) override;
    bool move_active_to_next_workspace() override;
    bool move_active_to_prev_workspace() override;
    bool move_active_to_back_and_forth() override;
    bool move_to_scratchpad() override;
    bool show_scratchpad() override;
    bool toggle_floating() override;
    bool toggle_pinned_to_workspace() override;
    bool set_is_pinned(bool) override;
    bool toggle_tabbing() override;
    bool toggle_stacking() override;
    bool set_layout(LayoutScheme scheme) override;
    bool set_layout_default() override;
    void move_cursor_to_output(Output const&) override;
    bool try_select_next_output() override;
    bool try_select_prev_output() override;
    bool try_select_output(Direction direction) override;
    bool try_select_output(std::vector<std::string> const& names) override;
    bool try_move_active_to_output(Direction direction) override;
    bool try_move_active_to_current() override;
    bool try_move_active_to_primary() override;
    bool try_move_active_to_nonprimary() override;
    bool try_move_active_to_next() override;
    bool try_move_active(std::vector<std::string> const& names) override;
    bool reload_config() override;
    [[nodiscard]] nlohmann::json to_json() const override;
    [[nodiscard]] nlohmann::json outputs_json() const override;
    [[nodiscard]] nlohmann::json workspaces_json() const override;
    [[nodiscard]] nlohmann::json workspace_to_json(uint32_t) const override;
    [[nodiscard]] nlohmann::json mode_to_json() const override;

    // Getters

    [[nodiscard]] Output const* get_active_output() const { return state.active_output.get(); }
    [[nodiscard]] std::vector<std::shared_ptr<Output>> const& get_output_list() const { return state.output_list; }
    [[nodiscard]] geom::Point const& get_cursor_position() const { return state.cursor_position; }
    [[nodiscard]] CompositorState const& get_state() const { return state; }

private:
    class Self;

    class Locker
    {
    public:
        explicit Locker(std::shared_ptr<Self> const&);
        ~Locker();

    private:
        std::lock_guard<std::recursive_mutex> lock_guard;
    };

    bool can_move_container() const;
    bool can_set_layout() const;
    std::shared_ptr<Container> toggle_floating_internal(std::shared_ptr<Container> const& container);

    /// Selects any type of container, including those that do not directly reference a window.
    void select_container(std::shared_ptr<Container> const&);
    std::shared_ptr<Output> const& _next_output_in_list(std::vector<std::string> const& names);
    std::shared_ptr<Output> const& _next_output_in_direction(Direction direction);

    bool is_starting_ = true;
    CompositorState& state;
    AllocationHint pending_allocation;
    std::vector<miral::Window> orphaned_window_list;
    miral::WindowManagerTools window_manager_tools;
    std::shared_ptr<MinimalWindowManager> floating_window_manager;
    AutoRestartingLauncher& external_client_launcher;
    miral::MirRunner& runner;
    std::shared_ptr<Config> config;
    WorkspaceObserverRegistrar workspace_observer_registrar;
    ModeObserverRegistrar mode_observer_registrar;
    WorkspaceManager workspace_manager;
    std::shared_ptr<Ipc> ipc;
    std::shared_ptr<Animator> animator;
    std::unique_ptr<AnimatorLoop> animator_loop;
    WindowManagerToolsWindowController window_controller;
    IpcCommandExecutor i3_command_executor;
    SurfaceTracker& surface_tracker;
    std::shared_ptr<ContainerGroupContainer> group_selection;
    Scratchpad scratchpad_;
    std::shared_ptr<Self> self;
    mir::Server const& server;
};
}

#endif // MIRACLE_POLICY_H

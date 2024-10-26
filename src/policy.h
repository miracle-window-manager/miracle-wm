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
#include "compositor_state.h"
#include "config.h"
#include "i3_command_executor.h"
#include "ipc.h"
#include "minimal_window_manager.h"
#include "mode_observer.h"
#include "output.h"
#include "surface_tracker.h"
#include "window_manager_tools_window_controller.h"
#include "workspace_manager.h"

#include <memory>
#include <miral/internal_client.h>
#include <miral/output.h>
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

class Policy : public miral::WindowManagementPolicy
{
public:
    Policy(
        miral::WindowManagerTools const&,
        AutoRestartingLauncher&,
        miral::MirRunner&,
        std::shared_ptr<MiracleConfig> const&,
        SurfaceTracker&,
        mir::Server const&,
        CompositorState&);
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

    bool try_request_horizontal();
    bool try_request_vertical();
    bool try_toggle_layout(bool cycle_through_all);
    void try_toggle_resize_mode();
    bool try_resize(Direction direction);
    bool try_move(Direction direction);
    bool try_move_by(Direction direction, int pixels);
    bool try_move_to(int x, int y);
    bool try_select(Direction direction);
    bool try_close_window();
    bool quit();
    bool try_toggle_fullscreen();
    bool select_workspace(int number, bool back_and_forth = true);
    bool select_workspace(std::string const& name, bool back_and_forth);
    bool next_workspace();
    bool prev_workspace();
    bool back_and_forth_workspace();
    bool next_workspace_on_output(Output const&);
    bool prev_workspace_on_output(Output const&);
    bool move_active_to_workspace(int number, bool back_and_forth = true);
    bool move_active_to_workspace_named(std::string const&, bool back_and_forth);
    bool move_active_to_next();
    bool move_active_to_prev();
    bool move_active_to_back_and_forth();
    bool toggle_floating();
    bool toggle_pinned_to_workspace();
    bool set_is_pinned(bool);
    bool toggle_tabbing();
    bool toggle_stacking();
    bool set_layout(LayoutScheme scheme);
    bool set_layout_default();

    // Getters

    [[nodiscard]] Output const* get_active_output() const { return state.active_output.get(); }
    [[nodiscard]] std::vector<std::shared_ptr<Output>> const& get_output_list() const { return output_list; }
    [[nodiscard]] geom::Point const& get_cursor_position() const { return state.cursor_position; }
    [[nodiscard]] CompositorState const& get_state() const { return state; }

private:
    bool can_move_container() const;
    bool can_set_layout() const;

    bool is_starting_ = true;
    CompositorState& state;
    std::vector<std::shared_ptr<Output>> output_list;
    std::weak_ptr<Output> pending_output;
    AllocationHint pending_allocation;
    std::vector<Window> orphaned_window_list;
    miral::WindowManagerTools window_manager_tools;
    std::shared_ptr<MinimalWindowManager> floating_window_manager;
    AutoRestartingLauncher& external_client_launcher;
    miral::MirRunner& runner;
    std::shared_ptr<MiracleConfig> config;
    WorkspaceObserverRegistrar workspace_observer_registrar;
    ModeObserverRegistrar mode_observer_registrar;
    WorkspaceManager workspace_manager;
    std::shared_ptr<Ipc> ipc;
    Animator animator;
    WindowManagerToolsWindowController window_controller;
    I3CommandExecutor i3_command_executor;
    SurfaceTracker& surface_tracker;
    std::shared_ptr<ContainerGroupContainer> group_selection;
};
}

#endif // MIRACLE_POLICY_H

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
#include "drag_and_drop_service.h"
#include "ipc.h"
#include "ipc_command_executor.h"
#include "mode_observer.h"
#include "move_service.h"
#include "output.h"
#include "scratchpad.h"
#include "window_manager_tools_window_controller.h"
#include "workspace_manager.h"

#include <memory>
#include <miral/window_management_policy.h>
#include <miral/window_manager_tools.h>
#include <vector>

namespace miral
{
class MirRunner;
class ExternalClientLauncher;
}

namespace miracle
{

class Container;
class ContainerGroupContainer;
class AnimatorLoop;
class OutputManager;

class Policy : public miral::WindowManagementPolicy
{
public:
    Policy(
        miral::WindowManagerTools const&,
        mir::Server const&,
        miral::MirRunner&,
        miral::ExternalClientLauncher& external_client_launcher,
        std::shared_ptr<Config> const&,
        std::shared_ptr<CompositorState> const& state);
    ~Policy() override;

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
    void handle_animation(
        AnimationStepResult const& asr,
        std::weak_ptr<Container> const& container);
    auto confirm_inherited_move(
        const miral::WindowInfo& window_info,
        mir::geometry::Displacement movement) -> mir::geometry::Rectangle override;
    void advise_application_zone_create(miral::Zone const& application_zone) override;
    void advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original) override;
    void advise_application_zone_delete(miral::Zone const& application_zone) override;
    void advise_end() override;

private:
    class Self;

    std::shared_ptr<Config> config;
    std::shared_ptr<CompositorState> state;
    std::shared_ptr<Animator> animator;
    std::shared_ptr<WindowManagerToolsWindowController> window_controller;
    std::unique_ptr<AutoRestartingLauncher> launcher;
    std::shared_ptr<WorkspaceObserverRegistrar> workspace_observer_registrar;
    std::shared_ptr<ModeObserverRegistrar> mode_observer_registrar;
    std::shared_ptr<OutputManager> output_manager;
    std::shared_ptr<WorkspaceManager> workspace_manager;
    std::shared_ptr<Self> self;
    std::shared_ptr<Scratchpad> scratchpad_;
    std::shared_ptr<CommandController> command_controller;
    std::unique_ptr<DragAndDropService> drag_and_drop_service;
    std::unique_ptr<MoveService> move_service;
    std::shared_ptr<Ipc> ipc;
    std::unique_ptr<AnimatorLoop> animator_loop;
    std::shared_ptr<ContainerGroupContainer> group_selection;

    bool is_starting_ = true;
    AllocationHint pending_allocation;
};
}

#endif // MIRACLE_POLICY_H

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

#ifndef MIRACLEWM_WINDOW_MANAGER_TOOLS_TILING_INTERFACE_H
#define MIRACLEWM_WINDOW_MANAGER_TOOLS_TILING_INTERFACE_H

#include "animator.h"
#include "window_controller.h"
#include <miral/window_manager_tools.h>

namespace mir
{
class ServerActionQueue;
}
namespace miracle
{
class CompositorState;
class Config;
class Policy;

class WindowManagerToolsWindowController : public WindowController
{
public:
    WindowManagerToolsWindowController(
        miral::WindowManagerTools const&,
        Animator& animator,
        CompositorState& state,
        std::shared_ptr<Config> const& config,
        std::shared_ptr<mir::ServerActionQueue> const& server_action_queue,
        Policy* policy);
    void open(miral::Window const&) override;
    bool is_fullscreen(miral::Window const&) override;
    void set_rectangle(miral::Window const&, geom::Rectangle const&, geom::Rectangle const&) override;
    MirWindowState get_state(miral::Window const&) override;
    void change_state(miral::Window const&, MirWindowState state) override;
    void clip(miral::Window const&, geom::Rectangle const&) override;
    void noclip(miral::Window const&) override;
    void select_active_window(miral::Window const&) override;
    std::shared_ptr<Container> get_container(miral::Window const&) override;
    void raise(miral::Window const&) override;
    void send_to_back(miral::Window const&) override;
    void set_user_data(miral::Window const&, std::shared_ptr<void> const&) override;
    void modify(miral::Window const&, miral::WindowSpecification const&) override;
    miral::WindowInfo& info_for(miral::Window const&) override;
    miral::ApplicationInfo& info_for(miral::Application const&) override;
    miral::ApplicationInfo& app_info(miral::Window const&) override;
    void close(miral::Window const& window) override;
    void move_cursor_to(float x, float y) override;
    void set_size_hack(AnimationHandle handle, mir::geometry::Size const& size) override;
    miral::Window window_at(float x, float y) override;
    void process_animation(AnimationStepResult const&, std::shared_ptr<Container> const&) override;

private:
    miral::WindowManagerTools tools;
    Animator& animator;
    CompositorState& state;
    std::shared_ptr<Config> config;
    std::shared_ptr<mir::ServerActionQueue> server_action_queue;
    Policy* policy;

    class WindowAnimation : public Animation
    {
    public:
        WindowAnimation(
            AnimationHandle handle,
            AnimationDefinition definition,
            mir::geometry::Rectangle const& from,
            mir::geometry::Rectangle const& to,
            mir::geometry::Rectangle const& current,
            WindowManagerToolsWindowController* controller,
            std::shared_ptr<Container> const& container);
        void on_tick(AnimationStepResult const&) override;

    private:
        WindowManagerToolsWindowController* controller;
        std::weak_ptr<Container> container;
    };
};
}

#endif // MIRACLEWM_WINDOW_MANAGER_TOOLS_TILING_INTERFACE_H

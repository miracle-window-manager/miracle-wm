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

#define MIR_LOG_COMPONENT "window_manager_tools_tiling_interface"

#include "window_manager_tools_window_controller.h"
#include "animator.h"
#include "compositor_state.h"
#include "leaf_container.h"
#include "window_helpers.h"
#include <mir/log.h>
#include <mir/scene/surface.h>

using namespace miracle;

WindowManagerToolsWindowController::WindowManagerToolsWindowController(
    miral::WindowManagerTools const& tools,
    Animator& animator,
    CompositorState& state) :
    tools { tools },
    animator { animator },
    state { state }
{
}

void WindowManagerToolsWindowController::open(miral::Window const& window)
{
    auto container = get_container(window);
    if (!container)
    {
        mir::log_error("Cannot set rectangle of window that lacks container");
        return;
    }

    auto const& info = info_for(window);
    geom::Rectangle rect { window.top_left(), window.size() };
    if (info.parent())
    {
        on_animation({ container->animation_handle(), true, rect }, container);
        return;
    }

    animator.window_open(
        container->animation_handle(),
        rect,
        rect,
        rect,
        [this, container = container](miracle::AnimationStepResult const& result)
    {
        on_animation(result, container);
    });
}

bool WindowManagerToolsWindowController::is_fullscreen(miral::Window const& window)
{
    auto& info = tools.info_for(window);
    return window_helpers::is_window_fullscreen(info.state());
}

void WindowManagerToolsWindowController::set_rectangle(
    miral::Window const& window, geom::Rectangle const& from, geom::Rectangle const& to)
{
    auto container = get_container(window);
    if (!container)
    {
        mir::log_error("Cannot set rectangle of window that lacks container");
        return;
    }

    auto const& info = info_for(window);
    if (info.parent())
    {
        on_animation({ container->animation_handle(), true, to }, container);
        return;
    }

    animator.window_move(
        container->animation_handle(),
        from,
        to,
        geom::Rectangle { window.top_left(), window.size() },
        [this, container = container](AnimationStepResult const& result)
    {
        on_animation(result, container);
    });
}

MirWindowState WindowManagerToolsWindowController::get_state(miral::Window const& window)
{
    auto& window_info = tools.info_for(window);
    return window_info.state();
}

void WindowManagerToolsWindowController::change_state(miral::Window const& window, MirWindowState state)
{
    auto& window_info = tools.info_for(window);
    miral::WindowSpecification spec;
    spec.state() = state;
    tools.place_and_size_for_state(spec, window_info);
    tools.modify_window(window, spec);
}

void WindowManagerToolsWindowController::clip(miral::Window const& window, geom::Rectangle const& r)
{
    auto& window_info = tools.info_for(window);
    window_info.clip_area(r);
}

void WindowManagerToolsWindowController::noclip(miral::Window const& window)
{
    auto& window_info = tools.info_for(window);
    window_info.clip_area(mir::optional_value<geom::Rectangle>());
}

void WindowManagerToolsWindowController::select_active_window(miral::Window const& window)
{
    if (state.mode == WindowManagerMode::resizing)
        return;

    tools.select_active_window(window);
}

std::shared_ptr<Container> WindowManagerToolsWindowController::get_container(miral::Window const& window)
{
    if (window == miral::Window {})
        return nullptr;

    auto& info = tools.info_for(window);
    if (info.userdata())
        return static_pointer_cast<Container>(info.userdata());

    if (info.parent())
        return get_container(info.parent());

    return nullptr;
}

void WindowManagerToolsWindowController::raise(miral::Window const& window)
{
    tools.raise_tree(window);
}

void WindowManagerToolsWindowController::send_to_back(miral::Window const& window)
{
    tools.send_tree_to_back(window);
}

void WindowManagerToolsWindowController::on_animation(
    miracle::AnimationStepResult const& result,
    std::shared_ptr<Container> const& container)
{
    auto window = container->window().value();
    bool needs_modify = false;
    miral::WindowSpecification spec;

    if (result.position)
    {
        spec.top_left() = mir::geometry::Point(
            result.position.value().x,
            result.position.value().y);
        needs_modify = true;
    }

    if (result.size)
    {
        spec.size() = mir::geometry::Size(
            result.size.value().x,
            result.size.value().y);
        needs_modify = true;
    }

    if (result.transform)
    {
        container->set_transform(result.transform.value());
        needs_modify = true;
    }

    if (needs_modify)
        tools.modify_window(window, spec);

    // NOTE: The clip area needs to reflect the current position + transform of the window.
    // Failing to set a clip area will cause overflowing windows to briefly disregard their
    // compacted size.
    // TODO: When we have rotation in our transforms, then we need to handle rotations.
    //  At that point, the top_left corner will change. We will need to find an AABB
    //  to represent the clip area.
    if (result.is_complete)
        container->constrain();
    else
    {
        if (container->get_type() == ContainerType::leaf)
            clip(window, result.clip_area);
        else
            noclip(window);
    }
}

void WindowManagerToolsWindowController::set_user_data(
    miral::Window const& window, std::shared_ptr<void> const& data)
{
    miral::WindowSpecification spec;
    spec.userdata() = data;
    tools.modify_window(window, spec);
}

void WindowManagerToolsWindowController::modify(
    miral::Window const& window, miral::WindowSpecification const& spec)
{
    tools.modify_window(window, spec);
}

miral::WindowInfo& WindowManagerToolsWindowController::info_for(miral::Window const& window)
{
    return tools.info_for(window);
}

miral::ApplicationInfo& WindowManagerToolsWindowController::app_info(miral::Window const& window)
{
    return tools.info_for(window.application());
}

void WindowManagerToolsWindowController::close(miral::Window const& window)
{
    tools.ask_client_to_close(window);
}

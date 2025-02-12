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
#include "config.h"
#include "leaf_container.h"
#include "policy.h"
#include "window_helpers.h"
#include <mir/log.h>
#include <mir/scene/surface.h>
#include <mir/server_action_queue.h>

using namespace miracle;

WindowManagerToolsWindowController::WindowManagerToolsWindowController(
    miral::WindowManagerTools const& tools,
    std::shared_ptr<Animator> const& animator,
    std::shared_ptr<CompositorState> const& state,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<mir::ServerActionQueue> const& server_action_queue,
    Policy* policy) :
    tools { tools },
    animator { animator },
    state { state },
    config { config },
    server_action_queue { server_action_queue },
    policy { policy }
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
        policy->handle_animation({ container->animation_handle(), true, rect }, container);
        return;
    }

    if (!config->are_animations_enabled())
    {
        policy->handle_animation(AnimationStepResult { container->animation_handle(), true, rect }, container);
        return;
    }

    auto animation = std::make_shared<WindowAnimation>(
        container->animation_handle(),
        config->get_animation_definitions()[(int)AnimateableEvent::window_open],
        rect,
        rect,
        rect,
        this,
        container);

    animator->append(animation);
}

void WindowManagerToolsWindowController::set_rectangle(
    miral::Window const& window, geom::Rectangle const& from, geom::Rectangle const& to, bool with_animations)
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
        policy->handle_animation({ container->animation_handle(), true, to }, container);
        return;
    }

    if (!config->are_animations_enabled() || !with_animations)
    {
        policy->handle_animation(
            AnimationStepResult { container->animation_handle(),
                true,
                to,
                glm::vec2(to.top_left.x.as_int(), to.top_left.y.as_int()),
                glm::vec2(to.size.width.as_int(), to.size.height.as_int()),
                glm::mat4(1.f) },
            container);
        return;
    }

    auto animation = std::make_shared<WindowAnimation>(
        container->animation_handle(),
        config->get_animation_definitions()[(int)AnimateableEvent::window_move],
        from,
        to,
        geom::Rectangle { window.top_left(), window.size() },
        this,
        container);

    animator->append(animation);
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
    if (state->mode() != WindowManagerMode::normal)
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

WindowManagerToolsWindowController::WindowAnimation::WindowAnimation(
    AnimationHandle handle,
    AnimationDefinition definition,
    mir::geometry::Rectangle const& from,
    mir::geometry::Rectangle const& to,
    mir::geometry::Rectangle const& current,
    WindowManagerToolsWindowController* controller,
    std::shared_ptr<Container> const& container) :
    Animation(handle, definition, from, to, current),
    controller { controller },
    container { container }
{
}

void WindowManagerToolsWindowController::WindowAnimation::on_tick(AnimationStepResult const& asr)
{
    controller->server_action_queue->enqueue(controller, [controller = controller, asr = asr, container = container]()
    {
        controller->policy->handle_animation(asr, container);
    });
}

void WindowManagerToolsWindowController::process_animation(
    AnimationStepResult const& result,
    std::shared_ptr<Container> const& container)
{
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
        container->set_transform(result.transform.value());

    if (needs_modify)
    {
        if (!container->window())
            return;

        auto window = container->window().value();
        if (!window)
            return;
        tools.modify_window(window, spec);

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

miral::ApplicationInfo& WindowManagerToolsWindowController::info_for(miral::Application const& app)
{
    return tools.info_for(app);
}

miral::ApplicationInfo& WindowManagerToolsWindowController::app_info(miral::Window const& window)
{
    return tools.info_for(window.application());
}

void WindowManagerToolsWindowController::close(miral::Window const& window)
{
    tools.ask_client_to_close(window);
}

void WindowManagerToolsWindowController::move_cursor_to(float x, float y)
{
    tools.move_cursor_to(geom::PointF { x, y });
}

void WindowManagerToolsWindowController::set_size_hack(AnimationHandle handle, mir::geometry::Size const& size)
{
    animator->set_size_hack(handle, size);
}

miral::Window WindowManagerToolsWindowController::window_at(float x, float y)
{
    return tools.window_at({ x, y });
}
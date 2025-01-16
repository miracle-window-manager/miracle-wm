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

#include "minimal_window_manager.h"
#include "config.h"
#include "constants.h"
#include <miral/toolkit_event.h>

namespace
{
enum class Gesture
{
    none,
    pointer_moving,
    pointer_resizing,
    touch_moving,
    touch_resizing,
};

auto pointer_position(MirPointerEvent const* event) -> mir::geometry::Point
{
    return {
        mir_pointer_event_axis_value(event, mir_pointer_axis_x),
        mir_pointer_event_axis_value(event, mir_pointer_axis_y)
    };
}

auto touch_center(MirTouchEvent const* event) -> mir::geometry::Point
{
    auto const count = mir_touch_event_point_count(event);

    long total_x = 0;
    long total_y = 0;

    for (auto i = 0U; i != count; ++i)
    {
        total_x += mir_touch_event_axis_value(event, i, mir_touch_axis_x);
        total_y += mir_touch_event_axis_value(event, i, mir_touch_axis_y);
    }

    return { total_x / count, total_y / count };
}
}

struct miracle::MinimalWindowManager::Impl
{
    Impl(miral::WindowManagerTools const& tools, std::shared_ptr<Config> const& config) :
        tools { tools },
        config { config }
    {
    }
    miral::WindowManagerTools tools;

    Gesture gesture = Gesture::none;
    MirPointerButton pointer_gesture_button;
    miral::Window gesture_window;
    unsigned gesture_shift_keys = 0;
    MirResizeEdge resize_edge = mir_resize_edge_none;
    miral::Point resize_top_left;
    miral::Size resize_size;
    miral::Point old_cursor {};
    miral::Point old_touch {};

    bool prepare_for_gesture(miral::WindowInfo& window_info, miral::Point input_pos, Gesture gesture);

    bool begin_pointer_gesture(
        miral::WindowInfo& window_info, MirInputEvent const* input_event, Gesture gesture, MirResizeEdge edge);

    bool begin_touch_gesture(
        miral::WindowInfo& window_info, MirInputEvent const* input_event, Gesture gesture, MirResizeEdge edge);

    bool handle_pointer_event(MirPointerEvent const* event);

    bool handle_touch_event(MirTouchEvent const* event);

    void apply_resize_by(miral::Displacement movement);

    std::shared_ptr<Config> const config;
};

miracle::MinimalWindowManager::MinimalWindowManager(miral::WindowManagerTools const& tools, std::shared_ptr<Config> const& config) :
    tools {
        tools
},
    self { new Impl { tools, config } }
{
}

miracle::MinimalWindowManager::~MinimalWindowManager()
{
    delete self;
}

auto miracle::MinimalWindowManager::place_new_window(
    miral::ApplicationInfo const& /*app_info*/, miral::WindowSpecification const& requested_specification)
    -> miral::WindowSpecification
{
    return requested_specification;
}

void miracle::MinimalWindowManager::handle_window_ready(miral::WindowInfo& window_info)
{
    if (window_info.can_be_active())
    {
        tools.select_active_window(window_info.window());
    }
}

void miracle::MinimalWindowManager::handle_modify_window(
    miral::WindowInfo& window_info, miral::WindowSpecification const& modifications)
{
    tools.modify_window(window_info, modifications);
}

void miracle::MinimalWindowManager::handle_raise_window(miral::WindowInfo& window_info)
{
    tools.select_active_window(window_info.window());
}

auto miracle::MinimalWindowManager::confirm_placement_on_display(
    miral::WindowInfo const& /*window_info*/, MirWindowState /*new_state*/, miral::Rectangle const& new_placement)
    -> miral::Rectangle
{
    return new_placement;
}

bool miracle::MinimalWindowManager::handle_keyboard_event(MirKeyboardEvent const* event)
{
    return false;
}

bool miracle::MinimalWindowManager::handle_touch_event(MirTouchEvent const* event)
{
    if (self->handle_touch_event(event))
    {
        return true;
    }

    return false;
}

bool miracle::MinimalWindowManager::handle_pointer_event(MirPointerEvent const* event)
{
    return self->handle_pointer_event(event);
}

void miracle::MinimalWindowManager::handle_request_move(miral::WindowInfo& window_info, MirInputEvent const* input_event)
{
    if (begin_pointer_move(window_info, input_event))
    {
        return;
    }
    else if (begin_touch_move(window_info, input_event))
    {
        return;
    }
}

bool miracle::MinimalWindowManager::begin_pointer_move(miral::WindowInfo const& window_info, MirInputEvent const* input_event)
{
    return self->begin_pointer_gesture(tools.info_for(window_info.window()), input_event, Gesture::pointer_moving, mir_resize_edge_none);
}

bool miracle::MinimalWindowManager::begin_touch_move(miral::WindowInfo const& window_info, MirInputEvent const* input_event)
{
    return self->begin_touch_gesture(tools.info_for(window_info.window()), input_event, Gesture::touch_moving, mir_resize_edge_none);
}

void miracle::MinimalWindowManager::handle_request_resize(
    miral::WindowInfo& window_info, MirInputEvent const* input_event, MirResizeEdge edge)
{
    if (begin_pointer_resize(window_info, input_event, edge))
    {
        return;
    }
    else if (begin_touch_resize(window_info, input_event, edge))
    {
        return;
    }
}

bool miracle::MinimalWindowManager::begin_touch_resize(
    miral::WindowInfo const& window_info, MirInputEvent const* input_event, MirResizeEdge const& edge)
{
    return self->begin_touch_gesture(tools.info_for(window_info.window()), input_event, Gesture::touch_resizing, edge);
}

bool miracle::MinimalWindowManager::begin_pointer_resize(
    miral::WindowInfo const& window_info, MirInputEvent const* input_event, MirResizeEdge const& edge)
{
    return self->begin_pointer_gesture(tools.info_for(window_info.window()), input_event, Gesture::pointer_resizing, edge);
}

auto miracle::MinimalWindowManager::confirm_inherited_move(miral::WindowInfo const& window_info, miral::Displacement movement)
    -> miral::Rectangle
{
    return { window_info.window().top_left() + movement, window_info.window().size() };
}

void miracle::MinimalWindowManager::advise_new_window(miral::WindowInfo const& window_info)
{
}

void miracle::MinimalWindowManager::advise_focus_gained(miral::WindowInfo const& window_info)
{
    tools.raise_tree(window_info.window());
}

void miracle::MinimalWindowManager::advise_new_app(miral::ApplicationInfo&) { }
void miracle::MinimalWindowManager::advise_delete_app(miral::ApplicationInfo const&) { }

void miracle::MinimalWindowManager::advise_focus_lost(const miral::WindowInfo& window_info)
{
}

void miracle::MinimalWindowManager::advise_delete_window(miral::WindowInfo const& window_info)
{
}

bool miracle::MinimalWindowManager::Impl::prepare_for_gesture(
    miral::WindowInfo& window_info,
    miral::Point input_pos,
    Gesture gesture)
{
    switch (gesture)
    {
    case Gesture::pointer_moving:
    case Gesture::touch_moving:
    {
        switch (window_info.state())
        {
        case mir_window_state_restored:
            return true;

        case mir_window_state_maximized:
        case mir_window_state_vertmaximized:
        case mir_window_state_horizmaximized:
        case mir_window_state_attached:
        {
            miral::WindowSpecification mods;
            mods.state() = mir_window_state_restored;
            tools.place_and_size_for_state(mods, window_info);
            miral::Rectangle placement {
                mods.top_left() ? mods.top_left().value() : window_info.window().top_left(),
                mods.size() ? mods.size().value() : window_info.window().size()
            };
            // Keep the window's top edge in the same place
            placement.top_left.y = window_info.window().top_left().y;
            // Keep the window under the cursor/touch
            placement.top_left.x = std::min(placement.top_left.x, input_pos.x);
            placement.top_left.x = std::max(placement.top_left.x, input_pos.x - as_delta(placement.size.width));
            placement.top_left.y = std::min(placement.top_left.y, input_pos.y);
            placement.top_left.y = std::max(placement.top_left.y, input_pos.y - as_delta(placement.size.height));
            mods.top_left() = placement.top_left;
            mods.size() = placement.size;
            tools.modify_window(window_info, mods);
        }
            return true;

        default:
            break;
        }
    }
    break;

    case Gesture::pointer_resizing:
    case Gesture::touch_resizing:
        return window_info.state() == mir_window_state_restored;

    case Gesture::none:
        break;
    }

    return false;
}

bool miracle::MinimalWindowManager::Impl::begin_pointer_gesture(
    miral::WindowInfo& window_info, MirInputEvent const* input_event, Gesture gesture_, MirResizeEdge edge)
{
    if (mir_input_event_get_type(input_event) != mir_input_event_type_pointer)
        return false;

    MirPointerEvent const* const pointer_event = mir_input_event_get_pointer_event(input_event);
    auto const position = pointer_position(pointer_event);

    if (!prepare_for_gesture(window_info, position, gesture_))
        return false;

    old_cursor = position;
    gesture = gesture_;
    gesture_window = window_info.window();
    gesture_shift_keys = mir_pointer_event_modifiers(pointer_event) & MODIFIER_MASK;
    resize_top_left = gesture_window.top_left();
    resize_size = gesture_window.size();
    resize_edge = edge;

    for (auto button : { mir_pointer_button_primary, mir_pointer_button_secondary, mir_pointer_button_tertiary })
    {
        if (mir_pointer_event_button_state(pointer_event, button))
        {
            pointer_gesture_button = button;
            break;
        }
    }

    return true;
}

bool miracle::MinimalWindowManager::Impl::begin_touch_gesture(
    miral::WindowInfo& window_info,
    MirInputEvent const* input_event,
    Gesture gesture_,
    MirResizeEdge edge)
{
    if (mir_input_event_get_type(input_event) != mir_input_event_type_touch)
        return false;

    MirTouchEvent const* const touch_event = mir_input_event_get_touch_event(input_event);
    auto const position = touch_center(touch_event);

    if (!prepare_for_gesture(window_info, position, gesture_))
        return false;

    old_touch = position;
    gesture = gesture_;
    gesture_window = window_info.window();
    gesture_shift_keys = mir_touch_event_modifiers(touch_event) & MODIFIER_MASK;
    resize_top_left = gesture_window.top_left();
    resize_size = gesture_window.size();
    resize_edge = edge;

    return true;
}

bool miracle::MinimalWindowManager::Impl::handle_pointer_event(MirPointerEvent const* event)
{
    auto const action = mir_pointer_event_action(event);
    auto const shift_keys = mir_pointer_event_modifiers(event) & MODIFIER_MASK;
    auto const new_cursor = pointer_position(event);

    bool consumes_event = false;

    switch (gesture)
    {
    case Gesture::pointer_resizing:
        if (action == mir_pointer_action_motion && shift_keys == gesture_shift_keys && mir_pointer_event_button_state(event, pointer_gesture_button))
        {
            apply_resize_by(new_cursor - old_cursor);
            consumes_event = true;
        }
        else
        {
            gesture = Gesture::none;
        }
        break;

    case Gesture::pointer_moving:
        if (action == mir_pointer_action_motion && shift_keys == gesture_shift_keys && mir_pointer_event_button_state(event, pointer_gesture_button))
        {
            if (gesture_window)
            {
                tools.drag_window(gesture_window, new_cursor - old_cursor);
                consumes_event = true;
            }
            else
            {
                gesture = Gesture::none;
            }
        }
        else
        {
            gesture = Gesture::none;
        }
        break;

    default:
        break;
    }

    if (!consumes_event && action == mir_pointer_action_button_down)
    {
        if (auto const window = tools.window_at(new_cursor))
        {
            tools.select_active_window(window);
        }

        if (auto const window = tools.active_window())
        {
            if (mir_pointer_event_button_state(event, mir_pointer_button_primary))
            {
                if (shift_keys == config->get_primary_modifier())
                {
                    begin_pointer_gesture(
                        tools.info_for(window),
                        mir_pointer_event_input_event(event),
                        Gesture::pointer_moving, mir_resize_edge_none);
                    consumes_event = true;
                }
            }
        }
    }

    old_cursor = new_cursor;
    return consumes_event;
}

bool miracle::MinimalWindowManager::Impl::handle_touch_event(MirTouchEvent const* event)
{
    bool consumes_event = false;
    auto const new_touch = touch_center(event);
    auto const count = mir_touch_event_point_count(event);
    auto const shift_keys = mir_touch_event_modifiers(event) & MODIFIER_MASK;

    bool is_drag = true;
    for (auto i = 0U; i != count; ++i)
    {
        switch (mir_touch_event_action(event, i))
        {
        case mir_touch_action_up:
        case mir_touch_action_down:
            is_drag = false;
            // Falls through
        default:
            continue;
        }
    }

    switch (gesture)
    {
    case Gesture::touch_resizing:
        if (is_drag && gesture_shift_keys == shift_keys)
        {
            if (gesture_window)
            {
                apply_resize_by(new_touch - old_touch);
                consumes_event = true;
            }
            else
            {
                gesture = Gesture::none;
            }
        }
        else
        {
            gesture = Gesture::none;
        }
        break;

    case Gesture::touch_moving:
        if (is_drag && gesture_shift_keys == shift_keys)
        {
            if (gesture_window)
            {
                tools.drag_window(gesture_window, new_touch - old_touch);
                consumes_event = true;
            }
            else
            {
                gesture = Gesture::none;
            }
        }
        else
        {
            gesture = Gesture::none;
        }
        break;

    default:
        break;
    }

    if (!consumes_event && count == 1 && mir_touch_event_action(event, 0) == mir_touch_action_down)
    {
        if (auto const window = tools.window_at(new_touch))
        {
            tools.select_active_window(window);
        }
    }

    old_touch = new_touch;
    return consumes_event;
}

void miracle::MinimalWindowManager::Impl::apply_resize_by(miral::Displacement movement)
{
    if (gesture_window)
    {
        auto const top_left = resize_top_left;
        miral::Rectangle const old_pos { top_left, resize_size };

        auto new_width = old_pos.size.width;
        auto new_height = old_pos.size.height;

        if (resize_edge & mir_resize_edge_east)
            new_width = old_pos.size.width + movement.dx;

        if (resize_edge & mir_resize_edge_west)
            new_width = old_pos.size.width - movement.dx;

        if (resize_edge & mir_resize_edge_north)
            new_height = old_pos.size.height - movement.dy;

        if (resize_edge & mir_resize_edge_south)
            new_height = old_pos.size.height + movement.dy;

        miral::Size new_size { new_width, new_height };

        miral::Point new_pos = top_left;

        if (resize_edge & mir_resize_edge_west)
            new_pos.x = top_left.x + movement.dx;

        if (resize_edge & mir_resize_edge_north)
            new_pos.y = top_left.y + movement.dy;

        miral::WindowSpecification modifications;
        modifications.top_left() = new_pos;
        modifications.size() = new_size;
        tools.modify_window(gesture_window, modifications);
        resize_top_left = new_pos;
        resize_size = new_size;
    }
    else
    {
        gesture = Gesture::none;
    }
}

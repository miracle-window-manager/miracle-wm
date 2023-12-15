//
// Created by mattkae on 9/8/23.
//
#define MIR_LOG_COMPONENT "miracle"

#include "miracle_window_management_policy.h"
#include "task_bar.h"

#include <mir_toolkit/events/enums.h>
#include <miral/toolkit_event.h>
#include <miral/application_info.h>
#include <mir/log.h>
#include <linux/input.h>


using namespace miracle;

namespace
{
const int MODIFIER_MASK =
    mir_input_event_modifier_alt |
    mir_input_event_modifier_shift |
    mir_input_event_modifier_sym |
    mir_input_event_modifier_ctrl |
    mir_input_event_modifier_meta;

const std::string TERMINAL = "konsole";
}

MiracleWindowManagementPolicy::MiracleWindowManagementPolicy(
    const miral::WindowManagerTools & tools,
    miral::ExternalClientLauncher const& external_client_launcher,
    miral::InternalClientLauncher const& internal_client_launcher)
    : miral::MinimalWindowManager(tools),
      tree{geom::Size{1280, 1016}}, // TODO: Don't hardcode! Ask the compositor
      window_manager_tools{tools},
      external_client_launcher{external_client_launcher},
      internal_client_launcher{internal_client_launcher}
{
}

bool MiracleWindowManagementPolicy::handle_keyboard_event(MirKeyboardEvent const* event)
{
    if (MinimalWindowManager::handle_keyboard_event(event)) {
        return true;
    }

    auto const action = miral::toolkit::mir_keyboard_event_action(event);
    auto const scan_code = miral::toolkit::mir_keyboard_event_scan_code(event);
    auto const modifiers = miral::toolkit::mir_keyboard_event_modifiers(event) & MODIFIER_MASK;

    if (action == MirKeyboardAction::mir_keyboard_action_down && (modifiers & mir_input_event_modifier_alt))
    {
        if (scan_code == KEY_ENTER)
        {
            external_client_launcher.launch({TERMINAL});
            return true;
        }
        else if (scan_code == KEY_V)
        {
            tree.request_vertical();
            return true;
        }
        else if (scan_code == KEY_H)
        {
            tree.request_horizontal();
            return true;
        }
        else if (scan_code == KEY_R)
        {
            tree.toggle_resize_mode();
            return true;
        }
        else if (scan_code == KEY_UP)
        {
            if (modifiers & mir_input_event_modifier_shift)
            {
                if (tree.try_move_active_window(WindowMoveDirection::up))
                    return true;
            }
            else if (tree.try_resize_active_window(WindowResizeDirection::up))
                return true;
        }
        else if (scan_code == KEY_DOWN)
        {
            if (modifiers & mir_input_event_modifier_shift)
            {
                if (tree.try_move_active_window(WindowMoveDirection::down))
                    return true;
            }
            else if (tree.try_resize_active_window(WindowResizeDirection::down))
                return true;
        }
        else if (scan_code == KEY_LEFT)
        {
            if (modifiers & mir_input_event_modifier_shift)
            {
                if (tree.try_move_active_window(WindowMoveDirection::left))
                    return true;
            }
            else if (tree.try_resize_active_window(WindowResizeDirection::left))
                return true;
        }
        else if (scan_code == KEY_RIGHT)
        {
            if (modifiers & mir_input_event_modifier_shift)
            {
                if (tree.try_move_active_window(WindowMoveDirection::right))
                    return true;
            }
            else if (tree.try_resize_active_window(WindowResizeDirection::right))
                return true;
        }
    }

    return false;
}

auto MiracleWindowManagementPolicy::place_new_window(
    const miral::ApplicationInfo &app_info,
    const miral::WindowSpecification &requested_specification) -> miral::WindowSpecification
{
    // In this step, we'll ask the WindowTree where we should place the window on the display
    // We will also resize the adjacent windows accordingly in this step.
    return tree.allocate_position(requested_specification);
}

void MiracleWindowManagementPolicy::handle_window_ready(miral::WindowInfo &window_info)
{
    // The new placement has been confirmed. We can now add the window into the pending position
    // in the tree. This comes _after_ place_new_window has been called.
    tree.confirm(window_info.window());
    tools.select_active_window(window_info.window());
}

void MiracleWindowManagementPolicy::advise_focus_gained(const miral::WindowInfo &window_info)
{
    tree.advise_focus_gained(window_info.window());
}

void MiracleWindowManagementPolicy::advise_focus_lost(const miral::WindowInfo &window_info)
{
    tree.advise_focus_lost(window_info.window());
}

void MiracleWindowManagementPolicy::advise_delete_window(const miral::WindowInfo &window_info)
{
    tree.advise_delete_window(window_info.window());
}
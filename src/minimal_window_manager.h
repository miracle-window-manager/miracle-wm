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

#ifndef MIRAL_MINIMAL_WINDOW_MANAGER_H
#define MIRAL_MINIMAL_WINDOW_MANAGER_H

#include <mir_toolkit/events/enums.h>
#include <miral/window_management_policy.h>
#include <miral/window_manager_tools.h>

namespace miracle
{
class MiracleConfig;
}

namespace miracle
{

/// An adaptation of miral's MinimalWindowManager for miracle.
class MinimalWindowManager : public miral::WindowManagementPolicy
{
public:
    MinimalWindowManager(miral::WindowManagerTools const& tools, std::shared_ptr<MiracleConfig> const& config);
    ~MinimalWindowManager();

    /// Honours the requested specification
    auto place_new_window(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification const& requested_specification) -> miral::WindowSpecification override;

    /// If the window can have focus it is given focus
    void handle_window_ready(miral::WindowInfo& window_info) override;

    /// Honours the requested modifications
    void handle_modify_window(miral::WindowInfo& window_info, miral::WindowSpecification const& modifications) override;

    /// Gives focus to the requesting window (tree)
    void handle_raise_window(miral::WindowInfo& window_info) override;

    /// Honours the requested placement
    auto confirm_placement_on_display(
        miral::WindowInfo const& window_info, MirWindowState new_state, miral::Rectangle const& new_placement) -> miral::Rectangle override;

    /// Handles Alt-Tab, Alt-Grave and Alt-F4
    bool handle_keyboard_event(MirKeyboardEvent const* event) override;

    /// Handles touch to focus
    bool handle_touch_event(MirTouchEvent const* event) override;

    /// Handles pre-existing move & resize gestures, plus click to focus
    bool handle_pointer_event(MirPointerEvent const* event) override;

    /// Initiates a move gesture (only implemented for pointers)
    void handle_request_move(miral::WindowInfo& window_info, MirInputEvent const* input_event) override;

    /// Initiates a resize gesture (only implemented for pointers)
    void handle_request_resize(miral::WindowInfo& window_info, MirInputEvent const* input_event, MirResizeEdge edge) override;

    /// Honours the requested movement
    auto confirm_inherited_move(miral::WindowInfo const& window_info, miral::Displacement movement) -> miral::Rectangle override;

    /// Raises newly focused window
    void advise_focus_gained(miral::WindowInfo const& window_info) override;

    void advise_focus_lost(miral::WindowInfo const& window_info) override;

    void advise_new_app(miral::ApplicationInfo& app_info) override;

    void advise_delete_app(miral::ApplicationInfo const& app_info) override;

    /// \remark Since MirAL 5.0
    void advise_new_window(miral::WindowInfo const& app_info) override;

    /// \remark Since MirAL 5.0
    void advise_delete_window(miral::WindowInfo const& app_info) override;

protected:
    miral::WindowManagerTools tools;

    bool begin_pointer_move(miral::WindowInfo const& window_info, MirInputEvent const* input_event);
    bool begin_pointer_resize(miral::WindowInfo const& window_info, MirInputEvent const* input_event, MirResizeEdge const& edge);

    bool begin_touch_move(miral::WindowInfo const& window_info, MirInputEvent const* input_event);
    bool begin_touch_resize(miral::WindowInfo const& window_info, MirInputEvent const* input_event, MirResizeEdge const& edge);

private:
    struct Impl;
    Impl* const self;
};
}

#endif // MIRAL_MINIMAL_WINDOW_MANAGER_H

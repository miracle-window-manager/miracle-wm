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

#ifndef MIRACLE_WM_COMMAND_CONTROLLER_H
#define MIRACLE_WM_COMMAND_CONTROLLER_H

#include "compositor_state.h"
#include "direction.h"
#include "output.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace miral
{
class MirRunner;
}

namespace miracle
{
class Scratchpad;
class ModeObserverRegistrar;
class OutputManager;

class CommandControllerInterface
{
public:
    virtual ~CommandControllerInterface() = default;
    virtual void quit() = 0;
};

/// Responsible for fielding requests from the system and forwarding
/// them to an appropriate handler. Requests can come from any thread
/// (e.g. the keyboard input thread, the ipc thread, etc.).
/// This class behaves similar to a controller pattern in a server
/// whereby requests are made on the controller that are then sent
/// to the proper subsystem for processing. In this case, the subsystem
/// may be anything from the tiles in the grid, the scratchpad, etc.
class CommandController
{
public:
    CommandController(
        std::shared_ptr<Config> config,
        std::recursive_mutex& mutex,
        CompositorState& state,
        WindowController& window_controller,
        WorkspaceManager& workspace_manager,
        ModeObserverRegistrar& mode_observer_registrar,
        std::unique_ptr<CommandControllerInterface> interface,
        Scratchpad& scratchpad,
        OutputManager* output_manager);

    bool try_request_horizontal();
    bool try_request_vertical();
    bool try_toggle_layout(bool cycle_through_all);
    void try_toggle_resize_mode();
    bool try_resize(Direction direction, int pixels);
    bool try_set_size(std::optional<int> const& width, std::optional<int> const& height);
    bool try_move(Direction direction);
    bool try_move_by(Direction direction, int pixels);
    bool try_move_to(int x, int y);
    bool try_select(Direction direction);
    bool try_select_parent();
    bool try_select_child();
    bool try_select_floating();
    bool try_select_tiling();
    bool try_select_toggle();
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
    bool move_active_to_next_workspace();
    bool move_active_to_prev_workspace();
    bool move_active_to_back_and_forth();
    bool move_to_scratchpad();
    bool show_scratchpad();
    bool toggle_floating();
    bool toggle_pinned_to_workspace();
    bool set_is_pinned(bool);
    bool toggle_tabbing();
    bool toggle_stacking();
    bool set_layout(LayoutScheme scheme);
    bool set_layout_default();
    void move_cursor_to_output(Output const&);
    bool try_select_next_output();
    bool try_select_prev_output();
    bool try_select_output(Direction direction);
    bool try_select_output(std::vector<std::string> const& names);
    bool try_move_active_to_output(Direction direction);
    bool try_move_active_to_current();
    bool try_move_active_to_primary();
    bool try_move_active_to_nonprimary();
    bool try_move_active_to_next();
    bool try_move_active(std::vector<std::string> const& names);
    bool reload_config();
    void set_mode(WindowManagerMode mode);
    void select_container(std::shared_ptr<Container> const&);
    [[nodiscard]] nlohmann::json to_json() const;
    [[nodiscard]] nlohmann::json outputs_json() const;
    [[nodiscard]] nlohmann::json workspaces_json() const;
    [[nodiscard]] nlohmann::json workspace_to_json(uint32_t) const;
    [[nodiscard]] nlohmann::json mode_to_json() const;

private:
    std::shared_ptr<Config> config;
    std::recursive_mutex& mutex;
    CompositorState& state;
    WindowController& window_controller;
    WorkspaceManager& workspace_manager;
    ModeObserverRegistrar& mode_observer_registrar;
    std::unique_ptr<CommandControllerInterface> interface;
    Scratchpad& scratchpad_;
    OutputManager* output_manager;

    bool can_move_container() const;
    bool can_set_layout() const;
    std::shared_ptr<Container> toggle_floating_internal(std::shared_ptr<Container> const& container);

    Output* _next_output_in_list(std::vector<std::string> const& names);
    Output* _next_output_in_direction(Direction direction);
};
}

#endif // MIRACLE_WM_COMMAND_CONTROLLER_H

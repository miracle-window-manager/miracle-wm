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

#ifndef MIRACLE_WM_MOCK_CONFIGURATION_H
#define MIRACLE_WM_MOCK_CONFIGURATION_H

#include "config.h"
#include <gmock/gmock.h>

namespace miracle
{
namespace test
{
    class MockConfig : public Config
    {
    public:
        MOCK_METHOD(void, load, (mir::Server & server), (override));
        MOCK_METHOD(void, reload, (), (override));
        MOCK_METHOD(std::string const&, get_filename, (), (const, override));
        MOCK_METHOD(MirInputEventModifier, get_input_event_modifier, (), (const, override));
        MOCK_METHOD(CustomKeyCommand const*, matches_custom_key_command, (MirKeyboardAction action, int scan_code, unsigned int modifiers), (const, override));
        MOCK_METHOD(bool, matches_key_command, (MirKeyboardAction action, int scan_code, unsigned int modifiers, std::function<bool(DefaultKeyCommand)> const& f), (const, override));
        MOCK_METHOD(int, get_inner_gaps_x, (), (const, override));
        MOCK_METHOD(int, get_inner_gaps_y, (), (const, override));
        MOCK_METHOD(int, get_outer_gaps_x, (), (const, override));
        MOCK_METHOD(int, get_outer_gaps_y, (), (const, override));
        MOCK_METHOD(std::vector<StartupApp> const&, get_startup_apps, (), (const, override));
        MOCK_METHOD(std::optional<std::string> const&, get_terminal_command, (), (const, override));
        MOCK_METHOD(int, get_resize_jump, (), (const, override));
        MOCK_METHOD(std::vector<EnvironmentVariable> const&, get_env_variables, (), (const, override));
        MOCK_METHOD(BorderConfig const&, get_border_config, (), (const, override));
        MOCK_METHOD((std::array<AnimationDefinition, static_cast<int>(AnimateableEvent::max)> const&), get_animation_definitions, (), (const, override));
        MOCK_METHOD(bool, are_animations_enabled, (), (const, override));
        MOCK_METHOD(WorkspaceConfig, get_workspace_config, (std::optional<int> const& num, std::optional<std::string> const& name), (const, override));
        MOCK_METHOD(LayoutScheme, get_default_layout_scheme, (), (const, override));
        MOCK_METHOD(DragAndDropConfiguration, drag_and_drop, (), (const, override));
        MOCK_METHOD(int, register_listener, (std::function<void(miracle::Config&)> const&), (override));
        MOCK_METHOD(int, register_listener, (std::function<void(miracle::Config&)> const&, int priority), (override));
        MOCK_METHOD(void, unregister_listener, (int handle), (override));
        MOCK_METHOD(void, try_process_change, (), (override));
        MOCK_METHOD(uint, get_primary_modifier, (), (const, override));
        MOCK_METHOD(uint, move_modifier, (), (const, override));
    };
}
}

#endif // MIRACLE_WM_MOCK_CONFIGURATION_H

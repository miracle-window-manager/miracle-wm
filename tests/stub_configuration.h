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

#ifndef MIRACLE_WM_STUB_CONFIGURATION_H
#define MIRACLE_WM_STUB_CONFIGURATION_H

#include "config.h"

namespace miracle
{
namespace test
{
    class StubConfiguration : public miracle::MiracleConfig
    {
        void load(mir::Server& server) override { }
        [[nodiscard]] std::string const& get_filename() const override { return ""; }
        [[nodiscard]] MirInputEventModifier get_input_event_modifier() const override { return mir_input_event_modifier_none; }
        [[nodiscard]] CustomKeyCommand const* matches_custom_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers) const override
        {
            return nullptr;
        }

        bool matches_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers, std::function<bool(DefaultKeyCommand)> const& f) const override
        {
            return false;
        }

        [[nodiscard]] int get_inner_gaps_x() const override
        {
            return 0;
        }

        [[nodiscard]] int get_inner_gaps_y() const override
        {
            return 0;
        }

        [[nodiscard]] int get_outer_gaps_x() const override
        {
            return 0;
        }

        [[nodiscard]] int get_outer_gaps_y() const override
        {
            return 0;
        }

        [[nodiscard]] std::vector<StartupApp> const& get_startup_apps() const override
        {
            return {};
        }

        [[nodiscard]] std::optional<std::string> const& get_terminal_command() const override
        {
            return std::nullopt;
        }

        [[nodiscard]] int get_resize_jump() const override
        {
            return 0;
        }

        [[nodiscard]] std::vector<EnvironmentVariable> const& get_env_variables() const override
        {
            return {};
        }

        [[nodiscard]] BorderConfig const& get_border_config() const override
        {
            return border_config;
        }

        [[nodiscard]] std::array<AnimationDefinition, (int)AnimateableEvent::max> const& get_animation_definitions() const override
        {
            return animations;
        }

        [[nodiscard]] bool are_animations_enabled() const override
        {
            return false;
        }

        [[nodiscard]] WorkspaceConfig get_workspace_config(int key) const override
        {
            return WorkspaceConfig(key);
        }

        int register_listener(std::function<void(miracle::MiracleConfig&)> const&) override
        {
            return -1;
        }

        /// Register a listener on configuration change. A lower "priority" number signifies that the
        /// listener should be triggered earlier. A higher priority means later
        int register_listener(std::function<void(miracle::MiracleConfig&)> const&, int priority) override
        {
            return -1;
        }

        void unregister_listener(int handle) override
        {
        }

        void try_process_change() override { }

        uint get_primary_modifier() const override
        {
            return 0;
        }

        LayoutScheme get_default_layout_scheme() const override
        {
            return LayoutScheme::horizontal;
        }

    private:
        miracle::BorderConfig border_config;
        std::array<AnimationDefinition, (int)AnimateableEvent::max> animations;
    };
}
}

#endif // MIRACLE_WM_STUB_CONFIGURATION_H

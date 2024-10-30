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

#ifndef MIRACLEWM_CONFIG_H
#define MIRACLEWM_CONFIG_H

#include "animation_defintion.h"
#include "container.h"

#include <atomic>
#include <functional>
#include <glm/glm.hpp>
#include <linux/input.h>
#include <memory>
#include <mir/fd.h>
#include <miral/toolkit_event.h>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace mir
{
class Server;
}

namespace YAML
{
class Node;
}

namespace miral
{
class MirRunner;
class FdHandle;
}
namespace miracle
{

enum DefaultKeyCommand
{
    Terminal = 0,
    RequestVertical,
    RequestHorizontal,
    ToggleResize,
    ResizeUp,
    ResizeDown,
    ResizeLeft,
    ResizeRight,
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    SelectUp,
    SelectDown,
    SelectLeft,
    SelectRight,
    QuitActiveWindow,
    QuitCompositor,
    Fullscreen,
    SelectWorkspace1,
    SelectWorkspace2,
    SelectWorkspace3,
    SelectWorkspace4,
    SelectWorkspace5,
    SelectWorkspace6,
    SelectWorkspace7,
    SelectWorkspace8,
    SelectWorkspace9,
    SelectWorkspace0,
    MoveToWorkspace1,
    MoveToWorkspace2,
    MoveToWorkspace3,
    MoveToWorkspace4,
    MoveToWorkspace5,
    MoveToWorkspace6,
    MoveToWorkspace7,
    MoveToWorkspace8,
    MoveToWorkspace9,
    MoveToWorkspace0,
    ToggleFloating,
    TogglePinnedToWorkspace,
    ToggleTabbing,
    ToggleStacking,
    MAX
};

struct KeyCommand
{
    MirKeyboardAction action;
    uint modifiers;
    int key;
};

struct CustomKeyCommand : KeyCommand
{
    std::string command;
};

typedef std::vector<KeyCommand> KeyCommandList;

struct StartupApp
{
    std::string command;
    bool restart_on_death = false;
    bool no_startup_id = false;
    bool should_halt_compositor_on_death = false;
    bool in_systemd_scope = false;
};

struct EnvironmentVariable
{
    std::string key;
    std::string value;
};

struct BorderConfig
{
    int size = 0;
    glm::vec4 focus_color = glm::vec4(0);
    glm::vec4 color = glm::vec4(0);
};

struct WorkspaceConfig
{
    int num = -1;
    ContainerType layout = ContainerType::leaf;
};

enum class RenderFilter : int
{
    none,
    grayscale,
    protanopia,
    deuteranopia,
    tritanopia
};

class MiracleConfig
{
public:
    virtual ~MiracleConfig() = default;
    virtual void load(mir::Server& server) = 0;
    [[nodiscard]] virtual std::string const& get_filename() const = 0;
    [[nodiscard]] virtual MirInputEventModifier get_input_event_modifier() const = 0;
    [[nodiscard]] virtual CustomKeyCommand const* matches_custom_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers) const = 0;
    virtual bool matches_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers, std::function<bool(DefaultKeyCommand)> const& f) const = 0;
    [[nodiscard]] virtual int get_inner_gaps_x() const = 0;
    [[nodiscard]] virtual int get_inner_gaps_y() const = 0;
    [[nodiscard]] virtual int get_outer_gaps_x() const = 0;
    [[nodiscard]] virtual int get_outer_gaps_y() const = 0;
    [[nodiscard]] virtual std::vector<StartupApp> const& get_startup_apps() const = 0;
    [[nodiscard]] virtual std::optional<std::string> const& get_terminal_command() const = 0;
    [[nodiscard]] virtual int get_resize_jump() const = 0;
    [[nodiscard]] virtual std::vector<EnvironmentVariable> const& get_env_variables() const = 0;
    [[nodiscard]] virtual BorderConfig const& get_border_config() const = 0;
    [[nodiscard]] virtual std::array<AnimationDefinition, (int)AnimateableEvent::max> const& get_animation_definitions() const = 0;
    [[nodiscard]] virtual bool are_animations_enabled() const = 0;
    [[nodiscard]] virtual WorkspaceConfig get_workspace_config(int key) const = 0;
    [[nodiscard]] virtual LayoutScheme get_default_layout_scheme() const = 0;

    virtual int register_listener(std::function<void(miracle::MiracleConfig&)> const&) = 0;
    /// Register a listener on configuration change. A lower "priority" number signifies that the
    /// listener should be triggered earlier. A higher priority means later
    virtual int register_listener(std::function<void(miracle::MiracleConfig&)> const&, int priority) = 0;
    virtual void unregister_listener(int handle) = 0;
    virtual void try_process_change() = 0;
    virtual uint get_primary_modifier() const = 0;
};

class FilesystemConfiguration : public MiracleConfig
{
public:
    explicit FilesystemConfiguration(miral::MirRunner&);
    FilesystemConfiguration(miral::MirRunner&, std::string const&, bool load_immediately = false);
    ~FilesystemConfiguration() = default;
    FilesystemConfiguration(FilesystemConfiguration const&) = default;
    auto operator=(FilesystemConfiguration const&) -> FilesystemConfiguration& = default;

    void load(mir::Server& server) override;
    [[nodiscard]] std::string const& get_filename() const override;
    [[nodiscard]] MirInputEventModifier get_input_event_modifier() const override;
    [[nodiscard]] CustomKeyCommand const* matches_custom_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers) const override;
    bool matches_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers, std::function<bool(DefaultKeyCommand)> const& f) const override;
    [[nodiscard]] int get_inner_gaps_x() const override;
    [[nodiscard]] int get_inner_gaps_y() const override;
    [[nodiscard]] int get_outer_gaps_x() const override;
    [[nodiscard]] int get_outer_gaps_y() const override;
    [[nodiscard]] std::vector<StartupApp> const& get_startup_apps() const override;
    [[nodiscard]] std::optional<std::string> const& get_terminal_command() const override;
    [[nodiscard]] int get_resize_jump() const override;
    [[nodiscard]] std::vector<EnvironmentVariable> const& get_env_variables() const override;
    [[nodiscard]] BorderConfig const& get_border_config() const override;
    [[nodiscard]] std::array<AnimationDefinition, (int)AnimateableEvent::max> const& get_animation_definitions() const override;
    [[nodiscard]] bool are_animations_enabled() const override;
    [[nodiscard]] WorkspaceConfig get_workspace_config(int key) const override;
    [[nodiscard]] LayoutScheme get_default_layout_scheme() const override;
    int register_listener(std::function<void(miracle::MiracleConfig&)> const&) override;
    int register_listener(std::function<void(miracle::MiracleConfig&)> const&, int priority) override;
    void unregister_listener(int handle) override;
    void try_process_change() override;
    [[nodiscard]] uint get_primary_modifier() const override;

private:
    struct ChangeListener
    {
        std::function<void(miracle::MiracleConfig&)> listener;
        int priority;
        int handle;
    };

    static uint parse_modifier(std::string const& stringified_action_key);
    void _init(std::optional<StartupApp> const& systemd_app, std::optional<StartupApp> const& exec_app);
    void _reload();
    void _watch(miral::MirRunner& runner);
    void read_animation_definitions(YAML::Node const&);

    miral::MirRunner& runner;
    int next_listener_handle = 0;
    std::vector<ChangeListener> on_change_listeners;
    std::string default_config_path;
    std::string config_path;
    bool no_config = false;
    mir::Fd inotify_fd;
    std::unique_ptr<miral::FdHandle> watch_handle;
    int file_watch = 0;
    std::mutex mutex;
    std::atomic<bool> has_changes = false;
    bool is_loaded_ = false;

    static const uint miracle_input_event_modifier_default = 1 << 18;
    struct ConfigDetails
    {
        ConfigDetails();
        uint primary_modifier = mir_input_event_modifier_meta;
        std::vector<CustomKeyCommand> custom_key_commands;
        KeyCommandList key_commands[DefaultKeyCommand::MAX];
        int inner_gaps_x = 10;
        int inner_gaps_y = 10;
        int outer_gaps_x = 10;
        int outer_gaps_y = 10;
        std::vector<StartupApp> startup_apps;
        std::optional<std::string> terminal = "miracle-wm-sensible-terminal";
        std::string desired_terminal = "";
        int resize_jump = 50;
        std::vector<EnvironmentVariable> environment_variables;
        BorderConfig border_config;
        bool animations_enabled = true;
        std::array<AnimationDefinition, (int)AnimateableEvent::max> animation_defintions;
        std::vector<WorkspaceConfig> workspace_configs;
    };

    ConfigDetails options;
};
}

#endif

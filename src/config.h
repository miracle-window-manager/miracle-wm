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
#include "config_error_handler.h"
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
#include <yaml-cpp/yaml.h>

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
constexpr uint miracle_input_event_modifier_default = 1 << 18;

enum class DefaultKeyCommand
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
    std::optional<int> num;
    std::optional<ContainerType> layout;
    std::optional<std::string> name;
};

enum class RenderFilter : int
{
    none,
    grayscale,
    protanopia,
    deuteranopia,
    tritanopia
};

struct DragAndDropConfiguration
{
    bool enabled = true;
    uint modifiers = miracle_input_event_modifier_default;
};

class Config
{
public:
    virtual ~Config() = default;
    virtual void load(mir::Server& server) = 0;
    virtual void reload() = 0;
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
    [[nodiscard]] virtual std::array<AnimationDefinition, static_cast<int>(AnimateableEvent::max)> const& get_animation_definitions() const = 0;
    [[nodiscard]] virtual bool are_animations_enabled() const = 0;
    [[nodiscard]] virtual WorkspaceConfig get_workspace_config(std::optional<int> const& num, std::optional<std::string> const& name) const = 0;
    [[nodiscard]] virtual LayoutScheme get_default_layout_scheme() const = 0;
    [[nodiscard]] virtual DragAndDropConfiguration drag_and_drop() const = 0;

    virtual int register_listener(std::function<void(miracle::Config&)> const&) = 0;
    /// Register a listener on configuration change. A lower "priority" number signifies that the
    /// listener should be triggered earlier. A higher priority means later
    virtual int register_listener(std::function<void(miracle::Config&)> const&, int priority) = 0;
    virtual void unregister_listener(int handle) = 0;
    virtual void try_process_change() = 0;
    [[nodiscard]] virtual uint get_primary_modifier() const = 0;
    uint process_modifier(uint modifier) const;
};

class FilesystemConfiguration : public Config
{
public:
    explicit FilesystemConfiguration(miral::MirRunner&);
    FilesystemConfiguration(miral::MirRunner&, std::string const&, bool load_immediately = false);
    ~FilesystemConfiguration() override = default;
    FilesystemConfiguration(FilesystemConfiguration const&) = delete;
    auto operator=(FilesystemConfiguration const&) -> FilesystemConfiguration& = delete;

    void load(mir::Server& server) override;
    void reload() override;
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
    [[nodiscard]] std::array<AnimationDefinition, static_cast<int>(AnimateableEvent::max)> const& get_animation_definitions() const override;
    [[nodiscard]] bool are_animations_enabled() const override;
    [[nodiscard]] WorkspaceConfig get_workspace_config(std::optional<int> const& num, std::optional<std::string> const& name) const override;
    [[nodiscard]] LayoutScheme get_default_layout_scheme() const override;
    [[nodiscard]] DragAndDropConfiguration drag_and_drop() const override;
    int register_listener(std::function<void(miracle::Config&)> const&) override;
    int register_listener(std::function<void(miracle::Config&)> const&, int priority) override;
    void unregister_listener(int handle) override;
    void try_process_change() override;
    [[nodiscard]] uint get_primary_modifier() const override;

private:
    struct ConfigDetails
    {
        ConfigDetails();
        uint primary_modifier = mir_input_event_modifier_meta;
        std::vector<CustomKeyCommand> custom_key_commands;
        KeyCommandList key_commands[static_cast<int>(DefaultKeyCommand::MAX)];
        int inner_gaps_x = 10;
        int inner_gaps_y = 10;
        int outer_gaps_x = 10;
        int outer_gaps_y = 10;
        std::vector<StartupApp> startup_apps;
        std::optional<std::string> terminal = "miracle-wm-sensible-terminal";
        int resize_jump = 50;
        std::vector<EnvironmentVariable> environment_variables;
        BorderConfig border_config;
        bool animations_enabled = true;
        std::array<AnimationDefinition, static_cast<int>(AnimateableEvent::max)> animation_definitions;
        std::vector<WorkspaceConfig> workspace_configs;
        DragAndDropConfiguration drag_and_drop;
    };

    struct ChangeListener
    {
        std::function<void(miracle::Config&)> listener;
        int priority;
        int handle;
    };

    void _init(std::optional<StartupApp> const& systemd_app, std::optional<StartupApp> const& exec_app);
    void _watch(miral::MirRunner& runner);
    void add_error(YAML::Node const&);
    void read_action_key(YAML::Node const&);
    void read_default_action_overrides(YAML::Node const&);
    void read_custom_actions(YAML::Node const&);
    void read_inner_gaps(YAML::Node const&);
    void read_outer_gaps(YAML::Node const&);
    void read_startup_apps(YAML::Node const&);
    void read_terminal(YAML::Node const&);
    void read_resize_jump(YAML::Node const&);
    void read_environment_variables(YAML::Node const&);
    void read_border(YAML::Node const&);
    void read_workspaces(YAML::Node const&);
    void read_animation_definitions(YAML::Node const&);
    void read_enable_animations(YAML::Node const&);
    void read_drag_and_drop(YAML::Node const&);

    static std::optional<uint> try_parse_modifier(std::string const& stringified_action_key);

    template <typename T>
    bool try_parse_value(YAML::Node const& node, T& value)
    {
        try
        {
            value = node.as<T>();
        }
        catch (YAML::BadConversion const& e)
        {
            builder << "Unable to parse value to correct type";
            add_error(node);
            return false;
        }
        return true;
    }

    template <typename T>
    bool try_parse_value(YAML::Node const& root, const char* key, T& value, bool optional = false)
    {
        if (!root[key])
        {
            if (!optional)
            {
                builder << "Node is missing key: " << key;
                add_error(root);
            }
            return false;
        }

        return try_parse_value(root[key], value);
    }

    template <typename T>
    T try_parse_string_to_optional_value(YAML::Node const& node, std::function<T(std::string const&)> const& parse)
    {
        try
        {
            auto const& v = node.as<std::string>();
            T retval = parse(v);
            if (retval == std::nullopt)
            {
                builder << "Invalid option: " << v;
                add_error(node);
            }

            return retval;
        }
        catch (YAML::BadConversion const& e)
        {
            builder << "Unable to parse enum value";
            add_error(node);
            return std::nullopt;
        }
    }

    template <typename T>
    T try_parse_string_to_optional_value(YAML::Node const& root, const char* key, std::function<T(std::string const&)> const& parse)
    {
        if (!root[key])
        {
            builder << "Missing key in value: " << key;
            add_error(root);
            return std::nullopt;
        }

        return try_parse_string_to_optional_value(root[key], parse);
    }

    bool try_parse_color(YAML::Node const& node, glm::vec4&);
    bool try_parse_color(YAML::Node const& root, const char* key, glm::vec4&);
    bool try_parse_modifiers(YAML::Node const& node, uint& modifiers);

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
    std::stringstream builder;
    ConfigDetails options;
    ConfigErrorHandler error_handler;
};
}

#endif

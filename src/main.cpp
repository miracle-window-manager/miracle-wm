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

#define MIR_LOG_COMPONENT "miracle-main"

#include "auto_restarting_launcher.h"
#include "compositor_state.h"
#include "config.h"
#include "miracle_gl_config.h"
#include "policy.h"
#include "renderer.h"
#include "surface_tracker.h"
#include "version.h"

#include <libnotify/notify.h>
#include <mir/log.h>
#include <mir/options/option.h>
#include <mir/renderer/gl/gl_surface.h>
#include <mir/server.h>
#include <miral/add_init_callback.h>
#include <miral/append_event_filter.h>
#include <miral/custom_renderer.h>
#include <miral/display_configuration_option.h>
#include <miral/external_client.h>
#include <miral/keymap.h>
#include <miral/runner.h>
#include <miral/wayland_extensions.h>
#include <miral/window_management_options.h>
#include <miral/x11_support.h>
#include <miroil/open_gl_context.h>

#define PRINT_OPENING_MESSAGE(x) mir::log_info("Welcome to miracle-wm v%s", x);

using namespace miral;

/// Wraps another miral API so that we can gain access to the underlying Server.
class ServerMiddleman
{
public:
    explicit ServerMiddleman(std::function<void(::mir::Server&)> const& f) :
        f { f }
    {
    }
    void operator()(mir::Server& server) const
    {
        f(server);
    }

private:
    std::function<void(::mir::Server&)> f;
};

int main(int argc, char const* argv[])
{
    PRINT_OPENING_MESSAGE(MIRACLE_VERSION_STRING);
    MirRunner runner { argc, argv };
    miracle::CompositorState compositor_state;

    std::function<void()> shutdown_hook { [] {} };
    runner.add_stop_callback([&]
    { shutdown_hook(); });

    ExternalClientLauncher external_client_launcher;
    miracle::AutoRestartingLauncher auto_restarting_launcher(runner, external_client_launcher);
    miracle::SurfaceTracker surface_tracker;
    auto config = std::make_shared<miracle::FilesystemConfiguration>(runner);
    for (auto const& env : config->get_env_variables())
    {
        setenv(env.key.c_str(), env.value.c_str(), 1);
    }

    WindowManagerOptions* options;
    auto window_managers = ServerMiddleman(
        [&](mir::Server& server)
    {
        config->load(server);
        options = new WindowManagerOptions {
            add_window_manager_policy<miracle::Policy>(
                "tiling", auto_restarting_launcher, runner, config, surface_tracker, server, compositor_state)
        };
        (*options)(server);
    });

    Keymap config_keymap;

    notify_init("miracle-wm");

    WaylandExtensions wayland_extensions = WaylandExtensions {}
                                               .enable(miral::WaylandExtensions::zwlr_layer_shell_v1)
                                               .enable(miral::WaylandExtensions::zwlr_foreign_toplevel_manager_v1)
                                               .enable(miral::WaylandExtensions::zxdg_output_manager_v1)
                                               .enable(miral::WaylandExtensions::zwp_virtual_keyboard_manager_v1)
                                               .enable(miral::WaylandExtensions::zwlr_virtual_pointer_manager_v1)
                                               .enable(miral::WaylandExtensions::zwp_input_method_manager_v2)
                                               .enable(miral::WaylandExtensions::zwlr_screencopy_manager_v1)
                                               .enable(miral::WaylandExtensions::ext_session_lock_manager_v1);

    for (auto const& extension : { "zwp_pointer_constraints_v1", "zwp_relative_pointer_manager_v1" })
        wayland_extensions.enable(extension);

    return runner.run_with(
        { window_managers,
            wayland_extensions,
            X11Support {}.default_to_enabled(),
            config_keymap,
            external_client_launcher,
            display_configuration_options,
            AppendEventFilter([&config](MirEvent const*)
    {
        config->try_process_change();
        return false;
    }),
            CustomRenderer([&](std::unique_ptr<mir::graphics::gl::OutputSurface> x, std::shared_ptr<mir::graphics::GLRenderingProvider> y)
    {
        return std::make_unique<miracle::Renderer>(std::move(y), std::move(x), config, surface_tracker, compositor_state);
    }),
            miroil::OpenGLContext(new miracle::GLConfig()) });
}

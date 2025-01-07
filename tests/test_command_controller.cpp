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

#include "command_controller.h"
#include "mode_observer.h"
#include "scratchpad.h"
#include "stub_configuration.h"
#include "stub_window_controller.h"
#include "stub_container.h"
#include "workspace_manager.h"
#include "workspace_observer.h"

#include <gtest/gtest.h>
#include <miral/runner.h>

using namespace miracle;

class StubCommandControllerInteface : public CommandControllerInterface
{
public:
    void quit() override {}
};

class CommandControllerTest : public testing::Test
{
public:
    CommandControllerTest()
    : config(std::make_shared<test::StubConfiguration>()),
      window_controller(data),
      workspace_manager(workspace_registry, config, state),
      scratchpad(window_controller, state),
      command_controller(
        config,
        mutex,
        state,
        window_controller,
        workspace_manager,
        mode_observer_registrar,
        std::make_unique<StubCommandControllerInteface>(),
        scratchpad
      )
    {
    }

    std::shared_ptr<Config> config;
    std::recursive_mutex mutex;
    CompositorState state;
    std::vector<StubWindowData> data;
    StubWindowController window_controller;
    WorkspaceObserverRegistrar workspace_registry;
    WorkspaceManager workspace_manager;
    ModeObserverRegistrar mode_observer_registrar;
    Scratchpad scratchpad;
    CommandController command_controller;
};

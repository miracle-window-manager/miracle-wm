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
#include "drag_and_drop_service.h"
#include "mock_configuration.h"
#include "mock_container.h"
#include "mock_output_factory.h"
#include "mock_window_controller.h"
#include "mock_workspace.h"
#include "mode_observer.h"
#include "output_manager.h"
#include "scratchpad.h"
#include "workspace_manager.h"
#include "workspace_observer.h"
#include <gtest/gtest.h>
#include <memory>
#include <mutex>

using namespace miracle;

class StubCommandControllerInterface : public CommandControllerInterface
{
public:
    void quit() override { }
};

class CommandControllerTest : public testing::Test
{
public:
    CommandControllerTest() :
        output_manager(std::make_shared<OutputManager>(std::unique_ptr<test::MockOutputFactory>(output_factory))),
        config(std::make_shared<test::MockConfig>()),
        window_controller(std::make_shared<test::MockWindowController>()),
        workspace_manager(std::make_shared<WorkspaceManager>(workspace_registry, config, output_manager)),
        scratchpad(std::make_shared<Scratchpad>(window_controller, output_manager)),
        command_controller(std::make_shared<CommandController>(
            config,
            mutex,
            state,
            window_controller,
            workspace_manager,
            mode_observer_registrar,
            std::make_unique<StubCommandControllerInterface>(),
            scratchpad,
            output_manager))
    {
    }

    std::recursive_mutex mutex;
    test::MockOutputFactory* output_factory = new test::MockOutputFactory();
    std::shared_ptr<OutputManager> output_manager;
    std::shared_ptr<test::MockConfig> config;
    std::shared_ptr<test::MockWindowController> window_controller;
    std::shared_ptr<WorkspaceObserverRegistrar> workspace_registry = std::make_shared<WorkspaceObserverRegistrar>();
    std::shared_ptr<WorkspaceManager> workspace_manager;
    std::shared_ptr<Scratchpad> scratchpad;
    std::shared_ptr<ModeObserverRegistrar> mode_observer_registrar = std::make_shared<ModeObserverRegistrar>();
    std::shared_ptr<CompositorState> state = std::make_shared<CompositorState>();
    std::shared_ptr<CommandController> command_controller;
};

TEST_F(CommandControllerTest, cannot_move_active_to_same_workspace_by_number)
{
    auto container = std::make_shared<testing::NiceMock<test::MockContainer>>();
    state->add(container);
    state->focus_container(container);

    auto workspace = std::make_shared<testing::NiceMock<test::MockWorkspace>>();
    EXPECT_CALL(*container, get_workspace())
        .WillOnce(testing::Return(workspace.get()));
    EXPECT_CALL(*workspace, num())
        .WillOnce(testing::Return(1));

    ASSERT_FALSE(command_controller->move_active_to_workspace(1));
}

TEST_F(CommandControllerTest, cannot_move_active_to_same_workspace_by_name)
{
    auto container = std::make_shared<testing::NiceMock<test::MockContainer>>();
    state->add(container);
    state->focus_container(container);

    auto workspace = std::make_shared<testing::NiceMock<test::MockWorkspace>>();
    EXPECT_CALL(*container, get_workspace())
        .WillOnce(testing::Return(workspace.get()));
    std::optional<std::string> const name = "Test";
    EXPECT_CALL(*workspace, name())
        .WillOnce(testing::ReturnRef(name));

    std::string expected = "Test";
    ASSERT_FALSE(command_controller->move_active_to_workspace_named(expected, false));
}

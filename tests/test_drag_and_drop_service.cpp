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
#include "mock_output.h"
#include "mock_output_factory.h"
#include "mock_workspace.h"
#include "mode_observer.h"
#include "output_manager.h"
#include "scratchpad.h"
#include "stub_configuration.h"
#include "stub_container.h"
#include "stub_window_controller.h"
#include "workspace_manager.h"
#include "workspace_observer.h"
#include <gtest/gtest.h>
#include <mutex>

using namespace miracle;

class StubCommandControllerInterface : public CommandControllerInterface
{
public:
    void quit() override { }
};

class DragAndDropServiceTest : public testing::Test
{

public:
    DragAndDropServiceTest() :
        output_manager(std::make_shared<OutputManager>(std::unique_ptr<test::MockOutputFactory>(output_factory))),
        config(std::make_shared<test::MockConfig>()),
        window_controller(std::make_shared<StubWindowController>(data)),
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
            output_manager)),
        service(command_controller, config, output_manager)
    {
        ON_CALL(*config, drag_and_drop())
            .WillByDefault(::testing::Return(DragAndDropConfiguration {
                .enabled = true,
                .modifiers = mir_input_event_modifier_meta }));
    }

    std::recursive_mutex mutex;
    std::vector<StubWindowData> data;
    test::MockOutputFactory* output_factory = new test::MockOutputFactory();
    std::shared_ptr<OutputManager> output_manager;
    std::shared_ptr<test::MockConfig> config;
    std::shared_ptr<StubWindowController> window_controller;
    std::shared_ptr<WorkspaceObserverRegistrar> workspace_registry = std::make_shared<WorkspaceObserverRegistrar>();
    std::shared_ptr<WorkspaceManager> workspace_manager;
    std::shared_ptr<Scratchpad> scratchpad;
    std::shared_ptr<ModeObserverRegistrar> mode_observer_registrar = std::make_shared<ModeObserverRegistrar>();
    std::shared_ptr<CompositorState> state = std::make_shared<CompositorState>();
    std::shared_ptr<CommandController> command_controller;
    DragAndDropService service;
};

TEST_F(DragAndDropServiceTest, can_start_dragging)
{
    auto container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    test::MockOutput* mock_output = new test::MockOutput();
    ON_CALL(*mock_output, intersect(::testing::_, ::testing::_))
        .WillByDefault(::testing::Return(container));
    std::vector<std::shared_ptr<WorkspaceInterface>> workspaces;
    ON_CALL(*mock_output, get_workspaces())
        .WillByDefault(::testing::ReturnRef(workspaces));

    EXPECT_CALL(*output_factory, create(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::Return(std::unique_ptr<OutputInterface>(mock_output)));
    output_manager->create("Output1", 1, {
                                             { 0,    0    },
                                             { 1920, 1080 }
    },
        *workspace_manager);

    state->add(container);
    state->focus_container(container);

    ON_CALL(*container, drag_start())
        .WillByDefault(::testing::Return(true));

    ASSERT_TRUE(service.handle_pointer_event(
        *state,
        100,
        100,
        mir_pointer_action_button_down,
        mir_input_event_modifier_meta));

    ASSERT_EQ(state->mode(), WindowManagerMode::dragging);
}

TEST_F(DragAndDropServiceTest, can_stop_dragging)
{
    auto container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    test::MockOutput* mock_output = new test::MockOutput();
    ON_CALL(*mock_output, intersect(::testing::_, ::testing::_))
        .WillByDefault(::testing::Return(container));
    std::vector<std::shared_ptr<WorkspaceInterface>> workspaces;
    ON_CALL(*mock_output, get_workspaces())
        .WillByDefault(::testing::ReturnRef(workspaces));
    EXPECT_CALL(*output_factory, create(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::Return(std::unique_ptr<OutputInterface>(mock_output)));
    output_manager->create("Output1", 1, {
                                             { 0,    0    },
                                             { 1920, 1080 }
    },
        *workspace_manager);

    state->add(container);
    state->focus_container(container);

    ON_CALL(*container, drag_start())
        .WillByDefault(::testing::Return(true));

    service.handle_pointer_event(
        *state,
        100,
        100,
        mir_pointer_action_button_down,
        mir_input_event_modifier_meta);

    ASSERT_EQ(state->mode(), WindowManagerMode::dragging);

    service.handle_pointer_event(
        *state,
        100,
        100,
        mir_pointer_action_button_up,
        mir_input_event_modifier_meta);

    ASSERT_EQ(state->mode(), WindowManagerMode::normal);
}

TEST_F(DragAndDropServiceTest, can_drag_to_other_container)
{
    test::MockOutput* mock_output = new test::MockOutput();
    std::vector<std::shared_ptr<WorkspaceInterface>> workspaces;
    ON_CALL(*mock_output, get_workspaces())
        .WillByDefault(::testing::ReturnRef(workspaces));
    EXPECT_CALL(*output_factory, create(::testing::_, ::testing::_, ::testing::_))
        .WillOnce(testing::Return(std::unique_ptr<OutputInterface>(mock_output)));
    output_manager->create("Output1", 1, {
                                             { 0,    0    },
                                             { 1920, 1080 }
    },
        *workspace_manager);

    auto container_drag = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    state->add(container_drag);
    state->focus_container(container_drag);
    ON_CALL(*mock_output, intersect(::testing::_, ::testing::_))
        .WillByDefault(::testing::Return(container_drag));

    ON_CALL(*container_drag, drag_start())
        .WillByDefault(::testing::Return(true));

    service.handle_pointer_event(
        *state,
        100,
        100,
        mir_pointer_action_button_down,
        mir_input_event_modifier_meta);

    auto other_container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    state->add(other_container);

    std::shared_ptr<test::MockWorkspace> workspace = std::make_shared<test::MockWorkspace>();
    ON_CALL(*mock_output, active())
        .WillByDefault(::testing::Return(workspace));
    ON_CALL(*mock_output, intersect_leaf(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Return(other_container));
    ON_CALL(*workspace, is_empty())
        .WillByDefault(::testing::Return(false));
    ON_CALL(*container_drag, get_type())
        .WillByDefault(::testing::Return(ContainerType::leaf));
    ON_CALL(*other_container, get_type())
        .WillByDefault(::testing::Return(ContainerType::leaf));
    ON_CALL(*container_drag, get_workspace())
        .WillByDefault(::testing::Return(nullptr));

    ON_CALL(*other_container, get_workspace())
        .WillByDefault(::testing::Return(workspace.get()));

    EXPECT_CALL(*container_drag, move_to(::testing::_));

    service.handle_pointer_event(
        *state,
        500,
        500,
        mir_pointer_action_button_down,
        mir_input_event_modifier_none);
}

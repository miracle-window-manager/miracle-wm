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

#include "drag_and_drop_service.h"
#include "mock_configuration.h"
#include "mock_container.h"
#include "mock_tiling_window_tree.h"
#include "mock_workspace.h"
#include "output_manager.h"
#include "with_command_controller.h"
#include <gtest/gtest.h>

using namespace miracle;

class DragAndDropServiceTest : public testing::Test, public test::WithCommandController
{

public:
    DragAndDropServiceTest() :
        config(std::make_shared<::testing::NiceMock<test::MockConfig>>()),
        service(command_controller, config, &output_manager)
    {
        ON_CALL(*config, drag_and_drop())
            .WillByDefault(::testing::Return(DragAndDropConfiguration {
                .enabled = true,
                .modifiers = mir_input_event_modifier_meta }));
    }

    std::shared_ptr<::testing::NiceMock<test::MockConfig>> config;
    DragAndDropService service;
};

TEST_F(DragAndDropServiceTest, can_start_dragging)
{
    auto container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    state.add(container);
    state.focus_container(container);
    ON_CALL(*output_factory->output, intersect(::testing::_, ::testing::_))
        .WillByDefault(::testing::Return(container));

    ON_CALL(*container, drag_start())
        .WillByDefault(::testing::Return(true));

    ASSERT_TRUE(service.handle_pointer_event(
        state,
        100,
        100,
        mir_pointer_action_button_down,
        mir_input_event_modifier_meta));

    ASSERT_EQ(state.mode(), WindowManagerMode::dragging);
}

TEST_F(DragAndDropServiceTest, can_stop_dragging)
{
    auto container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    state.add(container);
    state.focus_container(container);
    ON_CALL(*output_factory->output, intersect(::testing::_, ::testing::_))
        .WillByDefault(::testing::Return(container));

    ON_CALL(*container, drag_start())
        .WillByDefault(::testing::Return(true));

    service.handle_pointer_event(
        state,
        100,
        100,
        mir_pointer_action_button_down,
        mir_input_event_modifier_meta);

    ASSERT_EQ(state.mode(), WindowManagerMode::dragging);

    service.handle_pointer_event(
        state,
        100,
        100,
        mir_pointer_action_button_up,
        mir_input_event_modifier_meta);

    ASSERT_EQ(state.mode(), WindowManagerMode::normal);
}

TEST_F(DragAndDropServiceTest, can_drag_to_other_container)
{
    auto container_drag = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    state.add(container_drag);
    state.focus_container(container_drag);
    ON_CALL(*output_factory->output, intersect(::testing::_, ::testing::_))
        .WillByDefault(::testing::Return(container_drag));

    ON_CALL(*container_drag, drag_start())
        .WillByDefault(::testing::Return(true));

    service.handle_pointer_event(
        state,
        100,
        100,
        mir_pointer_action_button_down,
        mir_input_event_modifier_meta);

    auto other_container = std::make_shared<::testing::NiceMock<test::MockContainer>>();
    state.add(other_container);

    std::shared_ptr<test::MockWorkspace> workspace = std::make_shared<test::MockWorkspace>();
    std::shared_ptr<test::MockTilingWindowTree> tree = std::make_shared<test::MockTilingWindowTree>();
    ON_CALL(*output_factory->output, active())
        .WillByDefault(::testing::Return(workspace.get()));
    ON_CALL(*output_factory->output, intersect_leaf(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Return(other_container));
    ON_CALL(*workspace, get_tree())
        .WillByDefault(::testing::Return(tree.get()));
    ON_CALL(*tree, is_empty())
        .WillByDefault(::testing::Return(false));
    ON_CALL(*container_drag, get_type())
        .WillByDefault(::testing::Return(ContainerType::leaf));
    ON_CALL(*other_container, get_type())
        .WillByDefault(::testing::Return(ContainerType::leaf));
    ON_CALL(*container_drag, tree())
        .WillByDefault(::testing::Return(nullptr));

    ON_CALL(*other_container, tree())
        .WillByDefault(::testing::Return(tree.get()));

    EXPECT_CALL(*tree, move_to);

    service.handle_pointer_event(
        state,
        500,
        500,
        mir_pointer_action_button_down,
        mir_input_event_modifier_none);
}

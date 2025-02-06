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

#include "mock_configuration.h"
#include "mock_container.h"
#include "mock_output.h"
#include "mock_output_factory.h"
#include "mock_window_controller.h"
#include "output_manager.h"
#include "scratchpad.h"
#include "workspace_manager.h"
#include "workspace_observer.h"
#include <gtest/gtest.h>

using namespace miracle;

class ScratchpadTest : public testing::Test
{
public:
};

TEST_F(ScratchpadTest, can_add_leaf_container_to_scratchpad)
{
    auto window_controller = std::make_shared<test::MockWindowController>();
    auto output_factory = std::make_unique<test::MockOutputFactory>();
    auto output_manager = std::make_shared<OutputManager>(std::move(output_factory));
    Scratchpad scratchpad(window_controller, output_manager);

    auto container = std::make_shared<test::MockContainer>();
    EXPECT_CALL(*container, get_type())
        .WillOnce(::testing::Return(ContainerType::leaf));
    EXPECT_CALL(*container, get_workspace())
        .WillOnce(::testing::Return(nullptr));
    EXPECT_CALL(*container, scratchpad_state(ScratchpadState::fresh));
    EXPECT_CALL(*container, set_workspace(nullptr));
    EXPECT_CALL(*container, hide());

    EXPECT_TRUE(scratchpad.move_to(container));
    EXPECT_TRUE(scratchpad.contains(container));
}

TEST_F(ScratchpadTest, cannot_add_non_leaf_container_to_scratchpad)
{
    auto window_controller = std::make_shared<test::MockWindowController>();
    auto output_factory = std::make_unique<test::MockOutputFactory>();
    auto output_manager = std::make_shared<OutputManager>(std::move(output_factory));
    Scratchpad scratchpad(window_controller, output_manager);

    auto container = std::make_shared<test::MockContainer>();
    EXPECT_CALL(*container, get_type())
        .WillRepeatedly(::testing::Return(ContainerType::parent));

    EXPECT_FALSE(scratchpad.move_to(container));
    EXPECT_FALSE(scratchpad.contains(container));
}

TEST_F(ScratchpadTest, can_show_container)
{
    // Setup
    auto window_controller = std::make_shared<test::MockWindowController>();
    auto output_factory = std::make_unique<test::MockOutputFactory>();
    auto output = new test::MockOutput();
    EXPECT_CALL(*output_factory, create(testing::_, testing::_, testing::_))
        .WillOnce(::testing::Return(std::unique_ptr<OutputInterface>(output)));
    mir::geometry::Rectangle output_area(
        mir::geometry::Point(0, 0),
        mir::geometry::Size(1920, 1280));
    EXPECT_CALL(*output, get_area())
        .WillOnce(::testing::ReturnRef(output_area));
    std::vector<std::shared_ptr<WorkspaceInterface>> empty_workspaces;
    EXPECT_CALL(*output, get_workspaces())
        .WillRepeatedly(::testing::ReturnRef(empty_workspaces));
    EXPECT_CALL(*output, id())
        .WillRepeatedly(::testing::Return(1));
    auto output_manager = std::make_shared<OutputManager>(std::move(output_factory));
    auto workspace_registry = std::make_shared<WorkspaceObserverRegistrar>();
    auto config = std::make_shared<testing::NiceMock<test::MockConfig>>();
    auto workspace_manager = std::make_shared<WorkspaceManager>(workspace_registry, config, output_manager);
    output_manager->create("Test", 1, mir::geometry::Rectangle {}, *workspace_manager);
    output_manager->focus(1);

    // Add the container
    Scratchpad scratchpad(window_controller, output_manager);
    auto container = std::make_shared<test::MockContainer>();
    EXPECT_CALL(*container, get_type())
        .WillOnce(::testing::Return(ContainerType::leaf));
    EXPECT_CALL(*container, get_workspace())
        .WillOnce(::testing::Return(nullptr));
    EXPECT_CALL(*container, scratchpad_state(ScratchpadState::fresh));
    EXPECT_CALL(*container, set_workspace(nullptr));
    EXPECT_CALL(*container, hide());
    EXPECT_TRUE(scratchpad.move_to(container));

    // Show the container and assert the position
    EXPECT_CALL(*container, scratchpad_state(ScratchpadState::changed));
    EXPECT_CALL(*container, show());
    EXPECT_CALL(*container, window())
        .WillOnce(::testing::Return(miral::Window()));
    EXPECT_CALL(*window_controller, modify(testing::_, testing::_));
    EXPECT_CALL(*window_controller, noclip(testing::_));
    EXPECT_TRUE(scratchpad.toggle_show(container));
}
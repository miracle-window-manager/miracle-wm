/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mock_output.h"
#include "mock_output_factory.h"
#include "output_manager.h"
#include "stub_configuration.h"
#include "workspace_manager.h"
#include "workspace_observer.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <mir/geometry/rectangle.h>

using namespace miracle;

class OutputManagerTest : public testing::Test
{
};

TEST(OutputManagerTest, create_output_success)
{
    // Arrange
    auto mock_factory = std::make_unique<test::MockOutputFactory>();
    auto mock_output = new test::MockOutput(); // Will be owned by unique_ptr

    EXPECT_CALL(*mock_factory, create("Output1", 1, mir::geometry::Rectangle {
                                                        { 0,    0    },
                                                        { 1920, 1080 }
    }))
        .WillOnce(testing::Return(std::unique_ptr<OutputInterface>(mock_output))); // Mock return value

    static const std::vector<std::shared_ptr<WorkspaceInterface>> empty_workspaces;
    ON_CALL(*mock_output, get_workspaces).WillByDefault(::testing::ReturnRef(empty_workspaces));

    auto workspace_registry = std::make_shared<WorkspaceObserverRegistrar>();
    auto config = std::make_shared<test::StubConfiguration>();
    auto manager = std::make_shared<OutputManager>(std::move(mock_factory));
    auto workspace_manager = std::make_shared<WorkspaceManager>(workspace_registry, config, manager);

    // Act
    OutputInterface* created_output = manager->create("Output1", 1, {
                                                                        { 0,    0    },
                                                                        { 1920, 1080 }
    },
        *workspace_manager);

    // Assert
    EXPECT_EQ(created_output, mock_output);
    ASSERT_EQ(manager->outputs().size(), 1);
    EXPECT_EQ(manager->outputs()[0].get(), mock_output);
}

TEST(OutputManagerTest, update_output_area)
{
    // Arrange
    auto mock_factory = std::make_unique<test::MockOutputFactory>();
    auto mock_output = new test::MockOutput(); // Will be owned by unique_ptr

    EXPECT_CALL(*mock_factory, create("Output1", 1, mir::geometry::Rectangle {
                                                        { 0,    0    },
                                                        { 1920, 1080 }
    }))
        .WillOnce(testing::Return(std::unique_ptr<OutputInterface>(mock_output)));

    ON_CALL(*mock_output, id())
        .WillByDefault(testing::Return(1));
    EXPECT_CALL(*mock_output, update_area(mir::geometry::Rectangle {
                                  { 0,    0   },
                                  { 1280, 720 }
    }));

    static const std::vector<std::shared_ptr<WorkspaceInterface>> empty_workspaces;
    ON_CALL(*mock_output, get_workspaces).WillByDefault(::testing::ReturnRef(empty_workspaces));

    auto workspace_registry = std::make_shared<WorkspaceObserverRegistrar>();
    auto config = std::make_shared<test::StubConfiguration>();
    auto manager = std::make_shared<OutputManager>(std::move(mock_factory));
    auto workspace_manager = std::make_shared<WorkspaceManager>(workspace_registry, config, manager);

    // Create output
    manager->create("Output1", 1, {
                                      { 0,    0    },
                                      { 1920, 1080 }
    },
        *workspace_manager);

    // Act
    manager->update(1, {
                           { 0,    0   },
                           { 1280, 720 }
    });
}

TEST(OutputManagerTest, remove_output)
{
    // Arrange
    auto mock_factory = std::make_unique<test::MockOutputFactory>();
    auto mock_output = new test::MockOutput(); // Will be owned by unique_ptr

    EXPECT_CALL(*mock_factory, create("Output1", 1, mir::geometry::Rectangle {
                                                        { 0,    0    },
                                                        { 1920, 1080 }
    }))
        .WillOnce(testing::Return(std::unique_ptr<OutputInterface>(mock_output)));

    static const std::vector<std::shared_ptr<WorkspaceInterface>> empty_workspaces;
    ON_CALL(*mock_output, get_workspaces).WillByDefault(::testing::ReturnRef(empty_workspaces));

    ON_CALL(*mock_output, id())
        .WillByDefault(testing::Return(1));

    EXPECT_CALL(*mock_output, set_defunct());

    auto workspace_registry = std::make_shared<WorkspaceObserverRegistrar>();
    auto config = std::make_shared<test::StubConfiguration>();
    auto manager = std::make_shared<OutputManager>(std::move(mock_factory));
    auto workspace_manager = std::make_shared<WorkspaceManager>(workspace_registry, config, manager);

    // Create output
    manager->create("Output1", 1, {
                                      { 0,    0    },
                                      { 1920, 1080 }
    },
        *workspace_manager);
    ASSERT_EQ(manager->outputs().size(), 1);

    // Act
    bool removed = manager->remove(1, *workspace_manager);

    // Assert
    EXPECT_TRUE(removed);
    EXPECT_EQ(manager->outputs().size(), 1);
}

TEST(OutputManagerTest, focus_and_unfocus)
{
    // Arrange
    auto mock_factory = std::make_unique<test::MockOutputFactory>();
    auto mock_output = new test::MockOutput(); // Will be owned by unique_ptr

    EXPECT_CALL(*mock_factory, create("Output1", 1, mir::geometry::Rectangle {
                                                        { 0,    0    },
                                                        { 1920, 1080 }
    }))
        .WillOnce(testing::Return(std::unique_ptr<OutputInterface>(mock_output)));
    ON_CALL(*mock_output, id())
        .WillByDefault(testing::Return(1));

    static const std::vector<std::shared_ptr<WorkspaceInterface>> empty_workspaces;
    ON_CALL(*mock_output, get_workspaces).WillByDefault(::testing::ReturnRef(empty_workspaces));

    auto workspace_registry = std::make_shared<WorkspaceObserverRegistrar>();
    auto config = std::make_shared<test::StubConfiguration>();
    auto manager = std::make_shared<OutputManager>(std::move(mock_factory));
    auto workspace_manager = std::make_shared<WorkspaceManager>(workspace_registry, config, manager);

    // Create output
    manager->create("Output1", 1, {
                                      { 0,    0    },
                                      { 1920, 1080 }
    },
        *workspace_manager);

    // Act
    bool focused = manager->focus(1);
    OutputInterface* focused_output = manager->focused();

    // Assert
    EXPECT_TRUE(focused);
    EXPECT_EQ(focused_output, mock_output);

    // Act: Unfocus
    bool unfocused = manager->unfocus(1);
    OutputInterface* after_unfocus = manager->focused();

    // Assert
    EXPECT_TRUE(unfocused);
    EXPECT_EQ(after_unfocus, nullptr);
}

TEST(OutputManagerTest, remove_focused_output)
{
    // Arrange
    auto mock_factory = std::make_unique<test::MockOutputFactory>();
    auto mock_output = new test::MockOutput(); // Will be owned by unique_ptr

    EXPECT_CALL(*mock_factory, create("Output1", 1, mir::geometry::Rectangle {
                                                        { 0,    0    },
                                                        { 1920, 1080 }
    }))
        .WillOnce(testing::Return(std::unique_ptr<OutputInterface>(mock_output)));
    ON_CALL(*mock_output, id())
        .WillByDefault(testing::Return(1));

    static const std::vector<std::shared_ptr<WorkspaceInterface>> empty_workspaces;
    ON_CALL(*mock_output, get_workspaces).WillByDefault(::testing::ReturnRef(empty_workspaces));

    auto workspace_registry = std::make_shared<WorkspaceObserverRegistrar>();
    auto config = std::make_shared<test::StubConfiguration>();
    auto manager = std::make_shared<OutputManager>(std::move(mock_factory));
    auto workspace_manager = std::make_shared<WorkspaceManager>(workspace_registry, config, manager);

    // Create and focus output
    manager->create("Output1", 1, {
                                      { 0,    0    },
                                      { 1920, 1080 }
    },
        *workspace_manager);
    manager->focus(1);
    ASSERT_EQ(manager->focused(), mock_output);

    // Act: Remove focused output
    bool removed = manager->remove(1, *workspace_manager);
    OutputInterface* focused_output = manager->focused();

    // Assert
    EXPECT_TRUE(removed);
    EXPECT_EQ(focused_output, nullptr);
    EXPECT_TRUE(manager->outputs().size() == 1);
}

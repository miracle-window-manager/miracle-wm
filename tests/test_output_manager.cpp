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
    test::MockOutput* mock_output = new test::MockOutput(); // Will be owned by unique_ptr

    EXPECT_CALL(*mock_factory, create("Output1", 1, mir::geometry::Rectangle {
                                                        { 0,    0    },
                                                        { 1920, 1080 }
    },
                                   ::testing::_))
        .WillOnce(testing::Return(std::unique_ptr<Output>(mock_output))); // Mock return value

    OutputManager manager(std::move(mock_factory));

    // Act
    Output* created_output = manager.create("Output1", 1, {
                                                              { 0,    0    },
                                                              { 1920, 1080 }
    });

    // Assert
    EXPECT_EQ(created_output, mock_output);
    ASSERT_EQ(manager.outputs().size(), 1);
    EXPECT_EQ(manager.outputs()[0].get(), mock_output);
}

TEST(OutputManagerTest, update_output_area)
{
    // Arrange
    auto mock_factory = std::make_unique<test::MockOutputFactory>();
    test::MockOutput* mock_output = new test::MockOutput(); // Will be owned by unique_ptr

    EXPECT_CALL(*mock_factory, create("Output1", 1, mir::geometry::Rectangle {
                                                        { 0,    0    },
                                                        { 1920, 1080 }
    },
                                   ::testing::_))
        .WillOnce(testing::Return(std::unique_ptr<Output>(mock_output)));

    ON_CALL(*mock_output, id())
        .WillByDefault(testing::Return(1));
    EXPECT_CALL(*mock_output, update_area(mir::geometry::Rectangle {
                                  { 0,    0   },
                                  { 1280, 720 }
    }));

    OutputManager manager(std::move(mock_factory));

    // Create output
    manager.create("Output1", 1, {
                                     { 0,    0    },
                                     { 1920, 1080 }
    });

    // Act
    manager.update(1, {
                          { 0,    0   },
                          { 1280, 720 }
    });
}

TEST(OutputManagerTest, remove_output)
{
    // Arrange
    auto mock_factory = std::make_unique<test::MockOutputFactory>();
    test::MockOutput* mock_output = new test::MockOutput(); // Will be owned by unique_ptr

    EXPECT_CALL(*mock_factory, create("Output1", 1, mir::geometry::Rectangle {
                                                        { 0,    0    },
                                                        { 1920, 1080 }
    },
                                   ::testing::_))
        .WillOnce(testing::Return(std::unique_ptr<Output>(mock_output)));

    ON_CALL(*mock_output, id())
        .WillByDefault(testing::Return(1));

    EXPECT_CALL(*mock_output, set_defunct());

    OutputManager manager(std::move(mock_factory));

    // Create output
    manager.create("Output1", 1, {
                                     { 0,    0    },
                                     { 1920, 1080 }
    });
    ASSERT_EQ(manager.outputs().size(), 1);

    // Act
    bool removed = manager.remove(1);

    // Assert
    EXPECT_TRUE(removed);
    EXPECT_EQ(manager.outputs().size(), 1);
}

TEST(OutputManagerTest, focus_and_unfocus)
{
    // Arrange
    auto mock_factory = std::make_unique<test::MockOutputFactory>();
    test::MockOutput* mock_output = new test::MockOutput(); // Will be owned by unique_ptr

    EXPECT_CALL(*mock_factory, create("Output1", 1, mir::geometry::Rectangle {
                                                        { 0,    0    },
                                                        { 1920, 1080 }
    },
                                   ::testing::_))
        .WillOnce(testing::Return(std::unique_ptr<Output>(mock_output)));
    ON_CALL(*mock_output, id())
        .WillByDefault(testing::Return(1));

    OutputManager manager(std::move(mock_factory));

    // Create output
    manager.create("Output1", 1, {
                                     { 0,    0    },
                                     { 1920, 1080 }
    });

    // Act
    bool focused = manager.focus(1);
    Output* focused_output = manager.focused();

    // Assert
    EXPECT_TRUE(focused);
    EXPECT_EQ(focused_output, mock_output);

    // Act: Unfocus
    bool unfocused = manager.unfocus(1);
    Output* after_unfocus = manager.focused();

    // Assert
    EXPECT_TRUE(unfocused);
    EXPECT_EQ(after_unfocus, nullptr);
}

TEST(OutputManagerTest, remove_focused_output)
{
    // Arrange
    auto mock_factory = std::make_unique<test::MockOutputFactory>();
    test::MockOutput* mock_output = new test::MockOutput(); // Will be owned by unique_ptr

    EXPECT_CALL(*mock_factory, create("Output1", 1, mir::geometry::Rectangle {
                                                        { 0,    0    },
                                                        { 1920, 1080 }
    },
                                   ::testing::_))
        .WillOnce(testing::Return(std::unique_ptr<Output>(mock_output)));
    ON_CALL(*mock_output, id())
        .WillByDefault(testing::Return(1));

    OutputManager manager(std::move(mock_factory));

    // Create and focus output
    manager.create("Output1", 1, {
                                     { 0,    0    },
                                     { 1920, 1080 }
    });
    manager.focus(1);
    ASSERT_EQ(manager.focused(), mock_output);

    // Act: Remove focused output
    bool removed = manager.remove(1);
    Output* focused_output = manager.focused();

    // Assert
    EXPECT_TRUE(removed);
    EXPECT_EQ(focused_output, nullptr);
    EXPECT_TRUE(manager.outputs().size() == 1);
}
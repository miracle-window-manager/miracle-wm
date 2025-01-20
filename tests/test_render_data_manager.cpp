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

#include "mock_container.h"
#include "render_data_manager.h"
#include <gtest/gtest.h>

using namespace miracle;

class RenderDataManagerTest : public testing::Test
{
public:
    RenderDataManager render_data_manager;
};

TEST_F(RenderDataManagerTest, values_are_populated_when_container_added)
{
    ::testing::NiceMock<test::MockContainer> container;
    ON_CALL(container, window())
        .WillByDefault(::testing::Return(miral::Window()));
    ON_CALL(container, get_type())
        .WillByDefault(::testing::Return(ContainerType::leaf));
    ON_CALL(container, get_output_transform())
        .WillByDefault(::testing::Return(glm::mat4(1.f)));
    ON_CALL(container, get_workspace_transform())
        .WillByDefault(::testing::Return(glm::mat4(1.f)));
    ON_CALL(container, get_transform())
        .WillByDefault(::testing::Return(glm::mat4(1.f)));
    ON_CALL(container, is_focused())
        .WillByDefault(::testing::Return(true));

    render_data_manager.add(container);

    auto result = render_data_manager.get();
    ASSERT_EQ(result.size(), 1);
    ASSERT_TRUE(result[0].needs_outline);
    ASSERT_TRUE(result[0].is_focused);
    ASSERT_EQ(result[0].transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].workspace_transform, glm::mat4(1.f));
}

TEST_F(RenderDataManagerTest, can_change_transform)
{
    ::testing::NiceMock<test::MockContainer> container;
    ON_CALL(container, window())
        .WillByDefault(::testing::Return(miral::Window()));
    ON_CALL(container, get_type())
        .WillByDefault(::testing::Return(ContainerType::leaf));
    ON_CALL(container, get_output_transform())
        .WillByDefault(::testing::Return(glm::mat4(1.f)));
    ON_CALL(container, get_workspace_transform())
        .WillByDefault(::testing::Return(glm::mat4(1.f)));
    ON_CALL(container, get_transform())
        .WillByDefault(::testing::Return(glm::mat4(1.f)));
    ON_CALL(container, is_focused())
        .WillByDefault(::testing::Return(true));

    render_data_manager.add(container);

    ON_CALL(container, get_transform())
        .WillByDefault(::testing::Return(glm::mat4(2.f)));
    render_data_manager.transform_change(container);

    auto result = render_data_manager.get();
    ASSERT_EQ(result.size(), 1);
    ASSERT_TRUE(result[0].needs_outline);
    ASSERT_TRUE(result[0].is_focused);
    ASSERT_EQ(result[0].transform, glm::mat4(2.f));
    ASSERT_EQ(result[0].workspace_transform, glm::mat4(1.f));
}

TEST_F(RenderDataManagerTest, can_change_workspace_transform)
{
    ::testing::NiceMock<test::MockContainer> container;
    ON_CALL(container, window())
        .WillByDefault(::testing::Return(miral::Window()));
    ON_CALL(container, get_type())
        .WillByDefault(::testing::Return(ContainerType::leaf));
    ON_CALL(container, get_output_transform())
        .WillByDefault(::testing::Return(glm::mat4(1.f)));
    ON_CALL(container, get_workspace_transform())
        .WillByDefault(::testing::Return(glm::mat4(1.f)));
    ON_CALL(container, get_transform())
        .WillByDefault(::testing::Return(glm::mat4(1.f)));
    ON_CALL(container, is_focused())
        .WillByDefault(::testing::Return(true));

    render_data_manager.add(container);

    ON_CALL(container, get_workspace_transform())
        .WillByDefault(::testing::Return(glm::mat4(2.f)));
    render_data_manager.workspace_transform_change(container);

    auto result = render_data_manager.get();
    ASSERT_EQ(result.size(), 1);
    ASSERT_TRUE(result[0].needs_outline);
    ASSERT_TRUE(result[0].is_focused);
    ASSERT_EQ(result[0].transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].workspace_transform, glm::mat4(2.f));
}

TEST_F(RenderDataManagerTest, can_change_focus)
{
    ::testing::NiceMock<test::MockContainer> container;
    ON_CALL(container, window())
        .WillByDefault(::testing::Return(miral::Window()));
    ON_CALL(container, get_type())
        .WillByDefault(::testing::Return(ContainerType::leaf));
    ON_CALL(container, get_output_transform())
        .WillByDefault(::testing::Return(glm::mat4(1.f)));
    ON_CALL(container, get_workspace_transform())
        .WillByDefault(::testing::Return(glm::mat4(1.f)));
    ON_CALL(container, get_transform())
        .WillByDefault(::testing::Return(glm::mat4(1.f)));
    ON_CALL(container, is_focused())
        .WillByDefault(::testing::Return(true));

    render_data_manager.add(container);

    ON_CALL(container, is_focused())
        .WillByDefault(::testing::Return(false));
    render_data_manager.focus_change(container);

    auto result = render_data_manager.get();
    ASSERT_EQ(result.size(), 1);
    ASSERT_TRUE(result[0].needs_outline);
    ASSERT_FALSE(result[0].is_focused);
    ASSERT_EQ(result[0].transform, glm::mat4(1.f));
    ASSERT_EQ(result[0].workspace_transform, glm::mat4(1.f));
}

class RenderDataManagerParameterizedTest : public RenderDataManagerTest, public ::testing::WithParamInterface<int>
{
};

TEST_P(RenderDataManagerParameterizedTest, can_add_many_containers)
{
    int value = GetParam();
    for (int i = 0; i < value; i++)
    {
        ::testing::NiceMock<test::MockContainer> container;
        ON_CALL(container, window())
            .WillByDefault(::testing::Return(miral::Window()));
        ON_CALL(container, get_type())
            .WillByDefault(::testing::Return(ContainerType::leaf));
        ON_CALL(container, get_output_transform())
            .WillByDefault(::testing::Return(glm::mat4(1.f)));
        ON_CALL(container, get_workspace_transform())
            .WillByDefault(::testing::Return(glm::mat4(1.f)));
        ON_CALL(container, get_transform())
            .WillByDefault(::testing::Return(glm::mat4(1.f)));
        ON_CALL(container, is_focused())
            .WillByDefault(::testing::Return(true));

        render_data_manager.add(container);
    }

    auto result = render_data_manager.get();
    ASSERT_EQ(result.size(), value);
}

INSTANTIATE_TEST_SUITE_P(
    RenderDataManagerParameterizedTest,
    RenderDataManagerParameterizedTest,
    ::testing::Values(1, 2, 8, 64, 128, 256, 512, 1024));
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

#include "compositor_state.h"
#include "config.h"
#include "leaf_container.h"
#include "mock_output_factory.h"
#include "mock_parent_container.h"
#include "mock_window_controller.h"
#include "mock_workspace.h"
#include "output_manager.h"
#include "stub_configuration.h"
#include <gtest/gtest.h>
#include <memory>

using namespace miracle;

class LeafContainerTest : public ::testing::Test
{
public:
    LeafContainerTest() :
        config(std::make_shared<test::StubConfiguration>()),
        workspace(std::make_unique<test::MockWorkspace>()),
        parent(std::make_shared<test::MockParentContainer>(
            state,
            output_manager.get(),
            window_controller,
            config,
            geom::Rectangle {
                { 0,   0   },
                { 800, 600 }
    },
            workspace.get(),
            nullptr,
            true)),
        leaf_container(std::make_shared<LeafContainer>(
            workspace.get(),
            window_controller,
            geom::Rectangle { { 0, 0 }, { 400, 300 } },
            config,
            parent,
            state,
            output_manager.get()))
    {
        state.add(leaf_container);
    }

protected:
    test::MockWindowController window_controller;
    std::shared_ptr<Config> config;
    CompositorState state;
    std::unique_ptr<OutputManager> output_manager = std::make_unique<OutputManager>(std::make_unique<test::MockOutputFactory>());
    std::unique_ptr<test::MockWorkspace> workspace;
    std::shared_ptr<test::MockParentContainer> parent;
    std::shared_ptr<LeafContainer> leaf_container;
};

TEST_F(LeafContainerTest, InitializesWithCorrectLogicalArea)
{
    auto area = leaf_container->get_logical_area();
    ASSERT_EQ(area.size.width.as_int(), 400);
    ASSERT_EQ(area.size.height.as_int(), 300);
}

TEST_F(LeafContainerTest, SetsAndGetsParentCorrectly)
{
    ASSERT_EQ(leaf_container->get_parent().lock(), parent);
}

TEST_F(LeafContainerTest, SetsAndGetsLogicalAreaCorrectly)
{
    geom::Rectangle new_area {
        { 10,  10  },
        { 200, 200 }
    };
    leaf_container->set_logical_area(new_area);
    ASSERT_EQ(leaf_container->get_logical_area(), new_area);
}

TEST_F(LeafContainerTest, SetsAndGetsStateCorrectly)
{
    EXPECT_CALL(window_controller, change_state(::testing::_, MirWindowState::mir_window_state_fullscreen))
        .Times(1);
    leaf_container->set_state(MirWindowState::mir_window_state_fullscreen);
    leaf_container->commit_changes();
}

TEST_F(LeafContainerTest, SetsAndGetsTreeCorrectly)
{
    auto new_workspace = std::make_unique<test::MockWorkspace>();
    leaf_container->set_workspace(new_workspace.get());
    ASSERT_EQ(leaf_container->get_workspace(), new_workspace.get());
}

TEST_F(LeafContainerTest, CorrectlyReportsIfFocused)
{
    state.focus_container(leaf_container);
    ASSERT_TRUE(leaf_container->is_focused());
}

TEST_F(LeafContainerTest, CorrectlyReportsIfNotFocused)
{
    state.focus_container(nullptr);
    ASSERT_FALSE(leaf_container->is_focused());
}

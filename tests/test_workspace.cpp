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
#include "leaf_container.h"
#include "mock_output.h"
#include "mock_output_factory.h"
#include "output_manager.h"
#include "parent_container.h"
#include "stub_configuration.h"
#include "stub_session.h"
#include "stub_surface.h"
#include "stub_window_controller.h"
#include "window_controller.h"
#include "workspace.h"
#include <gtest/gtest.h>

using namespace miracle;

namespace
{
const float OUTPUT_WIDTH = 1280;
const float OUTPUT_HEIGHT = 720;

const geom::Rectangle OUTPUT_SIZE {
    geom::Point(0, 0),
    geom::Size(OUTPUT_WIDTH, OUTPUT_HEIGHT)
};

const geom::Rectangle OTHER_OUTPUT_SIZE {
    geom::Point(OUTPUT_WIDTH, OUTPUT_HEIGHT),
    geom::Size(OUTPUT_WIDTH, OUTPUT_HEIGHT)
};

std::vector<std::shared_ptr<WorkspaceInterface>> empty_workspaces;
std::vector<miral::Zone> empty_app_zones;

std::unique_ptr<test::MockOutput> create_output(geom::Rectangle const& bounds)
{
    auto output = std::make_unique<testing::NiceMock<test::MockOutput>>();
    ON_CALL(*output, get_area())
        .WillByDefault(testing::ReturnRef(bounds));
    ON_CALL(*output, get_workspaces())
        .WillByDefault(testing::ReturnRef(empty_workspaces));
    ON_CALL(*output, get_app_zones())
        .WillByDefault(testing::ReturnRef(empty_app_zones));
    return output;
}
}

class WorkspaceTest : public testing::Test
{
public:
    WorkspaceTest() :
        state(std::make_shared<CompositorState>()),
        output(create_output(OUTPUT_SIZE)),
        window_controller(std::make_shared<StubWindowController>(pairs)),
        workspace(
            output.get(),
            0,
            0,
            "0",
            std::make_shared<test::StubConfiguration>(),
            window_controller,
            state)
    {
    }

    std::shared_ptr<LeafContainer> create_leaf(
        std::optional<std::shared_ptr<ParentContainer>> parent = std::nullopt,
        WorkspaceInterface* target_workspace = nullptr)
    {
        if (target_workspace == nullptr)
            target_workspace = &workspace;
        miral::WindowSpecification spec;
        miral::ApplicationInfo app_info;
        auto hint = target_workspace->allocate_position(app_info, spec, { ContainerType::leaf, parent });

        auto session = std::make_shared<test::StubSession>();
        sessions.push_back(session);
        auto surface = std::make_shared<test::StubSurface>();
        surfaces.push_back(surface);

        miral::Window window(session, surface);
        miral::WindowInfo info(window, spec);
        auto leaf = target_workspace->create_container(info, hint);
        pairs.push_back({ window, leaf });

        state->add(leaf);
        leaf->on_focus_gained();
        state->focus_container(leaf);
        return Container::as_leaf(leaf);
    }

    std::shared_ptr<CompositorState> state;
    std::vector<std::shared_ptr<test::StubSession>> sessions;
    std::vector<std::shared_ptr<test::StubSurface>> surfaces;
    std::vector<StubWindowData> pairs;
    std::shared_ptr<StubWindowController> window_controller;
    std::unique_ptr<test::MockOutput> output;
    Workspace workspace;
};

TEST_F(WorkspaceTest, can_add_single_window_without_border_and_gaps)
{
    auto leaf = create_leaf();
    ASSERT_EQ(leaf->get_logical_area().size, geom::Size(OUTPUT_WIDTH, OUTPUT_HEIGHT));
    ASSERT_EQ(leaf->get_logical_area().top_left, geom::Point(0, 0));
}

TEST_F(WorkspaceTest, can_add_two_windows_horizontally_without_border_and_gaps)
{
    auto leaf1 = create_leaf();
    auto leaf2 = create_leaf();

    ASSERT_EQ(leaf1->get_logical_area().size, geom::Size(OUTPUT_WIDTH / 2.f, OUTPUT_HEIGHT));
    ASSERT_EQ(leaf1->get_logical_area().top_left, geom::Point(0, 0));

    ASSERT_EQ(leaf2->get_logical_area().size, geom::Size(OUTPUT_WIDTH / 2.f, OUTPUT_HEIGHT));
    ASSERT_EQ(leaf2->get_logical_area().top_left, geom::Point(OUTPUT_WIDTH / 2.f, 0));
}

TEST_F(WorkspaceTest, can_add_two_windows_vertically_without_border_and_gaps)
{
    auto leaf1 = create_leaf();
    leaf1->request_vertical_layout();

    auto leaf2 = create_leaf();
    ASSERT_EQ(leaf1->get_logical_area().size, geom::Size(OUTPUT_WIDTH, OUTPUT_HEIGHT / 2.f));
    ASSERT_EQ(leaf1->get_logical_area().top_left, geom::Point(0, 0));

    ASSERT_EQ(leaf2->get_logical_area().size, geom::Size(OUTPUT_WIDTH, OUTPUT_HEIGHT / 2.f));
    ASSERT_EQ(leaf2->get_logical_area().top_left, geom::Point(0, OUTPUT_HEIGHT / 2.f));
}

TEST_F(WorkspaceTest, can_add_three_windows_horizontally_without_border_and_gaps)
{
    auto leaf1 = create_leaf();
    auto leaf2 = create_leaf();
    auto leaf3 = create_leaf();

    ASSERT_EQ(leaf1->get_logical_area().size, geom::Size(ceilf(OUTPUT_WIDTH / 3.f), OUTPUT_HEIGHT));
    ASSERT_EQ(leaf1->get_logical_area().top_left, geom::Point(0, 0));

    ASSERT_EQ(leaf2->get_logical_area().size, geom::Size(ceilf(OUTPUT_WIDTH / 3.f), OUTPUT_HEIGHT));
    ASSERT_EQ(leaf2->get_logical_area().top_left, geom::Point(ceilf(OUTPUT_WIDTH / 3.f), 0));

    ASSERT_EQ(leaf3->get_logical_area().size, geom::Size(floorf(OUTPUT_WIDTH / 3.f), OUTPUT_HEIGHT));
    ASSERT_EQ(leaf3->get_logical_area().top_left, geom::Point(floorf(OUTPUT_WIDTH * (2.f / 3.f)) - 1, 0));
}

TEST_F(WorkspaceTest, can_start_dragging_a_leaf)
{
    auto leaf1 = create_leaf();
    ASSERT_TRUE(leaf1->drag_start());
}

TEST_F(WorkspaceTest, can_drag_a_leaf_to_a_position)
{
    auto leaf1 = create_leaf();
    leaf1->drag_start();
    leaf1->drag(50, 50);
    auto const& data = window_controller->get_window_data(leaf1);
    ASSERT_EQ(data.rectangle.top_left.x.as_int(), 50);
    ASSERT_EQ(data.rectangle.top_left.y.as_int(), 50);
}

TEST_F(WorkspaceTest, can_stop_dragging_a_leaf)
{
    auto leaf1 = create_leaf();
    leaf1->drag_start();
    leaf1->drag(50, 50);
    leaf1->drag_stop();
    auto const& data = window_controller->get_window_data(leaf1);
    ASSERT_EQ(data.rectangle.top_left.x.as_int(), 0);
    ASSERT_EQ(data.rectangle.top_left.y.as_int(), 0);
}

TEST_F(WorkspaceTest, can_move_container_to_sibling)
{
    auto leaf1 = create_leaf();
    auto leaf2 = create_leaf();

    ASSERT_TRUE(leaf1->move_to(*leaf2));

    // Assert that leaf2 is in the first position
    ASSERT_EQ(leaf2->get_logical_area().top_left, geom::Point(0, 0));
    ASSERT_EQ(leaf1->get_logical_area().top_left, geom::Point(OUTPUT_WIDTH / 2.f, 0));
}

TEST_F(WorkspaceTest, can_move_container_to_different_parent)
{
    auto leaf1 = create_leaf();
    auto leaf2 = create_leaf();
    leaf2->request_vertical_layout();
    auto leaf3 = create_leaf(leaf2->get_parent().lock());

    ASSERT_TRUE(leaf1->move_to(*leaf3));

    ASSERT_EQ(leaf2->get_logical_area().top_left, geom::Point(0, 0));
    ASSERT_EQ(leaf3->get_logical_area().top_left, geom::Point(0, ceilf(OUTPUT_HEIGHT / 3.f)));
    ASSERT_EQ(leaf1->get_logical_area().top_left, geom::Point(0, ceilf(OUTPUT_HEIGHT * (2.f / 3.f))));
    ASSERT_EQ(workspace.get_root()->num_nodes(), 3);
}

TEST_F(WorkspaceTest, can_move_container_to_container_in_other_tree)
{
    auto other_output = create_output(OTHER_OUTPUT_SIZE);
    Workspace other(
        other_output.get(),
        1,
        1,
        "1",
        std::make_shared<test::StubConfiguration>(),
        window_controller,
        state);
    auto leaf1 = create_leaf();
    auto leaf2 = create_leaf(std::nullopt, &other);

    ASSERT_EQ(leaf1->get_workspace(), &workspace);
    ASSERT_EQ(leaf2->get_workspace(), &other);

    ASSERT_TRUE(leaf1->move_to(*leaf2));

    ASSERT_EQ(leaf2->get_workspace(), &other);
}

TEST_F(WorkspaceTest, can_move_container_to_tree)
{
    auto other_output = create_output(OTHER_OUTPUT_SIZE);
    Workspace other(
        other_output.get(),
        1,
        1,
        "1",
        std::make_shared<test::StubConfiguration>(),
        window_controller,
        state);
    auto leaf1 = create_leaf();

    ASSERT_EQ(leaf1->get_workspace(), &workspace);
    ASSERT_TRUE(other.add_to_root(*leaf1));
    ASSERT_EQ(leaf1->get_workspace(), &other);
    ASSERT_EQ(leaf1->get_logical_area(), OTHER_OUTPUT_SIZE);
}

TEST_F(WorkspaceTest, dragged_windows_do_not_change_their_position_when_a_new_window_is_added)
{
    auto leaf1 = create_leaf();
    leaf1->drag_start();
    leaf1->drag(100, 100);

    auto leaf2 = create_leaf();
    ASSERT_EQ(window_controller->get_window_data(leaf1).rectangle.top_left, mir::geometry::Point(100, 100));
    ASSERT_EQ(window_controller->get_window_data(leaf1).rectangle.size, geom::Size(OUTPUT_WIDTH / 2.f, OUTPUT_HEIGHT));
}

TEST_F(WorkspaceTest, dragged_windows_are_unconstrained)
{
    auto leaf1 = create_leaf();
    leaf1->drag_start();
    ASSERT_EQ(window_controller->get_window_data(leaf1).clip, std::nullopt);
    leaf1->drag(100, 100);
    leaf1->drag_stop();
    ASSERT_EQ(window_controller->get_window_data(leaf1).clip, leaf1->get_visible_area());
}

TEST_F(WorkspaceTest, workspace_bounds_are_initialized_to_output_size_when_no_app_zones_are_present)
{
    // Assert that the first tree (w/o app zones) is equal to the output size.
    ASSERT_EQ(workspace.get_root()->get_logical_area(), OUTPUT_SIZE);
}

TEST_F(WorkspaceTest, workspace_bounds_are_initialized_to_first_zone_size_when_app_zones_are_present)
{
    auto output = std::make_unique<testing::NiceMock<test::MockOutput>>();
    ON_CALL(*output, get_area())
        .WillByDefault(testing::ReturnRef(OTHER_OUTPUT_SIZE));
    ON_CALL(*output, get_workspaces())
        .WillByDefault(testing::ReturnRef(empty_workspaces));

    mir::geometry::Rectangle const zone_bounds(
        mir::geometry::Point(100, 100),
        mir::geometry::Size(500, 500));
    std::vector<miral::Zone> zones = { miral::Zone(zone_bounds) };
    ON_CALL(*output, get_app_zones())
        .WillByDefault(testing::ReturnRef(zones));
    Workspace other(
        output.get(),
        1,
        1,
        "1",
        std::make_shared<test::StubConfiguration>(),
        window_controller,
        state);

    // Assert that the first tree (w/o app zones) is equal to the output size.
    ASSERT_EQ(other.get_root()->get_logical_area(), zone_bounds);
}
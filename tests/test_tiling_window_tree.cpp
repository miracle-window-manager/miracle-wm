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
#include "output_manager.h"
#include "parent_container.h"
#include "single_mock_output_factory.h"
#include "stub_configuration.h"
#include "stub_session.h"
#include "stub_surface.h"
#include "stub_window_controller.h"
#include "tiling_window_tree.h"
#include "window_controller.h"
#include <gtest/gtest.h>
#include <miral/window_management_options.h>

using namespace miracle;

namespace
{
const float OUTPUT_WIDTH = 1280;
const float OUTPUT_HEIGHT = 720;

const geom::Rectangle TREE_BOUNDS {
    geom::Point(0, 0),
    geom::Size(OUTPUT_WIDTH, OUTPUT_HEIGHT)
};

const geom::Rectangle OTHER_TREE_BOUNDS {
    geom::Point(OUTPUT_WIDTH, OUTPUT_HEIGHT),
    geom::Size(OUTPUT_WIDTH, OUTPUT_HEIGHT)
};
}

class SimpleTilingWindowTreeInterface : public TilingWindowTreeInterface
{
public:
    explicit SimpleTilingWindowTreeInterface(geom::Rectangle const& bounds)
    {
        zones = { bounds };
    }

    std::vector<miral::Zone> const& get_zones() override
    {
        return zones;
    }

    Workspace* get_workspace() const override
    {
        return nullptr;
    }

private:
    std::vector<miral::Zone> zones;
};

class TilingWindowTreeTest : public testing::Test
{
public:
    TilingWindowTreeTest() :
        tree(
            std::make_unique<SimpleTilingWindowTreeInterface>(TREE_BOUNDS),
            window_controller,
            state,
            new OutputManager(std::make_unique<test::SingleMockOutputFactory>()),
            std::make_shared<test::StubConfiguration>(),
            TREE_BOUNDS)
    {
    }

    std::shared_ptr<LeafContainer> create_leaf(
        std::shared_ptr<ParentContainer> parent = nullptr,
        MiralTilingWindowTree* target_tree = nullptr)
    {
        if (target_tree == nullptr)
            target_tree = &tree;
        miral::WindowSpecification spec;
        spec = target_tree->place_new_window(spec, parent);

        auto session = std::make_shared<test::StubSession>();
        sessions.push_back(session);
        auto surface = std::make_shared<test::StubSurface>();
        surfaces.push_back(surface);

        miral::Window window(session, surface);
        miral::WindowInfo info(window, spec);

        auto leaf = target_tree->confirm_window(info, parent);
        pairs.push_back({ window, leaf });

        state.add(leaf);
        target_tree->advise_focus_gained(*leaf);
        state.focus_container(leaf);
        return leaf;
    }

    CompositorState state;
    std::vector<std::shared_ptr<test::StubSession>> sessions;
    std::vector<std::shared_ptr<test::StubSurface>> surfaces;
    std::vector<StubWindowData> pairs;
    StubWindowController window_controller { pairs };
    MiralTilingWindowTree tree;
};

TEST_F(TilingWindowTreeTest, can_add_single_window_without_border_and_gaps)
{
    auto leaf = create_leaf();
    ASSERT_EQ(leaf->get_logical_area().size, geom::Size(OUTPUT_WIDTH, OUTPUT_HEIGHT));
    ASSERT_EQ(leaf->get_logical_area().top_left, geom::Point(0, 0));
}

TEST_F(TilingWindowTreeTest, can_add_two_windows_horizontally_without_border_and_gaps)
{
    auto leaf1 = create_leaf();
    auto leaf2 = create_leaf();

    ASSERT_EQ(leaf1->get_logical_area().size, geom::Size(OUTPUT_WIDTH / 2.f, OUTPUT_HEIGHT));
    ASSERT_EQ(leaf1->get_logical_area().top_left, geom::Point(0, 0));

    ASSERT_EQ(leaf2->get_logical_area().size, geom::Size(OUTPUT_WIDTH / 2.f, OUTPUT_HEIGHT));
    ASSERT_EQ(leaf2->get_logical_area().top_left, geom::Point(OUTPUT_WIDTH / 2.f, 0));
}

TEST_F(TilingWindowTreeTest, can_add_two_windows_vertically_without_border_and_gaps)
{
    auto leaf1 = create_leaf();

    tree.request_vertical_layout(*leaf1);

    auto leaf2 = create_leaf();
    ASSERT_EQ(leaf1->get_logical_area().size, geom::Size(OUTPUT_WIDTH, OUTPUT_HEIGHT / 2.f));
    ASSERT_EQ(leaf1->get_logical_area().top_left, geom::Point(0, 0));

    ASSERT_EQ(leaf2->get_logical_area().size, geom::Size(OUTPUT_WIDTH, OUTPUT_HEIGHT / 2.f));
    ASSERT_EQ(leaf2->get_logical_area().top_left, geom::Point(0, OUTPUT_HEIGHT / 2.f));
}

TEST_F(TilingWindowTreeTest, can_add_three_windows_horizontally_without_border_and_gaps)
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

TEST_F(TilingWindowTreeTest, can_start_dragging_a_leaf)
{
    auto leaf1 = create_leaf();
    ASSERT_TRUE(leaf1->drag_start());
}

TEST_F(TilingWindowTreeTest, can_drag_a_leaf_to_a_position)
{
    auto leaf1 = create_leaf();
    leaf1->drag_start();
    leaf1->drag(50, 50);
    auto const& data = window_controller.get_window_data(leaf1);
    ASSERT_EQ(data.rectangle.top_left.x.as_int(), 50);
    ASSERT_EQ(data.rectangle.top_left.y.as_int(), 50);
}

TEST_F(TilingWindowTreeTest, can_stop_dragging_a_leaf)
{
    auto leaf1 = create_leaf();
    leaf1->drag_start();
    leaf1->drag(50, 50);
    leaf1->drag_stop();
    auto const& data = window_controller.get_window_data(leaf1);
    ASSERT_EQ(data.rectangle.top_left.x.as_int(), 0);
    ASSERT_EQ(data.rectangle.top_left.y.as_int(), 0);
}

TEST_F(TilingWindowTreeTest, can_move_container_to_sibling)
{
    auto leaf1 = create_leaf();
    auto leaf2 = create_leaf();

    ASSERT_TRUE(tree.move_to(*leaf1, *leaf2));

    // Assert that leaf2 is in the first position
    ASSERT_EQ(leaf2->get_logical_area().top_left, geom::Point(0, 0));
    ASSERT_EQ(leaf1->get_logical_area().top_left, geom::Point(OUTPUT_WIDTH / 2.f, 0));
}

TEST_F(TilingWindowTreeTest, can_move_container_to_different_parent)
{
    auto leaf1 = create_leaf();
    auto leaf2 = create_leaf();
    tree.request_vertical_layout(*leaf2);
    auto leaf3 = create_leaf(leaf2->get_parent().lock());

    ASSERT_TRUE(tree.move_to(*leaf1, *leaf3));

    ASSERT_EQ(leaf2->get_logical_area().top_left, geom::Point(0, 0));
    ASSERT_EQ(leaf3->get_logical_area().top_left, geom::Point(0, ceilf(OUTPUT_HEIGHT / 3.f)));
    ASSERT_EQ(leaf1->get_logical_area().top_left, geom::Point(0, ceilf(OUTPUT_HEIGHT * (2.f / 3.f))));
    ASSERT_EQ(tree.get_root()->num_nodes(), 3);
}

TEST_F(TilingWindowTreeTest, can_move_container_to_container_in_other_tree)
{
    MiralTilingWindowTree other_tree(
        std::make_unique<SimpleTilingWindowTreeInterface>(OTHER_TREE_BOUNDS),
        window_controller,
        state,
        new OutputManager(std::make_unique<test::SingleMockOutputFactory>()),
        std::make_shared<test::StubConfiguration>(),
        OTHER_TREE_BOUNDS);
    auto leaf1 = create_leaf();
    auto leaf2 = create_leaf(nullptr, &other_tree);

    ASSERT_EQ(leaf1->tree(), &tree);
    ASSERT_EQ(leaf2->tree(), &other_tree);

    ASSERT_TRUE(tree.move_to(*leaf1, *leaf2));

    ASSERT_EQ(leaf2->tree(), &other_tree);
}

TEST_F(TilingWindowTreeTest, can_move_container_to_tree)
{
    MiralTilingWindowTree other_tree(
        std::make_unique<SimpleTilingWindowTreeInterface>(OTHER_TREE_BOUNDS),
        window_controller,
        state,
        new OutputManager(std::make_unique<test::SingleMockOutputFactory>()),
        std::make_shared<test::StubConfiguration>(),
        OTHER_TREE_BOUNDS);
    auto leaf1 = create_leaf();

    ASSERT_EQ(leaf1->tree(), &tree);
    ASSERT_TRUE(other_tree.move_to_tree(leaf1));
    ASSERT_EQ(leaf1->tree(), &other_tree);
    ASSERT_EQ(leaf1->get_logical_area(), OTHER_TREE_BOUNDS);
}

TEST_F(TilingWindowTreeTest, dragged_windows_do_not_change_their_position_when_a_new_window_is_added)
{
    auto leaf1 = create_leaf();
    leaf1->drag_start();
    leaf1->drag(100, 100);

    auto leaf2 = create_leaf();
    ASSERT_EQ(window_controller.get_window_data(leaf1).rectangle.top_left, mir::geometry::Point(100, 100));
    ASSERT_EQ(window_controller.get_window_data(leaf1).rectangle.size, geom::Size(OUTPUT_WIDTH / 2.f, OUTPUT_HEIGHT));
}

TEST_F(TilingWindowTreeTest, dragged_windows_are_unconstrained)
{
    auto leaf1 = create_leaf();
    leaf1->drag_start();
    ASSERT_EQ(window_controller.get_window_data(leaf1).clip, std::nullopt);
    leaf1->drag(100, 100);
    leaf1->drag_stop();
    ASSERT_EQ(window_controller.get_window_data(leaf1).clip, leaf1->get_visible_area());
}

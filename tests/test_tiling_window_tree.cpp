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
geom::Rectangle r {
    geom::Point(0, 0),
    geom::Size(1280, 720)
};
}

class SimpleTilingWindowTreeInterface : public TilingWindowTreeInterface
{
public:
    std::vector<miral::Zone> const& get_zones() override
    {
        return zones;
    }

    Workspace* get_workspace() const override
    {
        return nullptr;
    }

private:
    std::vector<miral::Zone> zones = { r };
};

class TilingWindowTreeTest : public testing::Test
{
public:
    TilingWindowTreeTest() :
        tree(
            std::make_unique<SimpleTilingWindowTreeInterface>(),
            window_controller,
            state,
            std::make_shared<test::StubConfiguration>(),
            r)
    {
    }

    std::shared_ptr<LeafContainer> create_leaf()
    {
        miral::WindowSpecification spec;
        spec = tree.place_new_window(spec, nullptr);

        auto session = std::make_shared<test::StubSession>();
        sessions.push_back(session);
        auto surface = std::make_shared<test::StubSurface>();
        surfaces.push_back(surface);

        miral::Window window(session, surface);
        miral::WindowInfo info(window, spec);

        auto leaf = tree.confirm_window(info, nullptr);
        pairs.push_back({ window, leaf });

        state.add(leaf);
        tree.advise_focus_gained(*leaf);
        state.focus_container(leaf);
        return leaf;
    }

    CompositorState state;
    std::vector<std::shared_ptr<test::StubSession>> sessions;
    std::vector<std::shared_ptr<test::StubSurface>> surfaces;
    std::vector<StubWindowData> pairs;
    StubWindowController window_controller { pairs };
    TilingWindowTree tree;
};

TEST_F(TilingWindowTreeTest, can_add_single_window_without_border_and_gaps)
{
    auto leaf = create_leaf();
    ASSERT_EQ(leaf->get_logical_area().size, geom::Size(1280, 720));
    ASSERT_EQ(leaf->get_logical_area().top_left, geom::Point(0, 0));
}

TEST_F(TilingWindowTreeTest, can_add_two_windows_horizontally_without_border_and_gaps)
{
    auto leaf1 = create_leaf();
    auto leaf2 = create_leaf();

    ASSERT_EQ(leaf1->get_logical_area().size, geom::Size(1280 / 2.f, 720));
    ASSERT_EQ(leaf1->get_logical_area().top_left, geom::Point(0, 0));

    ASSERT_EQ(leaf2->get_logical_area().size, geom::Size(1280 / 2.f, 720));
    ASSERT_EQ(leaf2->get_logical_area().top_left, geom::Point(1280 / 2.f, 0));
}

TEST_F(TilingWindowTreeTest, can_add_two_windows_vertically_without_border_and_gaps)
{
    auto leaf1 = create_leaf();

    tree.request_vertical_layout(*leaf1);

    auto leaf2 = create_leaf();
    ASSERT_EQ(leaf1->get_logical_area().size, geom::Size(1280, 720 / 2.f));
    ASSERT_EQ(leaf1->get_logical_area().top_left, geom::Point(0, 0));

    ASSERT_EQ(leaf2->get_logical_area().size, geom::Size(1280, 720 / 2.f));
    ASSERT_EQ(leaf2->get_logical_area().top_left, geom::Point(0, 720 / 2.f));
}

TEST_F(TilingWindowTreeTest, can_add_three_windows_horizontally_without_border_and_gaps)
{
    auto leaf1 = create_leaf();
    auto leaf2 = create_leaf();
    auto leaf3 = create_leaf();

    ASSERT_EQ(leaf1->get_logical_area().size, geom::Size(ceilf(1280 / 3.f), 720));
    ASSERT_EQ(leaf1->get_logical_area().top_left, geom::Point(0, 0));

    ASSERT_EQ(leaf2->get_logical_area().size, geom::Size(ceilf(1280 / 3.f), 720));
    ASSERT_EQ(leaf2->get_logical_area().top_left, geom::Point(ceilf(1280 / 3.f), 0));

    ASSERT_EQ(leaf3->get_logical_area().size, geom::Size(floorf(1280 / 3.f), 720));
    ASSERT_EQ(leaf3->get_logical_area().top_left, geom::Point(floorf(1280 * (2.f / 3.f)) - 1, 0));
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
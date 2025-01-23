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

#ifndef MIRACLE_WM_MOCK_TILING_WINDOW_TREE_H
#define MIRACLE_WM_MOCK_TILING_WINDOW_TREE_H

#include "tiling_window_tree.h"
#include <functional>
#include <gmock/gmock.h>
#include <memory>
#include <optional>

namespace miracle
{
namespace test
{

    class MockTilingWindowTree : public TilingWindowTree
    {
    public:
        MOCK_METHOD(miral::WindowSpecification, place_new_window,
            (const miral::WindowSpecification& requested_specification,
                std::shared_ptr<ParentContainer> const& container),
            (override));

        MOCK_METHOD(std::shared_ptr<LeafContainer>, confirm_window,
            (miral::WindowInfo const&,
                std::shared_ptr<ParentContainer> const& container),
            (override));

        MOCK_METHOD(void, graft,
            (std::shared_ptr<Container> const&,
                std::shared_ptr<ParentContainer> const& parent,
                int index),
            (override));

        MOCK_METHOD(bool, resize_container,
            (Direction direction, int pixels, Container&), (override));

        MOCK_METHOD(bool, set_size,
            (std::optional<int> const& width,
                std::optional<int> const& height,
                Container&),
            (override));

        MOCK_METHOD(bool, move_container,
            (Direction direction, Container&), (override));

        MOCK_METHOD(bool, move_to,
            (Container & to_move, Container& target), (override));

        MOCK_METHOD(bool, move_to_tree,
            (std::shared_ptr<Container> const& container), (override));

        MOCK_METHOD(bool, select_next,
            (Direction direction, Container&), (override));

        MOCK_METHOD(bool, toggle_fullscreen,
            (LeafContainer&), (override));

        MOCK_METHOD(void, request_layout,
            (Container&, LayoutScheme), (override));

        MOCK_METHOD(void, request_vertical_layout,
            (Container&), (override));

        MOCK_METHOD(void, request_horizontal_layout,
            (Container&), (override));

        MOCK_METHOD(void, request_tabbing_layout,
            (Container&), (override));

        MOCK_METHOD(void, request_stacking_layout,
            (Container&), (override));

        MOCK_METHOD(void, toggle_layout,
            (Container&, bool cycle_thru_all), (override));

        MOCK_METHOD(void, advise_focus_gained,
            (LeafContainer&), (override));

        MOCK_METHOD(void, advise_delete_window,
            (std::shared_ptr<Container> const&), (override));

        MOCK_METHOD(void, set_area,
            (geom::Rectangle const& new_area), (override));

        MOCK_METHOD(geom::Rectangle, get_area, (), (const, override));

        MOCK_METHOD(bool, advise_fullscreen_container,
            (LeafContainer&), (override));

        MOCK_METHOD(bool, advise_restored_container,
            (LeafContainer&), (override));

        MOCK_METHOD(bool, handle_container_ready,
            (LeafContainer&), (override));

        MOCK_METHOD(bool, confirm_placement_on_display,
            (Container & container, MirWindowState new_state,
                mir::geometry::Rectangle& new_placement),
            (override));

        MOCK_METHOD(void, foreach_node,
            (std::function<void(std::shared_ptr<Container> const&)> const&), (const, override));

        MOCK_METHOD(bool, foreach_node_pred,
            (std::function<bool(std::shared_ptr<Container> const&)> const&), (const, override));

        MOCK_METHOD(std::shared_ptr<LeafContainer>, show, (), (override));

        MOCK_METHOD(void, hide, (), (override));

        MOCK_METHOD(void, recalculate_root_node_area, (), (override));

        MOCK_METHOD(bool, is_empty, (), (const, override));

        MOCK_METHOD(Workspace*, get_workspace, (), (const, override));

        MOCK_METHOD(std::shared_ptr<ParentContainer> const&, get_root, (), (const, override));
    };
}
}

#endif // MIRACLE_WM_MOCK_TILING_WINDOW_TREE_H

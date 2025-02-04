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

#ifndef MIRACLE_WM_MOCK_WORKSPACE_H
#define MIRACLE_WM_MOCK_WORKSPACE_H

#include "workspace_interface.h"
#include <functional>
#include <gmock/gmock.h>
#include <memory>
#include <optional>
#include <string>

namespace miracle
{
namespace test
{
    class MockWorkspace : public WorkspaceInterface
    {
    public:
        MOCK_METHOD(void, set_area, (mir::geometry::Rectangle const&), (override));
        MOCK_METHOD(void, recalculate_area, (), (override));

        MOCK_METHOD(AllocationHint, allocate_position,
            (miral::ApplicationInfo const& app_info,
                miral::WindowSpecification& requested_specification,
                AllocationHint const& hint),
            (override));

        MOCK_METHOD(std::shared_ptr<Container>, create_container,
            (miral::WindowInfo const& window_info, AllocationHint const& type), (override));

        MOCK_METHOD(void, delete_container, (std::shared_ptr<Container> const& container), (override));
        MOCK_METHOD(bool, move_container, (Direction direction, Container&), (override));
        MOCK_METHOD(void, show, (), (override));
        MOCK_METHOD(void, hide, (), (override));
        MOCK_METHOD(bool, add_to_root, (Container&), (override));
        MOCK_METHOD(std::shared_ptr<ParentContainer>, create_floating_tree, (mir::geometry::Rectangle const&), (override));

        MOCK_METHOD(void, transfer_pinned_windows_to, (std::shared_ptr<WorkspaceInterface> const& other), (override));

        MOCK_METHOD(bool, for_each_window,
            (std::function<bool(std::shared_ptr<Container>)> const&), (const, override));

        MOCK_METHOD(void, advise_focus_gained, (std::shared_ptr<Container> const& container), (override));

        MOCK_METHOD(void, select_first_window, (), (override));

        MOCK_METHOD(OutputInterface*, get_output, (), (const, override));

        MOCK_METHOD(void, set_output, (OutputInterface*), (override));

        MOCK_METHOD(void, workspace_transform_change_hack, (), (override));

        MOCK_METHOD(bool, is_empty, (), (const, override));
        MOCK_METHOD(void, graft, (std::shared_ptr<Container> const&), (override));

        MOCK_METHOD(uint32_t, id, (), (const, override));
        MOCK_METHOD(std::optional<int>, num, (), (const, override));
        MOCK_METHOD(nlohmann::json, to_json, (bool), (const, override));
        MOCK_METHOD(std::optional<std::string> const&, name, (), (const, override));
        MOCK_METHOD(std::string, display_name, (), (const, override));
        MOCK_METHOD(std::shared_ptr<ParentContainer>, get_root, (), (const, override));
    };
}
}

#endif // MIRACLE_WM_MOCK_WORKSPACE_H

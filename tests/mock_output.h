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

#ifndef MIRACLE_WM_MOCK_OUTPUT_H
#define MIRACLE_WM_MOCK_OUTPUT_H

#include "output.h"
#include <gmock/gmock.h>
#include <miral/zone.h>

namespace miracle
{
namespace test
{
    class MockOutput : public Output
    {
    public:
        MOCK_METHOD(std::shared_ptr<Container>, intersect, (float x, float y), (override));
        MOCK_METHOD(std::shared_ptr<Container>, intersect_leaf, (float x, float y, bool ignore_selected), (override));
        MOCK_METHOD(AllocationHint, allocate_position,
            (miral::ApplicationInfo const& app_info,
                miral::WindowSpecification& requested_specification,
                AllocationHint hint),
            (override));
        MOCK_METHOD(std::shared_ptr<Container>, create_container,
            (miral::WindowInfo const& window_info, AllocationHint const& hint),
            (const, override));
        MOCK_METHOD(void, delete_container, (std::shared_ptr<Container> const& container), (override));
        MOCK_METHOD(void, advise_new_workspace, (WorkspaceCreationData const&&), (override));
        MOCK_METHOD(void, advise_workspace_deleted, (WorkspaceManager&, uint32_t id), (override));
        MOCK_METHOD(bool, advise_workspace_active, (WorkspaceManager&, uint32_t id), (override));
        MOCK_METHOD(void, advise_application_zone_create, (miral::Zone const& application_zone), (override));
        MOCK_METHOD(void, advise_application_zone_update,
            (miral::Zone const& updated, miral::Zone const& original),
            (override));
        MOCK_METHOD(void, move_workspace_to, (WorkspaceManager&, Workspace*), (override));
        MOCK_METHOD(void, advise_application_zone_delete, (miral::Zone const& application_zone), (override));
        MOCK_METHOD(bool, point_is_in_output, (int x, int y), (override));
        MOCK_METHOD(void, update_area, (geom::Rectangle const& area), (override));
        MOCK_METHOD(void, add_immediately,
            (miral::Window & window, AllocationHint hint),
            (override));
        MOCK_METHOD(void, graft, (std::shared_ptr<Container> const& container), (override));
        MOCK_METHOD(void, set_transform, (glm::mat4 const& in), (override));
        MOCK_METHOD(void, set_position, (glm::vec2 const&), (override));
        MOCK_METHOD(std::vector<miral::Window>, collect_all_windows, (), (const, override));
        MOCK_METHOD(Workspace*, active, (), (const, override));
        MOCK_METHOD(std::vector<std::shared_ptr<Workspace>> const&, get_workspaces, (), (const, override));
        MOCK_METHOD(geom::Rectangle const&, get_area, (), (const, override));
        MOCK_METHOD(std::vector<miral::Zone> const&, get_app_zones, (), (const, override));
        MOCK_METHOD(std::string const&, name, (), (const, override));
        MOCK_METHOD(int, id, (), (const, override));
        MOCK_METHOD(glm::mat4, get_transform, (), (const, override));
        MOCK_METHOD(geom::Rectangle, get_workspace_rectangle, (size_t i), (const, override));
        MOCK_METHOD(Workspace const*, workspace, (uint32_t id), (const, override));
        MOCK_METHOD(nlohmann::json, to_json, (), (const, override));
        MOCK_METHOD(void, set_info, (int id, std::string name), (override));
        MOCK_METHOD(void, set_defunct, (), (override));
        MOCK_METHOD(void, unset_defunct, (), (override));
        MOCK_METHOD(bool, is_defunct, (), (const, override));
    };

}
}

#endif // MIRACLE_WM_MOCK_OUTPUT_H

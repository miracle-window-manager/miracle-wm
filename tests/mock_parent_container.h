#ifndef MIRACLE_MOCK_PARENT_CONTAINER_H
#define MIRACLE_MOCK_PARENT_CONTAINER_H

#include "parent_container.h"
#include <gmock/gmock.h>

namespace miracle
{
namespace test
{

    class MockParentContainer : public ParentContainer
    {
    public:
        MockParentContainer(
            WindowController& window_controller,
            geom::Rectangle const& area,
            std::shared_ptr<Config> const& config,
            Workspace* workspace,
            std::shared_ptr<ParentContainer> const& parent,
            CompositorState const& state,
            OutputManager* output_manager) :
            ParentContainer(window_controller, area, config, workspace, parent, state, output_manager)
        {
        }
        MOCK_METHOD(void, show, (), (override));
        MOCK_METHOD(void, hide, (), (override));
        MOCK_METHOD(void, commit_changes, (), (override));
        MOCK_METHOD(geom::Rectangle, get_logical_area, (), (const, override));
        MOCK_METHOD(void, set_logical_area, (geom::Rectangle const&), (override));
        MOCK_METHOD(geom::Rectangle, get_visible_area, (), (const, override));
        MOCK_METHOD(void, constrain, (), (override));
        MOCK_METHOD(std::weak_ptr<ParentContainer>, get_parent, (), (const, override));
        MOCK_METHOD(void, set_parent, (std::shared_ptr<ParentContainer> const&), (override));
        MOCK_METHOD(size_t, get_min_height, (), (const, override));
        MOCK_METHOD(size_t, get_min_width, (), (const, override));
        MOCK_METHOD(void, handle_ready, (), (override));
        MOCK_METHOD(void, handle_modify, (miral::WindowSpecification const&), (override));
        MOCK_METHOD(void, handle_request_move, (MirInputEvent const*), (override));
        MOCK_METHOD(void, handle_request_resize, (MirInputEvent const*, MirResizeEdge), (override));
        MOCK_METHOD(void, handle_raise, (), (override));
        MOCK_METHOD(bool, resize, (Direction, int), (override));
        MOCK_METHOD(bool, set_size, (std::optional<int> const&, std::optional<int> const&), (override));
        MOCK_METHOD(bool, toggle_fullscreen, (), (override));
        MOCK_METHOD(void, request_horizontal_layout, (), (override));
        MOCK_METHOD(void, request_vertical_layout, (), (override));
        MOCK_METHOD(void, toggle_layout, (bool), (override));
        MOCK_METHOD(void, on_open, (), (override));
        MOCK_METHOD(void, on_focus_gained, (), (override));
        MOCK_METHOD(void, on_focus_lost, (), (override));
        MOCK_METHOD(void, on_move_to, (geom::Point const&), (override));
        MOCK_METHOD(geom::Rectangle, confirm_placement, (MirWindowState, geom::Rectangle const&), (override));
        MOCK_METHOD(Workspace*, get_workspace, (), (const, override));
        MOCK_METHOD(Output*, get_output, (), (const, override));
        MOCK_METHOD(glm::mat4, get_transform, (), (const, override));
        MOCK_METHOD(void, set_transform, (glm::mat4), (override));
        MOCK_METHOD(glm::mat4, get_workspace_transform, (), (const, override));
        MOCK_METHOD(glm::mat4, get_output_transform, (), (const, override));
        MOCK_METHOD(uint32_t, animation_handle, (), (const, override));
        MOCK_METHOD(void, animation_handle, (uint32_t), (override));
        MOCK_METHOD(bool, is_focused, (), (const, override));
        MOCK_METHOD(bool, is_fullscreen, (), (const, override));
        MOCK_METHOD(std::optional<miral::Window>, window, (), (const, override));
        MOCK_METHOD(bool, select_next, (Direction), (override));
        MOCK_METHOD(bool, pinned, (), (const, override));
        MOCK_METHOD(bool, pinned, (bool), (override));
        MOCK_METHOD(bool, move, (Direction), (override));
        MOCK_METHOD(bool, move_by, (Direction, int), (override));
        MOCK_METHOD(bool, move_to, (int, int), (override));
        MOCK_METHOD(bool, toggle_tabbing, (), (override));
        MOCK_METHOD(bool, toggle_stacking, (), (override));
        MOCK_METHOD(bool, drag_start, (), (override));
        MOCK_METHOD(void, drag, (int, int), (override));
        MOCK_METHOD(bool, drag_stop, (), (override));
        MOCK_METHOD(bool, set_layout, (LayoutScheme), (override));
        MOCK_METHOD(void, set_workspace, (Workspace*), (override));
        MOCK_METHOD(LayoutScheme, get_layout, (), (const, override));
        MOCK_METHOD(nlohmann::json, to_json, (), (const, override));
    };

} // namespace test
} // namespace miracle

#endif // MIRACLE_MOCK_PARENT_CONTAINER_H

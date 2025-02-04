#ifndef MIRACLE_MOCK_WINDOW_CONTROLLER_H
#define MIRACLE_MOCK_WINDOW_CONTROLLER_H

#include "window_controller.h"
#include <gmock/gmock.h>

namespace miracle
{
namespace test
{

    class MockWindowController : public WindowController
    {
    public:
        MOCK_METHOD(bool, is_fullscreen, (miral::Window const&), (override));
        MOCK_METHOD(void, set_rectangle, (miral::Window const&, geom::Rectangle const&, geom::Rectangle const&, bool), (override));
        MOCK_METHOD(MirWindowState, get_state, (miral::Window const&), (override));
        MOCK_METHOD(void, change_state, (miral::Window const&, MirWindowState), (override));
        MOCK_METHOD(void, clip, (miral::Window const&, geom::Rectangle const&), (override));
        MOCK_METHOD(void, noclip, (miral::Window const&), (override));
        MOCK_METHOD(void, select_active_window, (miral::Window const&), (override));
        MOCK_METHOD(std::shared_ptr<Container>, get_container, (miral::Window const&), (override));
        MOCK_METHOD(void, raise, (miral::Window const&), (override));
        MOCK_METHOD(void, send_to_back, (miral::Window const&), (override));
        MOCK_METHOD(void, open, (miral::Window const&), (override));
        MOCK_METHOD(void, close, (miral::Window const&), (override));
        MOCK_METHOD(void, set_user_data, (miral::Window const&, std::shared_ptr<void> const&), (override));
        MOCK_METHOD(void, modify, (miral::Window const&, miral::WindowSpecification const&), (override));
        MOCK_METHOD(miral::WindowInfo&, info_for, (miral::Window const&), (override));
        MOCK_METHOD(miral::ApplicationInfo&, info_for, (miral::Application const&), (override));
        MOCK_METHOD(miral::ApplicationInfo&, app_info, (miral::Window const&), (override));
        MOCK_METHOD(void, move_cursor_to, (float, float), (override));
        MOCK_METHOD(void, set_size_hack, (AnimationHandle, geom::Size const&), (override));
        MOCK_METHOD(miral::Window, window_at, (float, float), (override));
        MOCK_METHOD(void, process_animation, (AnimationStepResult const&, std::shared_ptr<Container> const&), (override));
    };

} // namespace test
} // namespace miracle

#endif // MIRACLE_MOCK_WINDOW_CONTROLLER_H

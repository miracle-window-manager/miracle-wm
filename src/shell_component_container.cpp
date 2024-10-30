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

#include "shell_component_container.h"
#include "window_controller.h"
#include <mir/scene/session.h>

namespace miracle
{

ShellComponentContainer::ShellComponentContainer(
    miral::Window const& window_,
    miracle::WindowController& window_controller) :
    window_ { window_ },
    window_controller { window_controller }
{
}

void ShellComponentContainer::show()
{
}

void ShellComponentContainer::hide()
{
}

void ShellComponentContainer::commit_changes()
{
}

mir::geometry::Rectangle ShellComponentContainer::get_logical_area() const
{
    return {
        window_.top_left(),
        window_.size()
    };
}

void ShellComponentContainer::set_logical_area(mir::geometry::Rectangle const& rectangle)
{
}

mir::geometry::Rectangle ShellComponentContainer::get_visible_area() const
{
    return get_logical_area();
}

void ShellComponentContainer::constrain()
{
}

std::weak_ptr<ParentContainer> ShellComponentContainer::get_parent() const
{
    return std::weak_ptr<ParentContainer>();
}

void ShellComponentContainer::set_parent(std::shared_ptr<ParentContainer> const& ptr)
{
}

size_t ShellComponentContainer::get_min_height() const
{
    return 0;
}

size_t ShellComponentContainer::get_min_width() const
{
    return 0;
}

void ShellComponentContainer::handle_ready()
{
    window_controller.select_active_window(window_);
}

void ShellComponentContainer::handle_modify(miral::WindowSpecification const& specification)
{
    window_controller.modify(window_, specification);
}

void ShellComponentContainer::handle_request_move(MirInputEvent const* input_event)
{
}

void ShellComponentContainer::handle_request_resize(MirInputEvent const* input_event, MirResizeEdge edge)
{
}

void ShellComponentContainer::handle_raise()
{
    window_controller.select_active_window(window_);
}

bool ShellComponentContainer::resize(Direction direction)
{
    return false;
}

bool ShellComponentContainer::toggle_fullscreen()
{
    return false;
}

void ShellComponentContainer::request_horizontal_layout()
{
}

void ShellComponentContainer::request_vertical_layout()
{
}

void ShellComponentContainer::toggle_layout(bool)
{
}

void ShellComponentContainer::on_focus_gained()
{
    window_controller.raise(window_);
}

void ShellComponentContainer::on_focus_lost()
{
}

void ShellComponentContainer::on_move_to(mir::geometry::Point const& top_left)
{
}

mir::geometry::Rectangle
ShellComponentContainer::confirm_placement(MirWindowState state, mir::geometry::Rectangle const& rectangle)
{
    return rectangle;
}

Workspace* ShellComponentContainer::get_workspace() const
{
    return nullptr;
}

Output* ShellComponentContainer::get_output() const
{
    return nullptr;
}

glm::mat4 ShellComponentContainer::get_transform() const
{
    return glm::mat4(1.f);
}

void ShellComponentContainer::set_transform(glm::mat4 transform)
{
}

glm::mat4 ShellComponentContainer::get_workspace_transform() const
{
    return glm::mat4(1.f);
}

glm::mat4 ShellComponentContainer::get_output_transform() const
{
    return glm::mat4(1.f);
}

uint32_t ShellComponentContainer::animation_handle() const
{
    return 0;
}

void ShellComponentContainer::animation_handle(uint32_t uint_32)
{
}

bool ShellComponentContainer::is_focused() const
{
    return false;
}

ContainerType ShellComponentContainer::get_type() const
{
    return ContainerType::shell;
}

void ShellComponentContainer::on_open()
{
}

std::optional<miral::Window> ShellComponentContainer::window() const
{
    return window_;
}

bool ShellComponentContainer::select_next(miracle::Direction)
{
    return false;
}

bool ShellComponentContainer::pinned(bool)
{
    return false;
}

bool ShellComponentContainer::pinned() const
{
    return true;
}

bool ShellComponentContainer::move(Direction direction)
{
    return false;
}

bool ShellComponentContainer::move_by(Direction direction, int pixels)
{
    return false;
}

bool ShellComponentContainer::move_to(int x, int y)
{
    return false;
}

bool ShellComponentContainer::is_fullscreen() const
{
    return false;
}

nlohmann::json ShellComponentContainer::to_json() const
{
    auto const app = window_.application();
    auto const& win_info = window_controller.info_for(window_);
    auto const visible_area = get_visible_area();
    auto const logical_area = get_logical_area();
    return {
        { "id",                   reinterpret_cast<std::uintptr_t>(this)                                                                                                                                                   },
        { "name",                 app->name()                                                                                                                                                                              },
        { "rect",                 {
                      { "x", logical_area.top_left.x.as_int() },
                      { "y", logical_area.top_left.y.as_int() },
                      { "width", logical_area.size.width.as_int() },
                      { "height", logical_area.size.height.as_int() },
                  }                                                                                                                                                                                       },
        { "focused",              is_focused()                                                                                                                                                                             },
        { "focus",                std::vector<int>()                                                                                                                                                                       },
        { "border",               "none"                                                                                                                                                                                   },
        { "current_border_width", 0                                                                                                                                                                                        },
        { "layout",               "dockarea"                                                                                                                                                                               },
        { "orientation",          "none"                                                                                                                                                                                   },
        { "window_rect",          {
                                                                                                                                                                                                     { "x", visible_area.top_left.x.as_int() },
                                                                                                                                                                                                     { "y", visible_area.top_left.y.as_int() },
                                                                                                                                                                                                     { "width", visible_area.size.width.as_int() },
                                                                                                                                                                                                     { "height", visible_area.size.height.as_int() },
                                                                                                                                                                                                 } },
        { "deco_rect",            {
                           { "x", 0 },
                           { "y", 0 },
                           { "width", logical_area.size.width.as_int() },
                           { "height", logical_area.size.height.as_int() },
                       }                                                                                                                                                                             },
        { "geometry",             {
                          { "x", 0 },
                          { "y", 0 },
                          { "width", logical_area.size.width.as_int() },
                          { "height", logical_area.size.height.as_int() },
                      }                                                                                                                                                                               },
        { "window",               0                                                                                                                                                                                        }, // TODO
        { "urgent",               false                                                                                                                                                                                    },
        { "floating_nodes",       std::vector<int>()                                                                                                                                                                       },
        { "sticky",               false                                                                                                                                                                                    },
        { "type",                 "dockarea"                                                                                                                                                                               },
        { "fullscreen_mode",      is_fullscreen() ? 1 : 0                                                                                                                                                                  }, // TODO: Support value 2
        { "pid",                  app->process_id()                                                                                                                                                                        },
        { "app_id",               win_info.application_id()                                                                                                                                                                },
        { "visible",              true                                                                                                                                                                                     },
        { "shell",                "miracle-wm"                                                                                                                                                                             }, // TODO
        { "inhibit_idle",         false                                                                                                                                                                                    },
        { "idle_inhibitors",      {
                                                            { "application", "none" },
                                                            { "user", "visible" },
                                                        }                                                                                                                                      },
        { "window_properties",    {}                                                                                                                                                                                       }, // TODO
        { "nodes",                std::vector<int>()                                                                                                                                                                       }
    };
}

} // miracle
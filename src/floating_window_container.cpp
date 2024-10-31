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

#include "mir_toolkit/common.h"
#define MIR_LOG_COMPONENT "floating_container"

#include "compositor_state.h"
#include "config.h"
#include "floating_window_container.h"
#include "leaf_container.h"
#include "output.h"
#include "workspace.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <mir/log.h>
#include <mir/scene/session.h>

using namespace miracle;

FloatingWindowContainer::FloatingWindowContainer(
    miral::Window const& window,
    std::shared_ptr<MinimalWindowManager> const& wm,
    WindowController& window_controller,
    Workspace* workspace,
    CompositorState const& state,
    std::shared_ptr<MiracleConfig> const& config) :
    window_ { window },
    wm { wm },
    window_controller { window_controller },
    workspace_ { workspace },
    state { state },
    config { config }
{
}

void FloatingWindowContainer::commit_changes()
{
}

mir::geometry::Rectangle FloatingWindowContainer::get_logical_area() const
{
    return get_visible_area();
}

void FloatingWindowContainer::set_logical_area(mir::geometry::Rectangle const& rectangle)
{
    window_controller.set_rectangle(window_, get_visible_area(), rectangle);
}

mir::geometry::Rectangle FloatingWindowContainer::get_visible_area() const
{
    return {
        window_.top_left(),
        window_.size()
    };
}

void FloatingWindowContainer::constrain() { }

void FloatingWindowContainer::set_parent(std::shared_ptr<ParentContainer> const& parent)
{
    throw std::logic_error("FloatingContainer cannot have a parent");
}

size_t FloatingWindowContainer::get_min_height() const
{
    return 1;
}

size_t FloatingWindowContainer::get_min_width() const
{
    return 1;
}

void FloatingWindowContainer::handle_ready()
{
    auto& info = window_controller.info_for(window_);
    wm->handle_window_ready(info);
}

void FloatingWindowContainer::handle_modify(miral::WindowSpecification const& modifications)
{
    auto& info = window_controller.info_for(window_);
    wm->handle_modify_window(info, modifications);
}

void FloatingWindowContainer::handle_request_move(MirInputEvent const* input_event)
{
    wm->handle_request_move(
        window_controller.info_for(window_), input_event);
}

void FloatingWindowContainer::handle_request_resize(
    MirInputEvent const* input_event, MirResizeEdge edge)
{
    wm->handle_request_resize(
        window_controller.info_for(window_), input_event, edge);
}

void FloatingWindowContainer::handle_raise()
{
    wm->handle_raise_window(window_controller.info_for(window_));
}

void FloatingWindowContainer::on_open()
{
    window_controller.open(window_);
}

void FloatingWindowContainer::on_focus_gained()
{
    if (get_output()->get_active_workspace()->get_workspace() != workspace_->get_workspace())
        return;

    wm->advise_focus_gained(window_controller.info_for(window_));
}

void FloatingWindowContainer::on_focus_lost()
{
    wm->advise_focus_lost(window_controller.info_for(window_));
}

void FloatingWindowContainer::on_move_to(geom::Point const& top_left)
{
    wm->advise_move_to(window_controller.info_for(window_), top_left);
}

mir::geometry::Rectangle FloatingWindowContainer::confirm_placement(
    MirWindowState state, mir::geometry::Rectangle const& placement)
{
    return wm->confirm_placement_on_display(
        window_controller.info_for(window_),
        state,
        placement);
}

bool FloatingWindowContainer::pinned() const
{
    return is_pinned;
}

bool FloatingWindowContainer::pinned(bool in)
{
    is_pinned = in;
    return true;
}

std::optional<miral::Window> FloatingWindowContainer::window() const
{
    return window_;
}

bool FloatingWindowContainer::resize(Direction direction)
{
    return false;
}

bool FloatingWindowContainer::toggle_fullscreen()
{
    return false;
}

void FloatingWindowContainer::request_horizontal_layout()
{
}

void FloatingWindowContainer::request_vertical_layout()
{
}

void FloatingWindowContainer::toggle_layout(bool)
{
}

void FloatingWindowContainer::show()
{
    if (pinned())
    {
        window_controller.raise(window_);
        return;
    }

    if (restore_state_)
    {
        miral::WindowSpecification spec;
        spec.state() = restore_state_.value();
        window_controller.modify(window_, spec);
        window_controller.raise(window_);
        restore_state_.reset();
    }
}

void FloatingWindowContainer::hide()
{
    if (pinned())
        return;

    restore_state_ = window_controller.info_for(window_).state();
    window_controller.change_state(window_, mir_window_state_hidden);
    window_controller.send_to_back(window_);
}

Workspace* FloatingWindowContainer::get_workspace() const
{
    return workspace_;
}

void FloatingWindowContainer::set_workspace(Workspace* workspace)
{
    workspace_ = workspace;
}

Output* FloatingWindowContainer::get_output() const
{
    return workspace_->get_output();
}

glm::mat4 FloatingWindowContainer::get_transform() const
{
    return transform;
}

void FloatingWindowContainer::set_transform(glm::mat4 transform_)
{
    transform = transform_;
}

uint32_t FloatingWindowContainer::animation_handle() const
{
    return animation_handle_;
}

void FloatingWindowContainer::animation_handle(uint32_t handle)
{
    animation_handle_ = handle;
}

bool FloatingWindowContainer::is_focused() const
{
    return state.active.get() == this;
}

bool FloatingWindowContainer::is_fullscreen() const
{
    return window_controller.is_fullscreen(window_);
}

ContainerType FloatingWindowContainer::get_type() const
{
    return ContainerType::floating_window;
}

glm::mat4 FloatingWindowContainer::get_workspace_transform() const
{
    if (pinned())
        return glm::mat4(1.f);

    auto output = get_output();
    auto workspace = get_workspace();
    auto const workspace_rect = output->get_workspace_rectangle(workspace->get_workspace());
    return glm::translate(
        glm::vec3(workspace_rect.top_left.x.as_int(), workspace_rect.top_left.y.as_int(), 0));
}

glm::mat4 FloatingWindowContainer::get_output_transform() const
{
    if (pinned())
        return glm::mat4(1.f);

    return get_output()->get_transform();
}

bool FloatingWindowContainer::select_next(Direction)
{
    return false;
}

bool FloatingWindowContainer::move(Direction direction)
{
    return move_by(direction, 10);
}

bool FloatingWindowContainer::move_by(Direction direction, int pixels)
{
    auto& info = window_controller.info_for(window_);
    auto prev_pos = window_.top_left();
    miral::WindowSpecification spec;
    switch (direction)
    {
    case Direction::down:
        spec.top_left() = {
            prev_pos.x.as_int(), prev_pos.y.as_int() + pixels
        };
        break;
    case Direction::up:
        spec.top_left() = {
            prev_pos.x.as_int(), prev_pos.y.as_int() - pixels
        };
        break;
    case Direction::left:
        spec.top_left() = {
            prev_pos.x.as_int() - pixels, prev_pos.y.as_int()
        };
        break;
    case Direction::right:
        spec.top_left() = {
            prev_pos.x.as_int() + pixels, prev_pos.y.as_int()
        };
        break;
    default:
        mir::log_warning("Unknown direction to move_active_window_by_amount: %d\n", (int)direction);
        return false;
    }

    window_controller.modify(info.window(), spec);
    return true;
}

bool FloatingWindowContainer::move_to(int x, int y)
{
    miral::WindowSpecification spec;
    spec.top_left() = { x, y };
    window_controller.modify(window_, spec);
    return true;
}

std::weak_ptr<ParentContainer> FloatingWindowContainer::get_parent() const
{
    return std::weak_ptr<ParentContainer>();
}

nlohmann::json FloatingWindowContainer::to_json() const
{
    auto const app = window_.application();
    auto const& win_info = window_controller.info_for(window_);
    auto logical_area = get_logical_area();
    auto visible_area = get_visible_area();
    auto workspace = get_workspace();
    auto output = get_output();
    bool visible = true;

    if (!output->is_active())
        visible = false;

    if (output->get_active_workspace_num() != workspace->get_workspace())
        visible = false;

    return {
        { "id",                   reinterpret_cast<std::uintptr_t>(this)                                                                                                                                                                                                    },
        { "name",                 app->name()                                                                                                                                                                                                                               },
        { "rect",                 {
                      { "x", logical_area.top_left.x.as_int() },
                      { "y", logical_area.top_left.y.as_int() },
                      { "width", logical_area.size.width.as_int() },
                      { "height", logical_area.size.height.as_int() },
                  }                                                                                                                                                                                                                                        },
        { "focused",              visible && is_focused()                                                                                                                                                                                                                              },
        { "focus",                std::vector<int>()                                                                                                                                                                                                                        },
        { "border",               "normal"                                                                                                                                                                                                                                  },
        { "current_border_width", config->get_border_config().size                                                                                                                                                                                                          },
        { "layout",               "none"                                                                                                                                                                                                                                    },
        { "orientation",          "none"                                                                                                                                                                                                                                    },
        { "percent",              1.0                                                                                                                                                                                                                                       },
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
                       }                                                                                                                                                                                                                              },
        { "geometry",             {
                          { "x", 0 },
                          { "y", 0 },
                          { "width", logical_area.size.width.as_int() },
                          { "height", logical_area.size.height.as_int() },
                      }                                                                                                                                                                                                                                },
        { "window",               0                                                                                                                                                                                                                                         }, // TODO
        { "urgent",               false                                                                                                                                                                                                                                     },
        { "floating_nodes",       std::vector<int>()                                                                                                                                                                                                                        },
        { "sticky",               false                                                                                                                                                                                                                                     },
        { "type",                 "floating_con"                                                                                                                                                                                                                            },
        { "fullscreen_mode",      is_fullscreen() ? 1 : 0                                                                                                                                                                                                                   }, // TODO: Support value 2
        { "pid",                  app->process_id()                                                                                                                                                                                                                         },
        { "app_id",               win_info.application_id()                                                                                                                                                                                                                 },
        { "visible",              pinned() || visible                                                                                                                                                                                                                       },
        { "shell",                "miracle-wm"                                                                                                                                                                                                                              }, // TODO
        { "inhibit_idle",         false                                                                                                                                                                                                                                     },
        { "idle_inhibitors",      {
                                                            { "application", "none" },
                                                            { "user", "visible" },
                                                        }                                                                                                                                                                                       },
        { "window_properties",    {}                                                                                                                                                                                                                                        }, // TODO
        { "nodes",                std::vector<int>()                                                                                                                                                                                                                        }
    };
}

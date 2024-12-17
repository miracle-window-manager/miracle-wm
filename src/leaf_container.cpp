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

#include "leaf_container.h"
#include "compositor_state.h"
#include "config.h"
#include "container_group_container.h"
#include "output.h"
#include "parent_container.h"
#include "tiling_window_tree.h"
#include "window_helpers.h"
#include "workspace.h"

#include <cmath>
#include <mir/scene/session.h>
#include <mir/scene/surface.h>
#include <mir_toolkit/common.h>

using namespace miracle;

LeafContainer::LeafContainer(
    WindowController& node_interface,
    geom::Rectangle area,
    std::shared_ptr<Config> const& config,
    TilingWindowTree* tree,
    std::shared_ptr<ParentContainer> const& parent,
    CompositorState const& state) :
    window_controller { node_interface },
    logical_area { std::move(area) },
    config { config },
    tree { tree },
    parent { parent },
    state { state }
{
}

void LeafContainer::associate_to_window(miral::Window const& in_window)
{
    window_ = in_window;
}

geom::Rectangle LeafContainer::get_logical_area() const
{
    return next_logical_area ? next_logical_area.value() : logical_area;
}

void LeafContainer::set_logical_area(geom::Rectangle const& target_rect)
{
    next_logical_area = target_rect;
}

std::weak_ptr<ParentContainer> LeafContainer::get_parent() const
{
    return parent;
}

void LeafContainer::set_parent(std::shared_ptr<ParentContainer> const& in_parent)
{
    parent = in_parent;
}

void LeafContainer::set_state(MirWindowState state)
{
    next_state = state;
}

geom::Rectangle LeafContainer::get_visible_area() const
{
    // TODO: Could cache these half values in the config
    int const half_gap_x = (int)(ceil((double)config->get_inner_gaps_x() / 2.0));
    int const half_gap_y = (int)(ceil((double)config->get_inner_gaps_y() / 2.0));
    auto neighbors = get_neighbors();
    int x = logical_area.top_left.x.as_int();
    int y = logical_area.top_left.y.as_int();
    int width = logical_area.size.width.as_int();
    int height = logical_area.size.height.as_int();
    if (neighbors[(int)Direction::left])
    {
        x += half_gap_x;
        width -= half_gap_x;
    }
    if (neighbors[(int)Direction::right])
    {
        width -= half_gap_x;
    }
    if (neighbors[(int)Direction::up])
    {
        y += half_gap_y;
        height -= half_gap_y;
    }
    if (neighbors[(int)Direction::down])
    {
        height -= half_gap_y;
    }

    int const border_size = config->get_border_config().size;
    x += border_size;
    width -= 2 * border_size;
    y += border_size;
    height -= 2 * border_size;

    return {
        geom::Point { x,     y      },
        geom::Size { width, height }
    };
}

void LeafContainer::constrain()
{
    if (window_controller.is_fullscreen(window_))
        window_controller.noclip(window_);
    else
        window_controller.clip(window_, get_visible_area());
}

size_t LeafContainer::get_min_width() const
{
    return 50;
}

size_t LeafContainer::get_min_height() const
{
    return 50;
}

void LeafContainer::handle_ready()
{
    tree->handle_container_ready(*this);
    get_workspace()->handle_ready_hack(*this);
    if (window_controller.is_fullscreen(window_))
        toggle_fullscreen();
}

void LeafContainer::handle_modify(miral::WindowSpecification const& modifications)
{
    auto const& info = window_controller.info_for(window_);

    auto mods = modifications;
    if (mods.state().is_set() && mods.state().value() != info.state())
    {
        set_state(mods.state().value());
        commit_changes();

        if (window_helpers::is_window_fullscreen(mods.state().value()))
            tree->advise_fullscreen_container(*this);
        else if (mods.state().value() == mir_window_state_restored)
            tree->advise_restored_container(*this);
    }

    // If we are trying to set the window size to something that we don't want it
    // to be, then let's consume it.
    if (!is_fullscreen()
        && mods.size().is_set()
        && get_visible_area().size != mods.size().value())
    {
        mods.size().consume();
    }

//    if (mods.size())
//        set_transform(glm::mat4(1.f));

    if (mods.size().is_set())
        window_controller.set_size_hack(animation_handle_, mods.size().value());

    window_controller.modify(window_, mods);
}

void LeafContainer::handle_raise()
{
    window_controller.select_active_window(window_);
}

bool LeafContainer::resize(miracle::Direction direction, int pixels)
{
    return tree->resize_container(direction, pixels, *this);
}

bool LeafContainer::set_size(std::optional<int> const& width, std::optional<int> const& height)
{
    return tree->set_size(width, height, *this);
}

void LeafContainer::show()
{
    next_state = before_shown_state;
    before_shown_state.reset();
    commit_changes();
    window_controller.raise(window_);
}

void LeafContainer::hide()
{
    before_shown_state = window_controller.get_state(window_);
    next_state = mir_window_state_hidden;
    commit_changes();
    window_controller.send_to_back(window_);
}

bool LeafContainer::toggle_fullscreen()
{
    if (window_controller.is_fullscreen(window_))
        next_state = mir_window_state_restored;
    else
        next_state = mir_window_state_fullscreen;

    commit_changes();
    return tree->toggle_fullscreen(*this);
}

mir::geometry::Rectangle LeafContainer::confirm_placement(
    MirWindowState state, mir::geometry::Rectangle const& placement)
{
    auto new_placement = placement;
    tree->confirm_placement_on_display(*this, state, new_placement);
    return new_placement;
}

void LeafContainer::on_open()
{
    window_controller.open(window_);
}

void LeafContainer::on_focus_gained()
{
    tree->advise_focus_gained(*this);
}

void LeafContainer::on_focus_lost()
{
}

void LeafContainer::on_move_to(geom::Point const&)
{
}

bool LeafContainer::is_fullscreen() const
{
    return window_controller.is_fullscreen(window_);
}

void LeafContainer::commit_changes()
{
    if (next_state)
    {
        window_controller.change_state(window_, next_state.value());
        constrain();
        next_state.reset();
    }

    if (next_logical_area)
    {
        auto previous = get_visible_area();
        logical_area = next_logical_area.value();
        next_logical_area.reset();
        if (!window_controller.is_fullscreen(window_))
            window_controller.set_rectangle(window_, previous, get_visible_area());
    }
}

void LeafContainer::handle_request_move(MirInputEvent const* input_event)
{
}

void LeafContainer::handle_request_resize(MirInputEvent const* input_event, MirResizeEdge edge)
{
}

void LeafContainer::request_horizontal_layout()
{
    tree->request_horizontal_layout(*this);
}

void LeafContainer::request_vertical_layout()
{
    tree->request_vertical_layout(*this);
}

void LeafContainer::toggle_layout(bool cycle_thru_all)
{
    tree->toggle_layout(*this, cycle_thru_all);
}

void LeafContainer::set_tree(TilingWindowTree* tree_)
{
    tree = tree_;
}

Workspace* LeafContainer::get_workspace() const
{
    return tree->get_workspace();
}

Output* LeafContainer::get_output() const
{
    return get_workspace()->get_output();
}

glm::mat4 LeafContainer::get_transform() const
{
    return transform;
}

void LeafContainer::set_transform(glm::mat4 transform_)
{
    if (auto surface = window_.operator std::shared_ptr<mir::scene::Surface>())
    {
        surface->set_transformation(transform_);
        transform = transform_;
    }
}

uint32_t LeafContainer::animation_handle() const
{
    return animation_handle_;
}

void LeafContainer::animation_handle(uint32_t handle)
{
    animation_handle_ = handle;
}

bool LeafContainer::is_focused() const
{
    if (state.active().get() == this || parent.lock()->is_focused())
        return true;

    auto group = Container::as_group(state.active());
    if (!group)
        return false;

    return group->contains(shared_from_this());
}

ContainerType LeafContainer::get_type() const
{
    return ContainerType::leaf;
}

bool LeafContainer::select_next(miracle::Direction direction)
{
    return tree->select_next(direction, *this);
}

bool LeafContainer::pinned(bool)
{
    return false;
}

bool LeafContainer::pinned() const
{
    return false;
}

bool LeafContainer::move(miracle::Direction direction)
{
    return tree->move_container(direction, *this);
}

bool LeafContainer::move_by(Direction, int)
{
    return false;
}

bool LeafContainer::move_to(int, int)
{
    return false;
}

bool LeafContainer::toggle_tabbing()
{
    if (auto sh_parent = parent.lock())
    {
        if (sh_parent->get_direction() == LayoutScheme::tabbing)
            tree->request_horizontal_layout(*this);
        else
            tree->request_tabbing_layout(*this);
    }
    return true;
}

bool LeafContainer::toggle_stacking()
{
    if (auto sh_parent = parent.lock())
    {
        if (sh_parent->get_direction() == LayoutScheme::stacking)
            tree->request_horizontal_layout(*this);
        else
            tree->request_stacking_layout(*this);
    }
    return true;
}

bool LeafContainer::set_layout(LayoutScheme scheme)
{
    tree->request_layout(*this, scheme);
    return true;
}

LayoutScheme LeafContainer::get_layout() const
{
    auto sh_parent = parent.lock().get();
    if (!sh_parent)
        return LayoutScheme::none;

    if (sh_parent->num_nodes() == 1)
        return sh_parent->get_layout();

    return LayoutScheme::none;
}

nlohmann::json LeafContainer::to_json() const
{
    auto const app = window_.application();
    auto const& win_info = window_controller.info_for(window_);
    auto visible_area = get_visible_area();
    auto workspace = get_workspace();
    auto output = get_output();
    auto locked_parent = parent.lock();
    bool visible = true;

    if (!output->is_active())
        visible = false;

    if (output->active() != workspace)
        visible = false;

    if (locked_parent == nullptr)
        visible = false;

    if (locked_parent->get_scheme() == LayoutScheme::stacking || locked_parent->get_scheme() == LayoutScheme::tabbing)
        if (!is_focused())
            visible = false;

    nlohmann::json properties = nlohmann::json::object();
    return {
        { "id",                   reinterpret_cast<std::uintptr_t>(this)                                                                                                                                                                                                                                   },
        { "name",                 app->name()                                                                                                                                                                                                                                                              },
        { "rect",                 {
                      { "x", logical_area.top_left.x.as_int() },
                      { "y", logical_area.top_left.y.as_int() },
                      { "width", logical_area.size.width.as_int() },
                      { "height", logical_area.size.height.as_int() },
                  }                                                                                                                                                                                                                                                                       },
        { "focused",              visible && is_focused()                                                                                                                                                                                                                                                  },
        { "focus",                std::vector<int>()                                                                                                                                                                                                                                                       },
        { "border",               "normal"                                                                                                                                                                                                                                                                 },
        { "current_border_width", config->get_border_config().size                                                                                                                                                                                                                                         },
        { "layout",               "none"                                                                                                                                                                                                                                                                   },
        { "orientation",          "none"                                                                                                                                                                                                                                                                   },
        { "percent",              get_percent_of_parent()                                                                                                                                                                                                                                                  },
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
                       }                                                                                                                                                                                                                                                             },
        { "geometry",             {
                          { "x", 0 },
                          { "y", 0 },
                          { "width", logical_area.size.width.as_int() },
                          { "height", logical_area.size.height.as_int() },
                      }                                                                                                                                                                                                                                                               },
        { "window",               0                                                                                                                                                                                                                                                                        }, // TODO
        { "urgent",               false                                                                                                                                                                                                                                                                    },
        { "floating_nodes",       std::vector<int>()                                                                                                                                                                                                                                                       },
        { "sticky",               false                                                                                                                                                                                                                                                                    },
        { "type",                 "con"                                                                                                                                                                                                                                                                    },
        { "fullscreen_mode",      is_fullscreen() ? 1 : 0                                                                                                                                                                                                                                                  }, // TODO: Support value 2
        { "pid",                  app->process_id()                                                                                                                                                                                                                                                        },
        { "app_id",               win_info.application_id()                                                                                                                                                                                                                                                },
        { "visible",              visible                                                                                                                                                                                                                                                                  },
        { "shell",                "miracle-wm"                                                                                                                                                                                                                                                             }, // TODO
        { "inhibit_idle",         false                                                                                                                                                                                                                                                                    },
        { "idle_inhibitors",      {
                                                            { "application", "none" },
                                                            { "user", "visible" },
                                                        }                                                                                                                                                                                                                      },
        { "window_properties",    properties                                                                                                                                                                                                                                                               }, // TODO
        { "nodes",                std::vector<int>()                                                                                                                                                                                                                                                       }
    };
}
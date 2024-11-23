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

#define MIR_LOG_COMPONENT "output_content"
#define GLM_ENABLE_EXPERIMENTAL

#include "output.h"
#include "animator.h"
#include "compositor_state.h"
#include "floating_window_container.h"
#include "leaf_container.h"
#include "vector_helpers.h"
#include "window_helpers.h"

#include "workspace.h"
#include "workspace_manager.h"
#include <glm/gtx/transform.hpp>
#include <memory>
#include <mir/log.h>
#include <mir/scene/surface.h>
#include <miral/toolkit_event.h>
#include <miral/window_info.h>

using namespace miracle;

Output::Output(
    miral::Output const& output,
    WorkspaceManager& workspace_manager,
    geom::Rectangle const& area,
    miral::WindowManagerTools const& tools,
    std::shared_ptr<MinimalWindowManager> const& floating_window_manager,
    CompositorState& state,
    std::shared_ptr<Config> const& config,
    WindowController& node_interface,
    Animator& animator) :
    output { output },
    workspace_manager { workspace_manager },
    area { area },
    tools { tools },
    floating_window_manager { floating_window_manager },
    state { state },
    config { config },
    window_controller { node_interface },
    animator { animator },
    handle { animator.register_animateable() }
{
}

Workspace* Output::active() const
{
    if (active_workspace.expired())
        return nullptr;

    return active_workspace.lock().get();
}

std::shared_ptr<Container> Output::intersect(const MirPointerEvent* event)
{
    if (active_workspace.expired())
    {
        mir::log_error("Output::handle_pointer_event: unexpectedly trying to handle a pointer event when we lack workspaces");
        return nullptr;
    }

    auto x = miral::toolkit::mir_pointer_event_axis_value(event, MirPointerAxis::mir_pointer_axis_x);
    auto y = miral::toolkit::mir_pointer_event_axis_value(event, MirPointerAxis::mir_pointer_axis_y);
    if (auto const window = tools.window_at({ x, y }))
        return window_controller.get_container(window);

    return nullptr;
}

AllocationHint Output::allocate_position(
    miral::ApplicationInfo const& app_info,
    miral::WindowSpecification& requested_specification,
    AllocationHint hint)
{
    hint.container_type = hint.container_type == ContainerType::none
        ? window_helpers::get_ideal_type(requested_specification)
        : hint.container_type;
    if (hint.container_type == ContainerType::shell)
        return hint;

    return active()->allocate_position(app_info, requested_specification, hint);
}

std::shared_ptr<Container> Output::create_container(
    miral::WindowInfo const& window_info, AllocationHint const& hint) const
{
    return active()->create_container(window_info, hint);
}

void Output::delete_container(std::shared_ptr<miracle::Container> const& container)
{
    auto workspace = container->get_workspace();
    if (!workspace)
        return;

    workspace->delete_container(container);
}

void Output::advise_new_workspace(WorkspaceCreationData const&& data)
{
    // Workspaces are always kept in sorted order with numbered workspaces in front followed by all other workspaces
    auto new_workspace = std::make_shared<Workspace>(
        this, tools, data.id, data.num, data.name, config, window_controller, state, floating_window_manager);
    insert_sorted(workspaces, new_workspace, [](std::shared_ptr<Workspace> const& a, std::shared_ptr<Workspace> const& b)
    {
        if (a->num() && b->num())
            return a->num().value() < b->num().value();
        else if (a->num())
            return true;
        else if (b->num())
            return false;
        else
            return false;
    });
}

void Output::advise_workspace_deleted(uint32_t id)
{
    for (auto it = workspaces.begin(); it != workspaces.end(); it++)
    {
        if (it->get()->id() == id)
        {
            workspaces.erase(it);
            return;
        }
    }
}

bool Output::advise_workspace_active(uint32_t id)
{
    std::shared_ptr<Workspace> from = nullptr;
    std::shared_ptr<Workspace> to = nullptr;
    int from_index = -1;
    int to_index = -1;
    for (int i = 0; i < workspaces.size(); i++)
    {
        auto const& workspace = workspaces[i];
        if (!active_workspace.expired() && workspace == active_workspace.lock())
        {
            from = workspace;
            from_index = i;
        }

        if (workspace->id() == id)
        {
            if (!active_workspace.expired() && active_workspace.lock()->id() == id)
                return true;

            to = workspace;
            to_index = i;
        }
    }

    if (!to)
    {
        mir::fatal_error("advise_workspace_active: switch to workspace that doesn't exist: %d", id);
        return false;
    }

    if (!from)
    {
        to->show();
        active_workspace = to;

        auto to_rectangle = get_workspace_rectangle(to_index);
        set_position(glm::vec2(
            -to_rectangle.top_left.x.as_int(),
            -to_rectangle.top_left.y.as_int()));
        to->trigger_rerender();
        return true;
    }

    // Note: It is very important that [active_workspace] be modified before notifications
    // are sent out.
    active_workspace = to;

    auto from_src = get_workspace_rectangle(from_index);
    from->transfer_pinned_windows_to(to);

    // Show everyone so that we can animate over all workspaces
    for (auto const& workspace : workspaces)
        workspace->show();

    geom::Rectangle real {
        { geom::X { position_offset.x }, geom::Y { position_offset.y } },
        area.size
    };
    auto to_src = get_workspace_rectangle(to_index);
    geom::Rectangle src {
        { geom::X { -from_src.top_left.x.as_int() }, geom::Y { from_src.top_left.y.as_int() } },
        area.size
    };
    geom::Rectangle dest {
        { geom::X { -to_src.top_left.x.as_int() }, geom::Y { to_src.top_left.y.as_int() } },
        area.size
    };

    // If 'from' is empty, we can delete the workspace. However, this means that from_src is now incorrect
    if (from->is_empty())
        workspace_manager.delete_workspace(from->id());

    animator.workspace_switch(
        handle,
        src,
        dest,
        real,
        [this, to = to, from = from](AnimationStepResult const& asr)
    {
        if (asr.is_complete)
        {
            if (asr.position)
                set_position(asr.position.value());
            if (asr.transform)
                set_transform(asr.transform.value());

            for (auto const& workspace : workspaces)
            {
                if (workspace != to)
                    workspace->hide();
            }

            to->trigger_rerender();
            return;
        }

        if (asr.position)
            set_position(asr.position.value());
        if (asr.transform)
            set_transform(asr.transform.value());

        for (auto const& workspace : workspaces)
            workspace->trigger_rerender();
    });

    return true;
}

void Output::advise_application_zone_create(miral::Zone const& application_zone)
{
    if (application_zone.extents().contains(area))
    {
        application_zone_list.push_back(application_zone);
        for (auto& workspace : workspaces)
            workspace->recalculate_area();
    }
}

void Output::advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original)
{
    for (auto& zone : application_zone_list)
        if (zone == original)
        {
            zone = updated;
            for (auto& workspace : workspaces)
                workspace->recalculate_area();
            break;
        }
}

void Output::advise_application_zone_delete(miral::Zone const& application_zone)
{
    auto const original_size = application_zone_list.size();
    application_zone_list.erase(
        std::remove(application_zone_list.begin(), application_zone_list.end(), application_zone),
        application_zone_list.end());

    if (application_zone_list.size() != original_size)
    {
        for (auto& workspace : workspaces)
            workspace->recalculate_area();
    }
}

bool Output::point_is_in_output(int x, int y)
{
    return area.contains(geom::Point(x, y));
}

void Output::update_area(geom::Rectangle const& new_area)
{
    area = new_area;
    for (auto& workspace : workspaces)
        workspace->set_area(area);
}

std::vector<miral::Window> Output::collect_all_windows() const
{
    std::vector<miral::Window> windows;
    for (auto& workspace : get_workspaces())
    {
        workspace->for_each_window([&](std::shared_ptr<Container> const& container)
        {
            if (auto window = container->window())
                windows.push_back(*window);
        });
    }

    return windows;
}

void Output::add_immediately(miral::Window& window, AllocationHint hint)
{
    auto& prev_info = window_controller.info_for(window);
    miral::WindowSpecification spec = window_helpers::copy_from(prev_info);

    // If we are adding a window immediately, let's force it back into existence
    if (spec.state() == mir_window_state_hidden)
        spec.state() = mir_window_state_restored;

    AllocationHint result = allocate_position(tools.info_for(window.application()), spec, hint);
    tools.modify_window(window, spec);
    auto container = create_container(window_controller.info_for(window), result);
    container->handle_ready();
}

void Output::graft(std::shared_ptr<Container> const& container)
{
    active()->graft(container);
}

geom::Rectangle Output::get_workspace_rectangle(size_t i) const
{
    // TODO: Support vertical workspaces one day in the future
    auto const& workspace = workspaces[i];
    size_t x = 0;
    if (workspace->num())
        x = (workspace->num().value() - 1) * area.size.width.as_int();
    else
    {
        // Find the index of the first non-numbered workspace
        size_t j = 0;
        for (; j < workspaces.size(); j++)
        {
            if (workspaces[j]->num() == std::nullopt)
                break;
        }

        x = ((WorkspaceManager::NUM_DEFAULT_WORKSPACES - 1) + (i - j)) * area.size.width.as_int();
    }

    return geom::Rectangle {
        geom::Point { geom::X { x },            geom::Y { 0 }             },
        geom::Size { area.size.width.as_int(), area.size.height.as_int() }
    };
}

[[nodiscard]] Workspace const* Output::workspace(uint32_t id) const
{
    for (auto const& workspace : workspaces)
    {
        if (workspace->id() == id)
            return workspace.get();
    }

    return nullptr;
}

glm::mat4 Output::get_transform() const
{
    return final_transform;
}

void Output::set_transform(glm::mat4 const& in)
{
    transform = in;
    final_transform = glm::translate(transform, glm::vec3(position_offset.x, position_offset.y, 0));
}

void Output::set_position(glm::vec2 const& v)
{
    position_offset = v;
    final_transform = glm::translate(transform, glm::vec3(position_offset.x, position_offset.y, 0));
}

nlohmann::json Output::to_json() const
{
    nlohmann::json nodes = nlohmann::json::array();
    for (auto const& workspace : workspaces)
    {
        if (workspace)
            nodes.push_back(workspace->to_json());
    }

    return {
        { "id",                   reinterpret_cast<std::uintptr_t>(this) },
        { "name",                 output.name()                          },
        { "type",                 "output"                               },
        { "layout",               "output"                               },
        { "orientation",          "none"                                 },
        { "visible",              true                                   },
        { "focused",              is_active()                            },
        { "urgent",               false                                  },
        { "border",               "none"                                 },
        { "current_border_width", 0                                      },
        { "window_rect",          {
                             { "x", 0 },
                             { "y", 0 },
                             { "width", 0 },
                             { "height", 0 },
                         }                       },
        { "deco_rect",            {
                           { "x", 0 },
                           { "y", 0 },
                           { "width", 0 },
                           { "height", 0 },
                       }                           },
        { "geometry",             {
                          { "x", 0 },
                          { "y", 0 },
                          { "width", 0 },
                          { "height", 0 },
                      }                             },
        { "rect",                 {
                      { "x", area.top_left.x.as_int() },
                      { "y", area.top_left.y.as_int() },
                      { "width", area.size.width.as_int() },
                      { "height", area.size.height.as_int() },
                  }                                     },
        { "nodes",                nodes                                  }
    };
}

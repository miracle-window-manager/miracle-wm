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

#define MIR_LOG_COMPONENT "parent_container"
#include "parent_container.h"
#include "compositor_state.h"
#include "config.h"
#include "container.h"
#include "leaf_container.h"
#include "output.h"
#include "tiling_window_tree.h"
#include "workspace.h"
#include <cmath>
#include <mir/log.h>

using namespace miracle;

namespace
{
struct InsertNodeInternalResult
{
    int size;
    int position;
};

InsertNodeInternalResult insert_node_internal(
    int lane_size,
    int lane_pos,
    int index,
    size_t node_count,
    std::function<int(int)> const& get_node_size,
    std::function<void(int, int, int)> const& set_node_size_position)
{
    int new_item_size = floor((double)lane_size / (double)(node_count + 1));
    int new_item_position = lane_pos + index * new_item_size;

    int size_lost = 0;
    int prev_pos = lane_pos;
    int prev_size = 0;
    for (int i = 0; i < node_count; i++)
    {
        int node_size = get_node_size(i);

        // Each node will lose a percentage of its width that corresponds to what it can give
        // (meaning that larger nodes give more width, and lesser nodes give less width)
        double percent_size_lost = ((double)node_size / (double)lane_size);
        int width_to_lose = (int)floor(percent_size_lost * new_item_size);
        size_lost += width_to_lose;

        if (i == index)
        {
            prev_size = new_item_size;
            prev_pos = new_item_position;
        }

        int changed_node_size = node_size - width_to_lose;
        int changed_node_pos = prev_pos + prev_size;
        set_node_size_position(i, changed_node_size, changed_node_pos);

        prev_pos = changed_node_pos;
        prev_size = changed_node_size;
    }

    if (node_count)
    {
        new_item_size += size_lost - new_item_size;
        new_item_position -= size_lost - new_item_size;
    }

    return { new_item_size, new_item_position };
}
}

ParentContainer::ParentContainer(
    WindowController& node_interface,
    geom::Rectangle area,
    std::shared_ptr<Config> const& config,
    TilingWindowTree* tree,
    std::shared_ptr<ParentContainer> const& parent,
    CompositorState const& state) :
    node_interface { node_interface },
    logical_area { std::move(area) },
    tree { tree },
    config { config },
    parent { parent },
    state { state },
    scheme { config->get_default_layout_scheme() }
{
}

geom::Rectangle ParentContainer::get_logical_area() const
{
    if (parent.lock() == nullptr)
    {
        auto x = config->get_outer_gaps_x();
        auto y = config->get_outer_gaps_y();

        auto modified_logical_area = geom::Rectangle(
            geom::Point(logical_area.top_left.x.as_int() + x, logical_area.top_left.y.as_int() + y),
            geom::Size(logical_area.size.width.as_int() - 2 * x, logical_area.size.height.as_int() - 2 * y));

        return modified_logical_area;
    }

    return logical_area;
}

geom::Rectangle ParentContainer::get_visible_area() const
{
    return get_logical_area();
}

size_t ParentContainer::num_nodes() const
{
    return sub_nodes.size();
}

geom::Rectangle ParentContainer::create_space(int pending_index)
{
    // TODO: When making space, we should ask the currently
    //  selected window if it wants to be a lane. If it does, we
    //  will grant its request and create a new lane.

    auto placement_area = get_logical_area();
    geom::Rectangle pending_logical_rect;
    if (scheme == LayoutScheme::horizontal)
    {
        auto result = insert_node_internal(
            placement_area.size.width.as_int(),
            placement_area.top_left.x.as_int(),
            pending_index,
            sub_nodes.size(),
            [&](int index)
        { return sub_nodes[index]->get_logical_area().size.width.as_int(); },
            [&](int index, int size, int pos)
        {
            sub_nodes[index]->set_logical_area({
                geom::Point {
                             pos,
                             placement_area.top_left.y.as_int()  },
                geom::Size {
                             size,
                             placement_area.size.height.as_int() }
            });
        });
        geom::Rectangle new_node_logical_rect = {
            geom::Point {
                         result.position,
                         placement_area.top_left.y.as_int()  },
            geom::Size {
                         result.size,
                         placement_area.size.height.as_int() }
        };
        pending_logical_rect = new_node_logical_rect;
    }
    else if (scheme == LayoutScheme::vertical)
    {
        auto result = insert_node_internal(
            placement_area.size.height.as_int(),
            placement_area.top_left.y.as_int(),
            pending_index,
            sub_nodes.size(),
            [&](int index)
        { return sub_nodes[index]->get_logical_area().size.height.as_int(); },
            [&](int index, int size, int pos)
        {
            sub_nodes[index]->set_logical_area({
                geom::Point {
                             placement_area.top_left.x.as_int(),
                             pos  },
                geom::Size {
                             placement_area.size.width.as_int(),
                             size }
            });
        });
        geom::Rectangle new_node_logical_rect = {
            geom::Point {
                         placement_area.top_left.x.as_int(),
                         result.position },
            geom::Size {
                         placement_area.size.width.as_int(),
                         result.size     }
        };
        pending_logical_rect = new_node_logical_rect;
    }
    else if (scheme == LayoutScheme::tabbing || scheme == LayoutScheme::stacking)
    {
        pending_logical_rect = placement_area;
    }
    else
    {
        mir::fatal_error("Invalid scheme during create_space");
    }

    return pending_logical_rect;
}

std::shared_ptr<LeafContainer> ParentContainer::create_space_for_window(int pending_index)
{
    if (pending_index < 0)
        pending_index = num_nodes();
    pending_node = std::make_shared<LeafContainer>(
        node_interface,
        create_space(pending_index),
        config,
        tree,
        as_parent(shared_from_this()),
        state);
    sub_nodes.insert(sub_nodes.begin() + pending_index, pending_node);
    return pending_node;
}

std::shared_ptr<LeafContainer> ParentContainer::confirm_window(miral::Window const& window)
{
    if (pending_node == nullptr)
    {
        mir::fatal_error("Unable to add the window to the scene. Was create_space_for_window called?");
        return nullptr;
    }

    auto retval = pending_node;
    pending_node->associate_to_window(window);
    pending_node->set_parent(Container::as_parent(shared_from_this()));
    pending_node = nullptr;
    commit_changes();
    return retval;
}

void ParentContainer::graft_existing(std::shared_ptr<Container> const& node, int index)
{
    auto rectangle = create_space(index);
    node->set_parent(as_parent(shared_from_this()));
    node->set_logical_area(rectangle);
    sub_nodes.insert(sub_nodes.begin() + index, node);
    relayout();
    constrain();
}

std::shared_ptr<ParentContainer> ParentContainer::convert_to_parent(std::shared_ptr<Container> const& container)
{
    auto index = get_index_of_node(container);
    if (index < 0)
    {
        mir::fatal_error("Attempting to convert a node to lane with an incorrect parent");
        return nullptr;
    }

    auto new_parent_node = std::make_shared<ParentContainer>(
        node_interface,
        container->get_logical_area(),
        config,
        tree,
        Container::as_parent(shared_from_this()),
        state);
    new_parent_node->sub_nodes.push_back(container);
    container->set_parent(new_parent_node);
    sub_nodes[index] = new_parent_node;
    return new_parent_node;
}

void ParentContainer::set_logical_area(const geom::Rectangle& target_rect)
{
    // We are setting the size of the lane, but each window might have an idea of how
    // its own height relates to the lane (e.g. I take up 300px of 900px lane while my
    // neighbor takes up the remaining 600px, horizontally).
    // We need to look at the target dimension and scale everyone relative to that.
    // However, the "non-main-axis" dimension will be consistent across each node.
    auto current_logical_area = get_logical_area();
    logical_area = target_rect;
    auto target_placement_area = get_logical_area();
    std::vector<geom::Rectangle> pending_size_updates;
    pending_size_updates.reserve(sub_nodes.size());
    if (scheme == LayoutScheme::horizontal)
    {
        int total_width = 0;
        for (size_t idx = 0; idx < sub_nodes.size(); idx++)
        {
            auto item = sub_nodes[idx];
            auto item_rect = item->get_logical_area();
            double percent_width_taken = (double)item_rect.size.width.as_int() / (double)current_logical_area.size.width.as_int();
            int new_width = (int)ceil((double)target_placement_area.size.width.as_int() * percent_width_taken);

            geom::Rectangle new_item_rect;
            new_item_rect.size = geom::Size {
                geom::Width { new_width },
                target_placement_area.size.height
            };
            if (idx == 0)
            {
                new_item_rect.top_left = geom::Point {
                    target_placement_area.top_left.x,
                    target_placement_area.top_left.y
                };
            }
            else
            {
                auto const& prev_rect = pending_size_updates[idx - 1];
                new_item_rect.top_left = geom::Point {
                    geom::X { prev_rect.top_left.x.as_int() + prev_rect.size.width.as_int() },
                    target_placement_area.top_left.y
                };
            }

            pending_size_updates.push_back(new_item_rect);
            total_width += new_width;
        }

        if (!pending_size_updates.empty())
        {
            int leftover_width = target_placement_area.size.width.as_int() - total_width;
            pending_size_updates.back().size.width = geom::Width { pending_size_updates.back().size.width.as_int() + leftover_width };
        }
    }
    else if (scheme == LayoutScheme::vertical)
    {
        int total_height = 0;
        for (size_t idx = 0; idx < sub_nodes.size(); idx++)
        {
            auto item = sub_nodes[idx];
            auto item_rect = item->get_logical_area();
            double percent_height_taken = static_cast<double>(item_rect.size.height.as_int()) / current_logical_area.size.height.as_int();
            int new_height = (int)floor((double)target_placement_area.size.height.as_int() * percent_height_taken);

            geom::Rectangle new_item_rect;
            new_item_rect.size = geom::Size {
                target_placement_area.size.width,
                geom::Height { new_height },
            };
            if (idx == 0)
            {
                new_item_rect.top_left = geom::Point {
                    target_placement_area.top_left.x,
                    target_placement_area.top_left.y
                };
            }
            else
            {
                auto const& prev_rect = pending_size_updates[idx - 1];
                new_item_rect.top_left = geom::Point {
                    target_placement_area.top_left.x,
                    geom::Y { prev_rect.top_left.y.as_int() + prev_rect.size.height.as_int() },
                };
            }

            pending_size_updates.push_back(new_item_rect);
            total_height += new_height;
        }

        if (!pending_size_updates.empty())
        {
            int leftover_height = target_placement_area.size.height.as_int() - total_height;
            pending_size_updates.back().size.height = geom::Height { pending_size_updates.back().size.height.as_int() + leftover_height };
        }
    }
    else if (scheme == LayoutScheme::tabbing || scheme == LayoutScheme::stacking)
    {
        for (size_t idx = 0; idx < sub_nodes.size(); idx++)
        {
            pending_size_updates.push_back(target_placement_area);
        }
    }
    else
    {
        mir::log_error("Cannot set_logical_area with invalid scheme");
    }

    for (size_t i = 0; i < sub_nodes.size(); i++)
    {
        sub_nodes[i]->set_logical_area(pending_size_updates[i]);
    }
}

void ParentContainer::commit_changes()
{
    for (auto& node : sub_nodes)
        node->commit_changes();
}

std::shared_ptr<Container> ParentContainer::at(size_t i) const
{
    if (i >= num_nodes())
        return nullptr;

    return sub_nodes[i];
}

std::shared_ptr<LeafContainer> ParentContainer::get_nth_window(size_t i) const
{
    if (i >= sub_nodes.size())
        return nullptr;

    if (sub_nodes[i]->is_leaf())
        return as_leaf(sub_nodes[i]);

    // The lane is correct, so let's get the first window in that lane.
    return as_parent(sub_nodes[i])->get_nth_window(0);
}

std::shared_ptr<Container> ParentContainer::find_where(std::function<bool(std::shared_ptr<Container> const&)> func) const
{
    for (auto node : sub_nodes)
        if (func(node))
            return node;

    for (auto const& node : sub_nodes)
    {
        if (node->is_lane())
        {
            if (auto retval = as_parent(node)->find_where(func))
                return retval;
        }
    }

    return nullptr;
}

const std::vector<std::shared_ptr<Container>>& ParentContainer::get_sub_nodes() const
{
    return sub_nodes;
}

void ParentContainer::swap_nodes(std::shared_ptr<Container> const& first, std::shared_ptr<Container> const& second)
{
    auto first_index = get_index_of_node(first);
    auto second_index = get_index_of_node(second);
    sub_nodes[second_index] = first;
    sub_nodes[first_index] = second;
    relayout();
    constrain();
}

void ParentContainer::remove(const std::shared_ptr<Container>& node)
{
    sub_nodes.erase(
        std::remove_if(sub_nodes.begin(), sub_nodes.end(), [&](std::shared_ptr<Container> const& content)
    {
        return content == node;
    }),
        sub_nodes.end());

    // If we have one child AND it is a lane, THEN we can absorb all of it's children
    if (sub_nodes.size() == 1 && sub_nodes[0]->is_lane())
    {
        auto dying_lane = as_parent(sub_nodes[0]);
        sub_nodes.clear();
        for (auto const& sub_node : dying_lane->get_sub_nodes())
        {
            sub_nodes.push_back(sub_node);
            sub_node->set_parent(as_parent(shared_from_this()));
        }
        set_layout(dying_lane->get_direction());
    }

    relayout();
}

int ParentContainer::get_index_of_node(miracle::Container const* node) const
{
    for (int i = 0; i < sub_nodes.size(); i++)
        if (sub_nodes[i].get() == node)
            return i;

    return -1;
}

int ParentContainer::get_index_of_node(std::shared_ptr<Container> const& node) const
{
    return get_index_of_node(node.get());
}

int ParentContainer::get_index_of_node(Container const& node) const
{
    return get_index_of_node(node.shared_from_this().get());
}

void ParentContainer::constrain()
{
    for (auto& node : sub_nodes)
        node->constrain();
}

size_t ParentContainer::get_min_width() const
{
    size_t size = 0;
    for (auto const& node : sub_nodes)
        size += node->get_min_width();
    return size;
}

size_t ParentContainer::get_min_height() const
{
    size_t size = 0;
    for (auto const& node : sub_nodes)
        size += node->get_min_height();
    return size;
}

std::weak_ptr<ParentContainer> ParentContainer::get_parent() const
{
    return parent;
}

void ParentContainer::set_parent(std::shared_ptr<ParentContainer> const& in_parent)
{
    parent = in_parent;
}

void ParentContainer::relayout()
{
    auto placement_area = get_logical_area();
    if (scheme == LayoutScheme::horizontal)
    {
        int total_width = 0;
        for (auto const& node : sub_nodes)
        {
            total_width += node->get_logical_area().size.width.as_int();
        }

        int diff_width = placement_area.size.width.as_value() - total_width;
        int diff_per_node = floor((double)diff_width / (double)sub_nodes.size());
        for (auto const& node : sub_nodes)
        {
            auto rectangle = node->get_logical_area();
            rectangle.size.width = geom::Width { rectangle.size.width.as_int() + diff_per_node };
            rectangle.size.height = geom::Height { placement_area.size.height };
            node->set_logical_area(rectangle);
        }
    }
    else if (scheme == LayoutScheme::vertical)
    {
        int total_height = 0;
        for (auto const& node : sub_nodes)
        {
            total_height += node->get_logical_area().size.height.as_int();
        }

        int diff_width = placement_area.size.height.as_value() - total_height;
        int diff_per_node = floor((double)diff_width / (double)sub_nodes.size());
        for (auto const& node : sub_nodes)
        {
            auto rectangle = node->get_logical_area();
            rectangle.size.width = geom::Width { placement_area.size.width };
            rectangle.size.height = geom::Height { rectangle.size.height.as_int() + diff_per_node };
            node->set_logical_area(rectangle);
        }
    }
    else if (scheme == LayoutScheme::tabbing || scheme == LayoutScheme::stacking)
    {
        for (auto const& node : sub_nodes)
            node->set_logical_area(placement_area);
    }
    else
    {
        mir::log_error("Invalid scheme during relayout");
    }

    // Note that it is important to use the logical_area here instead of the placement area
    set_logical_area(logical_area);
}

void ParentContainer::handle_ready()
{
}

void ParentContainer::handle_modify(miral::WindowSpecification const& specification)
{
}

void ParentContainer::handle_request_move(MirInputEvent const* input_event)
{
}

void ParentContainer::handle_request_resize(MirInputEvent const* input_event, MirResizeEdge edge)
{
}

void ParentContainer::handle_raise()
{
}

bool ParentContainer::resize(Direction direction, int pixels)
{
    return false;
}

bool ParentContainer::set_size(std::optional<int> const& width, std::optional<int> const& height)
{
    return false;
}

bool ParentContainer::toggle_fullscreen()
{
    return false;
}

void ParentContainer::request_horizontal_layout()
{
    scheme = LayoutScheme::horizontal;
    relayout();
}

void ParentContainer::request_vertical_layout()
{
    scheme = LayoutScheme::vertical;
    relayout();
}

void ParentContainer::toggle_layout(bool cycle_thru_all)
{
    if (cycle_thru_all)
        scheme = get_next_layout(scheme);
    else
    {
        if (scheme == LayoutScheme::vertical)
            scheme == LayoutScheme::horizontal;
        else if (scheme == LayoutScheme::horizontal)
            scheme = LayoutScheme::vertical;
    }

    relayout();
}

void ParentContainer::set_tree(TilingWindowTree* tree_)
{
    tree = tree_;
}

void ParentContainer::on_focus_gained()
{
    if (scheme == LayoutScheme::tabbing || scheme == LayoutScheme::stacking)
    {
        for (auto const& container : sub_nodes)
        {
            if (container != state.focused_container() && container->window())
                node_interface.send_to_back(container->window().value());
        }
    }
}

void ParentContainer::on_focus_lost()
{
}

void ParentContainer::on_move_to(mir::geometry::Point const& top_left)
{
}

mir::geometry::Rectangle
ParentContainer::confirm_placement(MirWindowState state, mir::geometry::Rectangle const& rectangle)
{
    return rectangle;
}

ContainerType ParentContainer::get_type() const
{
    return ContainerType::parent;
}

void ParentContainer::show()
{
    for (auto const& c : sub_nodes)
        c->show();
}

void ParentContainer::hide()
{
    for (auto const& c : sub_nodes)
        c->hide();
}

void ParentContainer::on_open()
{
}

Workspace* ParentContainer::get_workspace() const
{
    return tree->get_workspace();
}

Output* ParentContainer::get_output() const
{
    return tree->get_workspace()->get_output();
}

glm::mat4 ParentContainer::get_transform() const
{
    return glm::mat4(1.f);
}

void ParentContainer::set_transform(glm::mat4 transform)
{
}

glm::mat4 ParentContainer::get_workspace_transform() const
{
    return glm::mat4(1.f);
}

glm::mat4 ParentContainer::get_output_transform() const
{
    return glm::mat4(1.f);
}

uint32_t ParentContainer::animation_handle() const
{
    return 0;
}

void ParentContainer::animation_handle(uint32_t uint_32)
{
}

bool ParentContainer::is_focused() const
{
    return false;
}

std::optional<miral::Window> ParentContainer::window() const
{
    return std::optional<miral::Window>();
}

bool ParentContainer::select_next(miracle::Direction)
{
    return false;
}

bool ParentContainer::pinned(bool)
{
    return false;
}

bool ParentContainer::pinned() const
{
    return false;
}

bool ParentContainer::move(Direction direction)
{
    return false;
}

bool ParentContainer::move_by(Direction direction, int pixels)
{
    return false;
}

bool ParentContainer::move_to(int x, int y)
{
    return false;
}

bool ParentContainer::is_fullscreen() const
{
    return false;
}

bool ParentContainer::toggle_tabbing()
{
    if (scheme == LayoutScheme::tabbing)
        scheme = LayoutScheme::horizontal;
    else
        scheme = LayoutScheme::tabbing;

    relayout();
    return true;
}

bool ParentContainer::toggle_stacking()
{
    if (scheme == LayoutScheme::stacking)
        scheme = LayoutScheme::horizontal;
    else
        scheme = LayoutScheme::stacking;

    relayout();
    return true;
}

bool ParentContainer::set_layout(LayoutScheme new_scheme)
{
    scheme = new_scheme;
    relayout();
    constrain();
    commit_changes();
    return true;
}

LayoutScheme ParentContainer::get_layout() const
{
    return scheme;
}

nlohmann::json ParentContainer::to_json() const
{
    auto const visible_area = get_visible_area();
    auto const logical_area = get_logical_area();
    nlohmann::json containers_json;
    for (auto const& container : sub_nodes)

        containers_json.push_back(container->to_json());
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

    auto const id = reinterpret_cast<std::uintptr_t>(this);
    return {
        { "id",                   id                                                                                                                                                                                                                                                 },
        { "name",                 "Parent #" + std::to_string(id)                                                                                                                                                                                                                    },
        { "rect",                 {
                      { "x", logical_area.top_left.x.as_int() },
                      { "y", logical_area.top_left.y.as_int() },
                      { "width", logical_area.size.width.as_int() },
                      { "height", logical_area.size.height.as_int() },
                  }                                                                                                                                                                                                                                                 },
        { "focused",              visible && is_focused()                                                                                                                                                                                                                            },
        { "focus",                std::vector<int>()                                                                                                                                                                                                                                 },
        { "border",               "none"                                                                                                                                                                                                                                             },
        { "current_border_width", 0                                                                                                                                                                                                                                                  },
        { "layout",               to_string(scheme)                                                                                                                                                                                                                                  },
        { "orientation",          "none"                                                                                                                                                                                                                                             },
        { "percent",              get_percent_of_parent()                                                                                                                                                                                                                            },
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
                       }                                                                                                                                                                                                                                       },
        { "geometry",             {
                          { "x", 0 },
                          { "y", 0 },
                          { "width", logical_area.size.width.as_int() },
                          { "height", logical_area.size.height.as_int() },
                      }                                                                                                                                                                                                                                         },
        { "window",               0                                                                                                                                                                                                                                                  }, // TODO
        { "urgent",               false                                                                                                                                                                                                                                              },
        { "floating_nodes",       std::vector<int>()                                                                                                                                                                                                                                 },
        { "sticky",               false                                                                                                                                                                                                                                              },
        { "type",                 "con"                                                                                                                                                                                                                                              },
        { "fullscreen_mode",      is_fullscreen() ? 1 : 0                                                                                                                                                                                                                            }, // TODO: Support value 2
        { "visible",              visible                                                                                                                                                                                                                                            },
        { "shell",                "miracle-wm"                                                                                                                                                                                                                                       }, // TODO
        { "inhibit_idle",         false                                                                                                                                                                                                                                              },
        { "idle_inhibitors",      {}                                                                                                                                                                                                                                                 },
        { "window_properties",    {}                                                                                                                                                                                                                                                 }, // TODO
        { "nodes",                containers_json                                                                                                                                                                                                                                    }
    };
}
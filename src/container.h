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

#ifndef MIRACLE_CONTAINER_H
#define MIRACLE_CONTAINER_H

#include "direction.h"
#include "layout_scheme.h"
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <mir/geometry/rectangle.h>
#include <mir_toolkit/event.h>
#include <miral/window.h>
#include <miral/window_manager_tools.h>
#include <nlohmann/json.hpp>
#include <vector>

namespace geom = mir::geometry;

namespace miracle
{
class MiracleConfig;
class TilingWindowTree;
class LeafContainer;
class ParentContainer;
class FloatingWindowContainer;
class ContainerGroupContainer;
class Workspace;
class Output;

enum class ContainerType
{
    none,
    leaf,
    floating_window,
    floating_tree,
    shell,
    parent,
    group,
    stack
};

ContainerType container_type_from_string(std::string const& str);

/// Aligns with i3's concept of containers. A [Container] may map to
/// an individual [miral::Window] or it may not. You can think of a [Container]
/// as a logical rectangle on a [Workspace] upon which you can perform some
/// actions. Depending on the type of [Container], particular actions may
/// result in noops.
class Container : public std::enable_shared_from_this<Container>
{
public:
    virtual ContainerType get_type() const = 0;

    virtual void show() = 0;
    virtual void hide() = 0;

    /// Commits any changes made to this node to the screen. This must
    /// be call for changes to be pushed to the scene. Additionally,
    /// it is advised that this method is only called once all changes have
    /// been made for a particular operation.
    virtual void commit_changes() = 0;

    [[nodiscard]] virtual geom::Rectangle get_logical_area() const = 0;
    virtual void set_logical_area(geom::Rectangle const&) = 0;
    [[nodiscard]] virtual geom::Rectangle get_visible_area() const = 0;
    virtual void constrain() = 0;
    virtual std::weak_ptr<ParentContainer> get_parent() const = 0;
    virtual void set_parent(std::shared_ptr<ParentContainer> const&) = 0;
    virtual size_t get_min_height() const = 0;
    virtual size_t get_min_width() const = 0;
    virtual void handle_ready() = 0;
    virtual void handle_modify(miral::WindowSpecification const&) = 0;
    virtual void handle_request_move(MirInputEvent const* input_event) = 0;
    virtual void handle_request_resize(MirInputEvent const* input_event, MirResizeEdge edge) = 0;
    virtual void handle_raise() = 0;
    virtual bool resize(Direction direction) = 0;
    virtual bool toggle_fullscreen() = 0;
    virtual void request_horizontal_layout() = 0;
    virtual void request_vertical_layout() = 0;
    virtual void toggle_layout(bool cycle_thru_all) = 0;
    virtual void on_open() = 0;
    virtual void on_focus_gained() = 0;
    virtual void on_focus_lost() = 0;
    virtual void on_move_to(geom::Point const& top_left) = 0;
    virtual mir::geometry::Rectangle confirm_placement(
        MirWindowState, mir::geometry::Rectangle const&)
        = 0;
    virtual Workspace* get_workspace() const = 0;
    virtual Output* get_output() const = 0;
    virtual glm::mat4 get_transform() const = 0;
    virtual void set_transform(glm::mat4 transform) = 0;
    virtual glm::mat4 get_workspace_transform() const;
    virtual glm::mat4 get_output_transform() const;
    virtual uint32_t animation_handle() const = 0;
    virtual void animation_handle(uint32_t) = 0;
    virtual bool is_focused() const = 0;
    virtual bool is_fullscreen() const = 0;
    virtual std::optional<miral::Window> window() const = 0;
    virtual bool select_next(Direction) = 0;
    virtual bool pinned() const = 0;
    virtual bool pinned(bool) = 0;
    virtual bool move(Direction) = 0;
    virtual bool move_by(Direction, int pixels) = 0;
    virtual bool move_to(int x, int y) = 0;
    virtual bool toggle_tabbing() = 0;
    virtual bool toggle_stacking() = 0;
    virtual bool set_layout(LayoutScheme scheme) = 0;
    virtual LayoutScheme get_layout() const = 0;
    virtual nlohmann::json to_json() const = 0;

    bool is_leaf();
    bool is_lane();
    [[nodiscard]] float get_percent_of_parent() const;

    static std::shared_ptr<LeafContainer> as_leaf(std::shared_ptr<Container> const&);
    static std::shared_ptr<ParentContainer> as_parent(std::shared_ptr<Container> const&);
    static std::shared_ptr<FloatingWindowContainer> as_floating(std::shared_ptr<Container> const&);
    static std::shared_ptr<ContainerGroupContainer> as_group(std::shared_ptr<Container> const&);

protected:
    [[nodiscard]] std::array<bool, (size_t)Direction::MAX> get_neighbors() const;
};
}

#endif // MIRACLE_CONTAINER_H

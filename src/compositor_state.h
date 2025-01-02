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

#ifndef MIRACLE_WM_COMPOSITOR_STATE_H
#define MIRACLE_WM_COMPOSITOR_STATE_H

#include "container.h"

#include <algorithm>
#include <memory>
#include <mir/geometry/point.h>
#include <vector>

namespace miracle
{
class Output;

enum class WindowManagerMode
{
    normal = 0,

    /// While [resizing], only the window that was selected during
    /// resize can be selected. If that window closes, resize
    /// is completed.
    resizing,

    /// While [selecting], only [Container]s selected with the multi-select
    /// keybind/mousebind can be selected or deselected.
    selecting
};

class CompositorState
{
public:
    WindowManagerMode mode = WindowManagerMode::normal;
    mir::geometry::Point cursor_position;
    uint32_t modifiers = 0;
    bool has_clicked_floating_window = false;
    std::vector<std::shared_ptr<Output>> output_list;

    [[nodiscard]] std::shared_ptr<Container> focused_container() const;
    [[nodiscard]] std::shared_ptr<Output> focused_output() const;

    /// Focuses the provided container. If [is_anonymous] is true, the container
    /// will be focused even if it does not exist in the list.
    void focus_container(std::shared_ptr<Container> const&, bool is_anonymous = false);
    void unfocus_container(std::shared_ptr<Container> const& container);
    void focus_output(std::shared_ptr<Output> const&);
    void unfocus_output(std::shared_ptr<Output> const&);
    void add(std::shared_ptr<Container> const& container);
    void remove(std::shared_ptr<Container> const& container);
    [[nodiscard]] std::shared_ptr<Container> get_first_with_type(ContainerType type) const;
    [[nodiscard]] std::vector<std::weak_ptr<Container>> const& containers() const { return focus_order; }

private:
    std::weak_ptr<Container> focused;
    std::vector<std::weak_ptr<Container>> focus_order;
    std::weak_ptr<Output> output;
};
}

#endif // MIRACLE_WM_COMPOSITOR_STATE_H

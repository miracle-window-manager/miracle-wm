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

#include "floating_tree_container.h"
#include "tiling_window_tree.h"
#include "workspace.h"

namespace
{
class FloatingTreeTilingWindowTreeInterface : public miracle::TilingWindowTreeInterface
{
public:
    explicit FloatingTreeTilingWindowTreeInterface(miracle::Workspace* workspace) :
        workspace { workspace }
    {
    }

    std::vector<miral::Zone> const& get_zones() override
    {
        return zones;
    }

    miracle::Workspace* get_workspace() const override
    {
        return workspace;
    }

private:
    miracle::Workspace* workspace;
    std::vector<miral::Zone> zones;
};
}

namespace miracle
{
FloatingTreeContainer::FloatingTreeContainer(
    Workspace* workspace,
    WindowController& window_controller,
    CompositorState const& compositor_state,
    std::shared_ptr<MiracleConfig> const& config) :
    tree {
        std::make_unique<TilingWindowTree>(
            std::make_unique<FloatingTreeTilingWindowTreeInterface>(workspace),
            window_controller,
            compositor_state,
            config,
            geom::Rectangle { geom::Point { 100, 100 }, geom::Size { 640, 480 } }
             )
},
    workspace_ { workspace }
{
}

ContainerType FloatingTreeContainer::get_type() const
{
    return ContainerType::floating_tree;
}

void FloatingTreeContainer::show()
{
    tree->show();
}

void FloatingTreeContainer::hide()
{
    tree->hide();
}

void FloatingTreeContainer::commit_changes()
{
}

mir::geometry::Rectangle FloatingTreeContainer::get_logical_area() const
{
    return tree->get_area();
}

void FloatingTreeContainer::set_logical_area(mir::geometry::Rectangle const& rectangle)
{
    tree->set_area(rectangle);
}

mir::geometry::Rectangle FloatingTreeContainer::get_visible_area() const
{
    return get_logical_area();
}

void FloatingTreeContainer::constrain()
{
}

std::weak_ptr<ParentContainer> FloatingTreeContainer::get_parent() const
{
    return {};
}

void FloatingTreeContainer::set_parent(std::shared_ptr<ParentContainer> const& ptr)
{
    throw std::logic_error("FloatingTreeContainer::set_parent: invalid operation");
}

size_t FloatingTreeContainer::get_min_height() const
{
    return 0;
}

size_t FloatingTreeContainer::get_min_width() const
{
    return 0;
}

void FloatingTreeContainer::handle_ready()
{
}

void FloatingTreeContainer::handle_modify(miral::WindowSpecification const& specification)
{
}

void FloatingTreeContainer::handle_request_move(MirInputEvent const* input_event)
{
    // TODO
}

void FloatingTreeContainer::handle_request_resize(MirInputEvent const* input_event, MirResizeEdge edge)
{
    // TODO
}

void FloatingTreeContainer::handle_raise()
{
}

bool FloatingTreeContainer::resize(Direction direction)
{
    // TODO
    return false;
}

bool FloatingTreeContainer::toggle_fullscreen()
{
    // TODO:
    return false;
}

void FloatingTreeContainer::request_horizontal_layout()
{
}

void FloatingTreeContainer::request_vertical_layout()
{
}

void FloatingTreeContainer::toggle_layout(bool)
{
}

void FloatingTreeContainer::on_open()
{
}

void FloatingTreeContainer::on_focus_gained()
{
}

void FloatingTreeContainer::on_focus_lost()
{
}

void FloatingTreeContainer::on_move_to(mir::geometry::Point const& top_left)
{
    auto area = tree->get_area();
    area.top_left = top_left;
    tree->set_area(area);
}

mir::geometry::Rectangle
FloatingTreeContainer::confirm_placement(MirWindowState state, mir::geometry::Rectangle const& rectangle)
{
    return rectangle;
}

Workspace* FloatingTreeContainer::get_workspace() const
{
    return workspace_;
}

Output* FloatingTreeContainer::get_output() const
{
    return workspace_->get_output();
}

glm::mat4 FloatingTreeContainer::get_transform() const
{
    return glm::mat4(1.f);
}

void FloatingTreeContainer::set_transform(glm::mat4 transform)
{
}

glm::mat4 FloatingTreeContainer::get_workspace_transform() const
{
    return glm::mat4(1.f);
}

glm::mat4 FloatingTreeContainer::get_output_transform() const
{
    return glm::mat4(1.f);
}

uint32_t FloatingTreeContainer::animation_handle() const
{
    return 0;
}

void FloatingTreeContainer::animation_handle(uint32_t uint_32)
{
}

bool FloatingTreeContainer::is_focused() const
{
    // TODO:
    return false;
}

bool FloatingTreeContainer::is_fullscreen() const
{
    // TODO:
    return false;
}

std::optional<miral::Window> FloatingTreeContainer::window() const
{
    return {};
}

bool FloatingTreeContainer::select_next(Direction direction)
{
    return false;
}

bool FloatingTreeContainer::pinned() const
{
    // TODO:
    return false;
}

bool FloatingTreeContainer::pinned(bool b)
{
    // TODO:
    return false;
}

bool FloatingTreeContainer::move(Direction direction)
{
    // TODO:
    return false;
}

bool FloatingTreeContainer::move_by(Direction direction, int pixels)
{
    // TODO:
    return false;
}

bool FloatingTreeContainer::move_to(int x, int y)
{
    auto area = tree->get_area();
    area.top_left = geom::Point { x, y };
    tree->set_area(area);
    return true;
}
} // miracle
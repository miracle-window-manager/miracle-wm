#ifndef WINDOW_TREE_H
#define WINDOW_TREE_H

#include "node.h"
#include <memory>
#include <vector>
#include <miral/window.h>
#include <miral/window_specification.h>
#include <mir/geometry/rectangle.h>
#include <mir/geometry/rectangle.h>
#include <miral/window_manager_tools.h>
#include <miral/zone.h>

namespace geom = mir::geometry;

namespace miracle
{

enum class Direction
{
    up,
    left,
    down,
    right
};

struct WindowTreeOptions
{
    int gap_x;
    int gap_y;
};

/// Represents a tiling tree for an output.
class WindowTree
{
public:
    WindowTree(geom::Rectangle area, miral::WindowManagerTools const& tools, WindowTreeOptions const& options);
    ~WindowTree() = default;

    /// Makes space for the new window and returns its specified spot in the world.
    miral::WindowSpecification allocate_position(const miral::WindowSpecification &requested_specification);

    void advise_new_window(miral::WindowInfo const&);

    /// Places us into resize mode. Other operations are prohibited while we are in resize mode.
    void toggle_resize_mode();

    /// Try to resize the current active window in the provided direction
    bool try_resize_active_window(Direction direction);

    /// Move the active window in the provided direction
    bool try_move_active_window(Direction direction);

    /// Select the next window in the provided direction
    bool try_select_next(Direction direction);

    // Request a change to vertical window placement
    void request_vertical();

    // Request a change to horizontal window placement
    void request_horizontal();

    /// Advises us to focus the provided window.
    void advise_focus_gained(miral::Window&);

    /// Advises us to lose focus on the provided window.
    void advise_focus_lost(miral::Window&);

    /// Called when the window was deleted.
    void advise_delete_window(miral::Window&);

    void advise_resize(miral::WindowInfo const&, geom::Size const&);

    /// Called when the physical display is resized.
    void set_output_area(geom::Rectangle new_area);

    bool point_is_in_output(int x, int y);

    bool select_window_from_point(int x, int y);

    void advise_application_zone_create(miral::Zone const& application_zone);
    void advise_application_zone_update(miral::Zone const& updated, miral::Zone const& original);
    void advise_application_zone_delete(miral::Zone const& application_zone);

private:
    miral::WindowManagerTools tools;
    WindowTreeOptions options;
    std::shared_ptr<Node> root_lane;
    std::shared_ptr<Node> active_window;
    geom::Rectangle area;
    bool is_resizing = false;
    std::vector<miral::Zone> application_zone_list;

    std::shared_ptr<Node> get_active_lane();
    void handle_direction_request(NodeLayoutDirection direction);
    void resize_node_in_direction(std::shared_ptr<Node> node, Direction direction, int amount);
    /// From the provided node, find the next node in the provided direction.
    /// This method is guaranteed to return a Window node, not a Lane.
    std::shared_ptr<Node> traverse(std::shared_ptr<Node> from, Direction direction);
    void recalculate_root_node_area();
};

}


#endif //MIRCOMPOSITOR_WINDOW_TREE_H

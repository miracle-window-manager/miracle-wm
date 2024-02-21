#ifndef WORKSPACE_MANAGER_H
#define WORKSPACE_MANAGER_H

#include "workspace_observer.h"

#include <memory>
#include <vector>
#include <miral/window_manager_tools.h>
#include <list>
#include <map>
#include <functional>

namespace miracle
{

using miral::Window;
using miral::WindowInfo;
using miral::WindowManagerTools;
using miral::WindowSpecification;
using miral::Workspace;

class Screen;

class WorkspaceManager
{
public:
    explicit WorkspaceManager(
        WindowManagerTools const& tools,
        WorkspaceObserverRegistrar& registry,
        std::function<std::shared_ptr<Screen> const()> const& get_active_screen);
    virtual ~WorkspaceManager() = default;

    /// Request the workspace. If it does not yet exist, then one
    /// is created on the current Screen. If it does exist, we navigate
    /// to the screen containing that workspace and show it if it
    /// isn't already shown.
    std::shared_ptr<Screen> request_workspace(std::shared_ptr<Screen> screen, int workspace);

    bool request_first_available_workspace(std::shared_ptr<Screen> screen);

    bool move_active_to_workspace(std::shared_ptr<Screen> screen, int workspace);

    bool delete_workspace(int workspace);

    void request_focus(int workspace);

    static int constexpr NUM_WORKSPACES = 10;
    std::array<std::shared_ptr<Screen>, NUM_WORKSPACES> const& get_workspaces() { return workspaces; }
private:
    WindowManagerTools tools_;
    WorkspaceObserverRegistrar& registry;
    std::function<std::shared_ptr<Screen> const()> get_active_screen;
    std::array<std::shared_ptr<Screen>, NUM_WORKSPACES> workspaces;

    std::shared_ptr<Screen> const& get_workspace(int key);
};
}

#endif

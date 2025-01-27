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

#ifndef MIRACLE_WM_MIRAL_OUTPUT_FACTORY_H
#define MIRACLE_WM_MIRAL_OUTPUT_FACTORY_H

#include "output_factory.h"

namespace miracle
{
class WorkspaceManager;
class MinimalWindowManager;
class CompositorState;
class Config;
class WindowController;
class Animator;

class MiralOutputFactory : public OutputFactory
{
public:
    MiralOutputFactory(
        std::shared_ptr<MinimalWindowManager> const& floating_window_manager,
        CompositorState& state,
        std::shared_ptr<Config> const& options,
        WindowController&,
        Animator&);
    std::unique_ptr<Output> create(
        std::string name,
        int id,
        mir::geometry::Rectangle area,
        OutputManager* output_manager) override;

private:
    std::shared_ptr<MinimalWindowManager> floating_window_manager;
    CompositorState& state;
    std::shared_ptr<Config> config;
    WindowController& window_controller;
    Animator& animator;
};

}

#endif // MIRACLE_WM_MIRAL_OUTPUT_FACTORY_H

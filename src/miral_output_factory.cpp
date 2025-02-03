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

#include "miral_output_factory.h"
#include "miral_output.h"

using namespace miracle;

MiralOutputFactory::MiralOutputFactory(
    CompositorState& state,
    std::shared_ptr<Config> const& config,
    WindowController& node_interface,
    Animator& animator) :
    state { state },
    config { config },
    window_controller { node_interface },
    animator { animator }
{
}

std::unique_ptr<Output> MiralOutputFactory::create(
    std::string name, int id, mir::geometry::Rectangle area, OutputManager* output_manager)
{
    return std::make_unique<MiralWrapperOutput>(
        std::move(name),
        id,
        area,
        state,
        output_manager,
        config,
        window_controller,
        animator);
}
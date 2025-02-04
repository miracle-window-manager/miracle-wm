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

#include "output_factory.h"
#include "output.h"

using namespace miracle;

MiralOutputFactory::MiralOutputFactory(
    std::shared_ptr<CompositorState> const& state,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<Animator> const& animator) :
    state { state },
    config { config },
    window_controller { window_controller },
    animator { animator }
{
}

std::unique_ptr<OutputInterface> MiralOutputFactory::create(
    std::string name, int id, mir::geometry::Rectangle area)
{
    return std::make_unique<Output>(
        std::move(name),
        id,
        area,
        state,
        config,
        window_controller,
        animator);
}
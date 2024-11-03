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

#include "animation_defintion.h"
#define MIR_LOG_COMPONENT "animation_definition"
#include <mir/log.h>

using namespace miracle;

std::optional<AnimateableEvent> miracle::from_string_animateable_event(std::string const& str)
{
    if (str == "window_open")
        return AnimateableEvent::window_open;
    else if (str == "window_move")
        return AnimateableEvent::window_move;
    else if (str == "window_close")
        return AnimateableEvent::window_close;
    else if (str == "workspace_switch")
        return AnimateableEvent::workspace_switch;
    else
        return std::nullopt;
}

std::optional<EaseFunction> miracle::from_string_ease_function(std::string const& str)
{
    if (str == "linear")
        return EaseFunction::linear;
    else if (str == "ease_in_sine")
        return EaseFunction::ease_in_sine;
    else if (str == "ease_out_sine")
        return EaseFunction::ease_out_sine;
    else if (str == "ease_in_out_sine")
        return EaseFunction::ease_in_out_sine;
    else if (str == "ease_in_quad")
        return EaseFunction::ease_in_quad;
    else if (str == "ease_out_quad")
        return EaseFunction::ease_out_quad;
    else if (str == "ease_in_out_quad")
        return EaseFunction::ease_in_out_quad;
    else if (str == "ease_in_cubic")
        return EaseFunction::ease_in_cubic;
    else if (str == "ease_out_cubic")
        return EaseFunction::ease_out_cubic;
    else if (str == "ease_in_out_cubic")
        return EaseFunction::ease_in_out_cubic;
    else if (str == "ease_in_quart")
        return EaseFunction::ease_in_quart;
    else if (str == "ease_out_quart")
        return EaseFunction::ease_out_quart;
    else if (str == "ease_in_out_quart")
        return EaseFunction::ease_in_out_quart;
    else if (str == "ease_in_quint")
        return EaseFunction::ease_in_quint;
    else if (str == "ease_out_quint")
        return EaseFunction::ease_out_quint;
    else if (str == "ease_in_out_quint")
        return EaseFunction::ease_in_out_quint;
    else if (str == "ease_in_expo")
        return EaseFunction::ease_in_expo;
    else if (str == "ease_out_expo")
        return EaseFunction::ease_out_expo;
    else if (str == "ease_in_out_expo")
        return EaseFunction::ease_in_out_expo;
    else if (str == "ease_in_circ")
        return EaseFunction::ease_in_circ;
    else if (str == "ease_out_circ")
        return EaseFunction::ease_out_circ;
    else if (str == "ease_in_out_circ")
        return EaseFunction::ease_in_out_circ;
    else if (str == "ease_in_back")
        return EaseFunction::ease_in_back;
    else if (str == "ease_out_back")
        return EaseFunction::ease_out_back;
    else if (str == "ease_in_out_back")
        return EaseFunction::ease_in_out_back;
    else if (str == "ease_in_elastic")
        return EaseFunction::ease_in_elastic;
    else if (str == "ease_out_elastic")
        return EaseFunction::ease_out_elastic;
    else if (str == "ease_in_out_elastic")
        return EaseFunction::ease_in_out_elastic;
    else if (str == "ease_in_bounce")
        return EaseFunction::ease_in_bounce;
    else if (str == "ease_out_bounce")
        return EaseFunction::ease_out_bounce;
    else if (str == "ease_in_out_bounce")
        return EaseFunction::ease_in_out_bounce;
    else
        return std::nullopt;
}

std::optional<AnimationType> miracle::from_string_animation_type(std::string const& str)
{
    if (str == "disabled")
        return AnimationType::disabled;
    else if (str == "slide")
        return AnimationType::slide;
    else if (str == "grow")
        return AnimationType::grow;
    else if (str == "shrink")
        return AnimationType::shrink;
    else
        return std::nullopt;
}

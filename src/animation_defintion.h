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

#ifndef MIRACLE_WM_ANIMATION_DEFINTION_H
#define MIRACLE_WM_ANIMATION_DEFINTION_H

#include "mir/geometry/point.h"
#include <optional>
#include <string>

namespace miracle
{
/// Defines an event that can be animated.
enum class AnimateableEvent
{
    window_open,
    window_move,
    window_close,
    workspace_switch,
    max
};

/// Defines the ease function for an event
enum class EaseFunction
{
    linear,
    ease_in_sine,
    ease_out_sine,
    ease_in_out_sine,
    ease_in_quad,
    ease_out_quad,
    ease_in_out_quad,
    ease_in_cubic,
    ease_out_cubic,
    ease_in_out_cubic,
    ease_in_quart,
    ease_out_quart,
    ease_in_out_quart,
    ease_in_quint,
    ease_out_quint,
    ease_in_out_quint,
    ease_in_expo,
    ease_out_expo,
    ease_in_out_expo,
    ease_in_circ,
    ease_out_circ,
    ease_in_out_circ,
    ease_in_back,
    ease_out_back,
    ease_in_out_back,
    ease_in_elastic,
    ease_out_elastic,
    ease_in_out_elastic,
    ease_in_bounce,
    ease_out_bounce,
    ease_in_out_bounce,
    max
};

enum class AnimationType
{
    disabled,
    slide,
    grow,
    shrink,
    fade_in,
    fade_out,
    max
};

/// Defines an animation that the Animator can use.
/// Each animations is mapped to a single AnimateableEvent.
struct AnimationDefinition
{
    AnimationType type = AnimationType::max;
    EaseFunction function = EaseFunction::linear;
    float duration_seconds = 1.f;

    // Easing function values
    float c1 = 1.2;
    float c2 = 1.83;
    float c3 = 2.2;
    float c4 = 2.0943951023931953;
    float c5 = 1.3962634015954636;
    float n1 = 7.5625;
    float d1 = 2.75;
};

AnimateableEvent from_string_animateable_event(std::string const&);
EaseFunction from_string_ease_function(std::string const&);
AnimationType from_string_animation_type(std::string const&);

}

#endif // MIRACLE_WM_ANIMATION_DEFINTION_H

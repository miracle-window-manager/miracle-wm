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

#ifndef MIRACLEWM_ANIMATOR_H
#define MIRACLEWM_ANIMATOR_H

#include "animation_defintion.h"
#include <condition_variable>
#include <functional>
#include <glm/glm.hpp>
#include <mir/geometry/rectangle.h>
#include <mutex>
#include <optional>
#include <thread>

namespace mir
{
class ServerActionQueue;
}

namespace miracle
{
/// Unique handle provided to track animators
typedef uint32_t AnimationHandle;

/// Reserved for windows who lack an animation handle
extern const AnimationHandle none_animation_handle;

/// Callback data provided to the caller on each tick.
struct AnimationStepResult
{
    /// The handle of the animation to which this result matches
    AnimationHandle handle = none_animation_handle;

    /// Whether or not this result marks the end of animation.
    bool is_complete = false;

    /// The clip area that should be applied to this transformation
    mir::geometry::Rectangle clip_area;
    std::optional<glm::vec2> position;
    std::optional<glm::vec2> size;
    std::optional<glm::mat4> transform;
};

class Animation
{
public:
    Animation(
        AnimationHandle handle,
        AnimationDefinition definition,
        mir::geometry::Rectangle const& from,
        mir::geometry::Rectangle const& to,
        mir::geometry::Rectangle const& current);

    Animation& operator=(Animation const& other) = default;

    AnimationStepResult init();
    AnimationStepResult step(float const dt);
    [[nodiscard]] AnimationHandle get_handle() const { return handle; }
    float get_runtime_seconds() const { return runtime_seconds; }
    void set_current_size(mir::geometry::Size const& size);
    void mark_for_great_animator_in_the_sky();
    bool is_going_to_great_animator_in_the_sky() const;
    virtual void on_tick(AnimationStepResult const&) = 0;

private:
    AnimationHandle handle;
    AnimationDefinition definition;
    mir::geometry::Rectangle clip_area;
    mir::geometry::Rectangle from;
    mir::geometry::Rectangle to;
    mir::geometry::Size real_size;
    float runtime_seconds = 0.f;
    bool should_leave_this_animator_for_the_great_animator_in_the_sky = false;
};

/// Manages the animation queue. If multiple animations are queued for a window,
/// then the latest animation may override values from previous animations.
class Animator
{
public:
    /// Animateable components must register with the Animator before being
    /// able to be animated.
    AnimationHandle register_animateable();

    void tick(float dt);

    void append(std::shared_ptr<Animation> const&);
    void set_size_hack(AnimationHandle handle, mir::geometry::Size const& size);
    void remove_by_animation_handle(AnimationHandle handle);
    bool has_animations() const { return !queued_animations.empty(); }
    std::condition_variable& get_cv() { return cv; }
    std::mutex& get_lock() { return processing_lock; }

private:
    std::vector<std::shared_ptr<Animation>> queued_animations;
    std::thread run_thread;
    std::condition_variable cv;
    std::mutex processing_lock;
    AnimationHandle next_handle = 1;
};

} // miracle

#endif // MIRACLEWM_ANIMATOR_H

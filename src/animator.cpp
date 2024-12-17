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

#include "animator.h"
#include "config.h"
#include <chrono>
#include <mir/server_action_queue.h>
#define MIR_LOG_COMPONENT "animator"
#include <mir/log.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <utility>

using namespace miracle;
using namespace std::chrono_literals;

namespace
{
inline glm::vec2 to_glm_vec2(mir::geometry::Point const& p)
{
    return { p.x.as_int(), p.y.as_int() };
}

inline float get_percent_complete(float target, float real)
{
    if (target == 0)
        return 1.f;

    float percent = real / target;
    if (std::isinf(percent) != 0 || percent > 1.f)
        return 1.f;
    else
        return percent;
}
}

AnimationHandle const miracle::none_animation_handle = 0;

Animation::Animation(
    AnimationHandle handle,
    AnimationDefinition definition,
    mir::geometry::Rectangle const& from,
    mir::geometry::Rectangle const& to,
    mir::geometry::Rectangle const& current,
    std::function<void(AnimationStepResult const&)> const& callback) :
    handle { handle },
    definition { std::move(definition) },
    to { to },
    from { current },
    clip_area { current },
    callback { callback },
    runtime_seconds { 0.f },
    real_size { current.size }
{
    switch (definition.type)
    {
    case AnimationType::slide:
    {
        // Find out the percentage that we're already through the move. This could be negative, by design.
        glm::vec2 end = to_glm_vec2(to.top_left);
        glm::vec2 start = to_glm_vec2(from.top_left);
        glm::vec2 real_start = to_glm_vec2(current.top_left);
        auto percent_x = get_percent_complete(end.x - start.x, real_start.x - start.x);
        auto percent_y = get_percent_complete(end.y - start.y, real_start.y - start.y);

        // Find out the percentage that we're already through the resize. This could be negative, by design.
        float width_change = to.size.width.as_int() - from.size.width.as_int();
        float height_change = to.size.height.as_int() - from.size.height.as_int();
        float real_width_change = current.size.width.as_int() - from.size.width.as_int();
        float real_height_change = current.size.height.as_int() - from.size.height.as_int();

        float percent_w = get_percent_complete(width_change, real_width_change);
        float percent_h = get_percent_complete(height_change, real_height_change);

        float percentage = std::min(percent_x, std::min(percent_y, std::min(percent_w, percent_h)));
        percentage = std::clamp(percentage, 0.f, 1.f);
        runtime_seconds = percentage * definition.duration_seconds;
        break;
    }
    default:
        break;
    }
}

namespace
{
float ease_out_bounce(AnimationDefinition const& defintion, float x)
{
    if (x < 1 / defintion.d1)
    {
        return defintion.n1 * x * x;
    }
    else if (x < 2 / defintion.d1)
    {
        return defintion.n1 * (x -= 1.5f / defintion.d1) * x + 0.75f;
    }
    else if (x < 2.5 / defintion.d1)
    {
        return defintion.n1 * (x -= 2.25f / defintion.d1) * x + 0.9375f;
    }
    else
    {
        return defintion.n1 * (x -= 2.625f / defintion.d1) * x + 0.984375f;
    }
}

inline float ease(AnimationDefinition const& defintion, float t)
{
    // https://easings.net/
    switch (defintion.function)
    {
    case EaseFunction::linear:
        return t;
    case EaseFunction::ease_in_sine:
        return 1 - cosf((t * M_PI) / 2.f);
    case EaseFunction::ease_in_out_sine:
        return -(cosf(M_PI * t) - 1) / 2;
    case EaseFunction::ease_out_sine:
        return sinf((t * M_PI) / 2.f);
    case EaseFunction::ease_in_quad:
        return t * t;
    case EaseFunction::ease_out_quad:
        return 1 - (1 - t) * (1 - t);
    case EaseFunction::ease_in_out_quad:
        return t < 0.5 ? 2 * t * t : 1 - powf(-2 * t + 2, 2) / 2;
    case EaseFunction::ease_in_cubic:
        return t * t * t;
    case EaseFunction::ease_out_cubic:
        return 1 - powf(1 - t, 3);
    case EaseFunction::ease_in_out_cubic:
        return t < 0.5 ? 4 * t * t * t : 1 - powf(-2 * t + 2, 3) / 2;
    case EaseFunction::ease_in_quart:
        return t * t * t * t;
    case EaseFunction::ease_out_quart:
        return 1 - powf(1 - t, 4);
    case EaseFunction::ease_in_out_quart:
        return t < 0.5 ? 8 * t * t * t * t : 1 - powf(-2 * t + 2, 4) / 2;
    case EaseFunction::ease_in_quint:
        return t * t * t * t * t;
    case EaseFunction::ease_out_quint:
        return 1 - powf(1 - t, 5);
    case EaseFunction::ease_in_out_quint:
        return t < 0.5 ? 16 * t * t * t * t * t : 1 - powf(-2 * t + 2, 5) / 2;
    case EaseFunction::ease_in_expo:
        return t == 0 ? 0 : powf(2, 10 * t - 10);
    case EaseFunction::ease_out_expo:
        return t == 1 ? 1 : 1 - powf(2, -10 * t);
    case EaseFunction::ease_in_out_expo:
        return t == 0
            ? 0
            : t == 1
            ? 1
            : t < 0.5 ? powf(2, 20 * t - 10) / 2
                      : (2 - powf(2, -20 * t + 10)) / 2;
    case EaseFunction::ease_in_circ:
        return 1 - sqrtf(1 - powf(t, 2));
    case EaseFunction::ease_out_circ:
        return sqrtf(1 - powf(t - 1, 2));
    case EaseFunction::ease_in_out_circ:
        return t < 0.5f
            ? (1 - sqrtf(1 - powf(2 * t, 2))) / 2
            : (sqrtf(1 - powf(-2 * t + 2, 2)) + 1) / 2;
    case EaseFunction::ease_in_back:
        return defintion.c3 * t * t * t - defintion.c1 * t * t;
    case EaseFunction::ease_out_back:
    {
        return 1 + defintion.c3 * powf(t - 1, 3) + defintion.c1 * powf(t - 1, 2);
    }
    case EaseFunction::ease_in_out_back:
        return t < 0.5
            ? (powf(2 * t, 2) * ((defintion.c2 + 1) * 2 * t - defintion.c2)) / 2
            : (powf(2 * t - 2, 2) * ((defintion.c2 + 1) * (t * 2 - 2) + defintion.c2) + 2) / 2;
    case EaseFunction::ease_in_elastic:
        return t == 0
            ? 0
            : t == 1
            ? 1
            : -powf(2, 10 * t - 10) * sinf((t * 10 - 10.75f) * defintion.c4);
    case EaseFunction::ease_out_elastic:
        return t == 0
            ? 0
            : t == 1
            ? 1
            : powf(2, -10 * t) * sinf((t * 10 - 0.75f) * defintion.c4) + 1;
    case EaseFunction::ease_in_out_elastic:
        return t == 0
            ? 0
            : t == 1
            ? 1
            : t < 0.5
            ? -(powf(2, 20 * t - 10) * sinf((20 * t - 11.125f) * defintion.c5)) / 2
            : (powf(2, -20 * t + 10) * sinf((20 * t - 11.125f) * defintion.c5)) / 2 + 1;
    case EaseFunction::ease_in_bounce:
        return 1 - ease_out_bounce(defintion, 1 - t);
    case EaseFunction::ease_out_bounce:
        return ease_out_bounce(defintion, t);
    case EaseFunction::ease_in_out_bounce:
        return t < 0.5
            ? (1 - ease_out_bounce(defintion, 1 - 2 * t)) / 2
            : (1 + ease_out_bounce(defintion, 2 * t - 1)) / 2;
    default:
        return 1.f;
    }
}

inline float interpolate_scale(float p, float start, float end)
{
    float diff = end - start;
    if (diff == 0)
        return 1.f;

    // We want to find the percentage that we should scale relative
    // to the [start] value. For example, if we are growing from 200
    // to 250, and p=0.5, then we should be at width 225, which would
    // be a scale up of 225 / 220;
    float current = start + diff * p;
    return current / end;
}

inline float interpolate_scale2(float p, float start, float end, float real)
{
    float diff = end - start;
    if (diff == 0)
        return 1.f;

    float current = start + diff * p;
    return current / real;
}

struct SlideResult
{
    /// The current position that the surface should be in.
    /// This should also be used as the clip area position.
    glm::vec2 position;

    /// The current size of the clip area. The surface should NOT
    /// be set to this size, as it has already been set on init().
    /// This size is strictly meant for the clip area.
    glm::vec2 clip_area_size;

    /// The transformation ao apply to the surface.
    glm::mat4 transform;
};

inline SlideResult slide(float p, geom::Rectangle const& from, geom::Rectangle const& to, geom::Size const& committed_size)
{
    auto const distance = to.top_left - from.top_left;
    float const dx = (float)distance.dx.as_int() * p;
    float const dy = (float)distance.dy.as_int() * p;

    float const clip_scale_x = interpolate_scale(p, static_cast<float>(from.size.width.as_value()), static_cast<float>(to.size.width.as_value()));
    float const clip_scale_y = interpolate_scale(p, static_cast<float>(from.size.height.as_value()), static_cast<float>(to.size.height.as_value()));

    // This bit will only make sense by example.
    //
    // Let's say we're growing the width from 50px to 100px. When we first start animating,
    // the client will not have yet confirmed the size, so it will most be 50px.
    // In this case, [real_scale_x] will still be 200%, since it will want to scale from 50px
    // to 100px (assuming p=0).
    //
    // However, after a frame or two, the actual size of the window will be 100px. In this
    // case, we will want to scale down by ~50% (assuming p~=0) from 100px to ~50px.
    float const real_scale_x = interpolate_scale2(
        p,
        static_cast<float>(from.size.width.as_value()),
        static_cast<float>(to.size.width.as_value()),
        static_cast<float>(committed_size.width.as_value()));
    float const real_scale_y = interpolate_scale2(
        p,
        static_cast<float>(from.size.height.as_value()),
        static_cast<float>(to.size.height.as_value()),
        static_cast<float>(committed_size.height.as_value()));

    return {
        .position = glm::vec2(from.top_left.x.as_int() + dx, from.top_left.y.as_int() + dy),
        .clip_area_size = glm::vec2(to.size.width.as_int() * clip_scale_x, to.size.height.as_int() * clip_scale_y),
        .transform = glm::scale(glm::mat4(1.0), glm::vec3(real_scale_x, real_scale_y, 0.f))
    };
}

glm::vec2 to_vec2_point(geom::Rectangle const& r)
{
    return { r.top_left.x.as_int(), r.top_left.y.as_int() };
}

glm::vec2 to_vec2_size(geom::Rectangle const& r)
{
    return { r.size.width.as_int(), r.size.height.as_int() };
}
}

AnimationStepResult Animation::init()
{
    switch (definition.type)
    {
    case AnimationType::grow:
        return { handle, false, clip_area, std::nullopt, std::nullopt, glm::mat4(0.f) };
    case AnimationType::shrink:
        return { handle, false, clip_area, std::nullopt, std::nullopt, glm::mat4(1.f) };
    case AnimationType::slide:
    {
        // Sliding is funky. We resize immediately but remain in the same position. The transformation
        // and position are interpolated over time to give the illusion of moving and growing.
        auto result = slide(0, from, to, real_size);
        return { handle, false, clip_area, result.position, to_vec2_size(to), result.transform };
    }
    case AnimationType::disabled:
        return { handle, true, clip_area, to_vec2_point(to), to_vec2_size(to), glm::mat4(1.f) };
    default:
        return { handle, false, clip_area, std::nullopt, std::nullopt, std::nullopt };
    }
}

AnimationStepResult Animation::step()
{
    runtime_seconds += Animator::timestep_seconds;
    float const t = (runtime_seconds / definition.duration_seconds);

    if (runtime_seconds >= definition.duration_seconds)
    {
        return { handle, true, to, to_vec2_point(to), to_vec2_size(to), glm::mat4(1.f) };
    }

    switch (definition.type)
    {
    case AnimationType::slide:
    {
        auto const p = ease(definition, t);
        auto const result = slide(p, from, to, real_size);
        clip_area.top_left.x = geom::X { result.position.x };
        clip_area.top_left.y = geom::Y { result.position.y };
        clip_area.size.width = geom::Width { result.clip_area_size.x };
        clip_area.size.height = geom::Height { result.clip_area_size.y };
        return {
            handle,
            false,
            clip_area,
            result.position,
            std::nullopt,
            result.transform
        };
    }
    case AnimationType::grow:
    {
        auto p = ease(definition, t);
        glm::vec3 translate(
            (float)to.size.width.as_value() / 2.f,
            (float)to.size.height.as_value() / 2.f,
            0);
        auto inverse_translate = -translate;
        glm::mat4 transform = glm::translate(
            glm::scale(
                glm::translate(translate),
                glm::vec3(p, p, 1.f)),
            inverse_translate);
        return { handle, false, to, std::nullopt, std::nullopt, transform };
    }
    case AnimationType::shrink:
    {
        auto p = 1.f - ease(definition, t);
        glm::vec3 translate(
            (float)to.size.width.as_value() / 2.f,
            (float)to.size.height.as_value() / 2.f,
            0);
        auto inverse_translate = -translate;
        glm::mat4 transform = glm::translate(
            glm::scale(
                glm::translate(translate),
                glm::vec3(p, p, 1.f)),
            inverse_translate);
        return { handle, false, to, std::nullopt, std::nullopt, transform };
    }
    case AnimationType::disabled:
    default:
        return { handle, true, to, std::nullopt, std::nullopt, std::nullopt };
    }
}

void Animation::set_current_size(mir::geometry::Size const& size)
{
    real_size = size;
}

Animator::Animator(
    std::shared_ptr<mir::ServerActionQueue> const& server_action_queue,
    std::shared_ptr<Config> const& config) :
    server_action_queue { server_action_queue },
    config { config }
{
}

void Animator::start()
{
    run();
//    run_thread = std::thread([&]()
//    { run(); });
}

Animator::~Animator()
{
    stop();
};

AnimationHandle Animator::register_animateable()
{
    return next_handle++;
}

void Animator::append(miracle::Animation&& animation)
{
    std::lock_guard<std::mutex> lock(processing_lock);
    for (auto it = queued_animations.begin(); it != queued_animations.end();)
    {
        if (it->get_handle() == animation.get_handle())
        {
            it = queued_animations.erase(it);
        }
        else
            it++;
    }

    animation.get_callback()(animation.init());
    queued_animations.push_back(animation);
    cv.notify_one();
}

void Animator::window_move(
    AnimationHandle handle,
    mir::geometry::Rectangle const& from,
    mir::geometry::Rectangle const& to,
    mir::geometry::Rectangle const& current,
    std::function<void(AnimationStepResult const&)> const& callback)
{
    // If animations aren't enabled, let's give them the position that
    // they want to go to immediately and don't bother animating anything.
    if (!config->are_animations_enabled())
    {
        callback(
            AnimationStepResult { handle,
                true,
                to,
                glm::vec2(to.top_left.x.as_int(), to.top_left.y.as_int()),
                glm::vec2(to.size.width.as_int(), to.size.height.as_int()),
                glm::mat4(1.f) });
        return;
    }

    append(Animation(
        handle,
        config->get_animation_definitions()[(int)AnimateableEvent::window_move],
        from,
        to,
        current,
        callback));
}

void Animator::window_open(
    AnimationHandle handle,
    mir::geometry::Rectangle const& from,
    mir::geometry::Rectangle const& to,
    mir::geometry::Rectangle const& current,
    std::function<void(AnimationStepResult const&)> const& callback)
{
    // If animations aren't enabled, let's give them the position that
    // they want to go to immediately and don't bother animating anything.
    if (!config->are_animations_enabled())
    {
        callback(AnimationStepResult { handle, true, to });
        return;
    }

    append(Animation(
        handle,
        config->get_animation_definitions()[(int)AnimateableEvent::window_open],
        from,
        to,
        current,
        callback));
}

void Animator::workspace_switch(
    AnimationHandle handle,
    mir::geometry::Rectangle const& from,
    mir::geometry::Rectangle const& to,
    mir::geometry::Rectangle const& current,
    std::function<void(AnimationStepResult const&)> const& callback)
{
    if (!config->are_animations_enabled())
    {
        callback(
            AnimationStepResult { handle,
                true,
                to,
                glm::vec2(to.top_left.x.as_int(), to.top_left.y.as_int()),
                glm::vec2(to.size.width.as_int(), to.size.height.as_int()),
                glm::mat4(1.f) });
        return;
    }

    append(Animation(
        handle,
        config->get_animation_definitions()[(int)AnimateableEvent::workspace_switch],
        from,
        to,
        current,
        callback));
}

void Animator::run()
{
    using clock = std::chrono::high_resolution_clock;
    lag = 0ns;
    time_start = clock::now();
    running = true;

    server_action_queue->enqueue(this, [this]() { tick(); });

}

void Animator::tick() {
    using clock = std::chrono::high_resolution_clock;
    constexpr std::chrono::nanoseconds timestep(16ms);

    if (!running) {
        return;
    }

    {
//        std::unique_lock lock(processing_lock);
        if (queued_animations.empty())
        {
//            cv.wait(lock);
            time_start = clock::now();
            server_action_queue->enqueue_with_guaranteed_execution([&]() { tick(); });
            return;
        }
    }

    auto delta_time = clock::now() - time_start;
    time_start = clock::now();
    lag += std::chrono::duration_cast<std::chrono::nanoseconds>(delta_time);

    while (lag >= timestep)
    {
        lag -= timestep;
        step();
    }

    server_action_queue->enqueue(this, [this]() { tick(); });
}

void Animator::step()
{
    std::lock_guard<std::mutex> lock(processing_lock);
    for (auto it = queued_animations.begin(); it != queued_animations.end();)
    {
        auto& item = *it;
        auto result = item.step();

        item.get_callback()(result);
        if (result.is_complete)
            it = queued_animations.erase(it);
        else
            it++;
    }
}

void Animator::set_size_hack(AnimationHandle handle, mir::geometry::Size const& size)
{
    {
        std::lock_guard<std::mutex> lock(processing_lock);
        for (auto& animation : queued_animations)
        {
            if (animation.get_handle() == handle)
            {
                animation.set_current_size(size);
                animation.get_callback()(animation.step());
            }
        }
    }
}

void Animator::stop()
{
    if (!running)
        return;

    running = false;
    cv.notify_one();
//    run_thread.join();
}

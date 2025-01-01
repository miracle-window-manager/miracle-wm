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

#include "animator_loop.h"
#include "animator.h"

#include <mir/server_action_queue.h>

using namespace miracle;
using namespace std::chrono_literals;

ThreadedAnimatorLoop::ThreadedAnimatorLoop(std::shared_ptr<Animator> const& animator) :
    animator { animator }
{
}

ThreadedAnimatorLoop::~ThreadedAnimatorLoop()
{
    stop();
}

void ThreadedAnimatorLoop::start()
{
    run_thread = std::thread([this]()
    { run(); });
}

void ThreadedAnimatorLoop::stop()
{
    if (!running)
        return;

    running = false;
    run_thread.join();
}

void ThreadedAnimatorLoop::run()
{
    using clock = std::chrono::high_resolution_clock;
    constexpr std::chrono::nanoseconds timestep(16ms);
    auto last_time = clock::now();
    running = true;

    while (running)
    {
        {
            std::unique_lock lock(animator->get_lock());
            if (!animator->has_animations())
            {
                animator->get_cv().wait(lock);
                last_time = clock::now();
            }
        }

        delta_time = clock::now() - last_time;
        last_time = clock::now();
        animator->tick(delta_time.count());
        std::this_thread::sleep_for(1ms);
    }
}

ServerActionQueueAnimatorLoop::ServerActionQueueAnimatorLoop(
    std::shared_ptr<Animator> const& animator,
    std::shared_ptr<mir::ServerActionQueue> const& server_action_queue) :
    animator { animator },
    server_action_queue { server_action_queue }
{
}

ServerActionQueueAnimatorLoop::~ServerActionQueueAnimatorLoop()
{
    stop();
}

void ServerActionQueueAnimatorLoop::start()
{
    running = true;
    using clock = std::chrono::high_resolution_clock;
    last_time = clock::now();
    run();
}

void ServerActionQueueAnimatorLoop::stop()
{
    if (!running)
        return;

    running = false;
}

void ServerActionQueueAnimatorLoop::run()
{
    if (!running)
        return;

    using clock = std::chrono::high_resolution_clock;
    delta_time = clock::now() - last_time;
    last_time = clock::now();
    animator->tick(delta_time.count());

    server_action_queue->enqueue(this, [this]()
    {
        run();
    });
}
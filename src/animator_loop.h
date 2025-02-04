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

#ifndef MIRACLEWM_ANIMATOR_LOOP_H
#define MIRACLEWM_ANIMATOR_LOOP_H

#include <chrono>
#include <condition_variable>
#include <memory>
#include <thread>

namespace mir
{
class ServerActionQueue;
}

namespace miracle
{
class Animator;

class AnimatorLoop
{
public:
    virtual ~AnimatorLoop() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
};

class ThreadedAnimatorLoop : public AnimatorLoop
{
public:
    explicit ThreadedAnimatorLoop(std::shared_ptr<Animator> const&);
    ~ThreadedAnimatorLoop() override;
    void start() override;
    void stop() override;

private:
    void run();

    std::shared_ptr<Animator> animator;
    std::thread run_thread;
    bool running = false;
    std::chrono::duration<float> delta_time;
};

class ServerActionQueueAnimatorLoop : public AnimatorLoop
{
public:
    explicit ServerActionQueueAnimatorLoop(
        std::shared_ptr<Animator> const&,
        std::shared_ptr<mir::ServerActionQueue> const&);
    ~ServerActionQueueAnimatorLoop();
    void start() override;
    void stop() override;

private:
    void run();

    std::shared_ptr<Animator> animator;
    std::shared_ptr<mir::ServerActionQueue> server_action_queue;
    bool running = false;
    std::chrono::duration<float> delta_time;
    std::chrono::system_clock::time_point last_time;
};
}

#endif
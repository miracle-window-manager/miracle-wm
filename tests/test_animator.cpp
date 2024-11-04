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
#include "yaml-cpp/yaml.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <mir/server_action_queue.h>
#include <miral/runner.h>

using namespace miracle;

namespace
{
int argc = 1;
char const* argv[] = { "miracle-wm-tests" };
const std::string path = std::filesystem::current_path() / "test.yaml";
}

class ImmediateServerActionQueue : public mir::ServerActionQueue
{
    void enqueue(void const* owner, mir::ServerAction const& action) override
    {
        action();
    }

    void enqueue_with_guaranteed_execution(mir::ServerAction const& action) override
    {
        action();
    }

    void pause_processing_for(void const* owner) override {};
    void resume_processing_for(void const* owner) override {};
};

class AnimatorTest : public testing::Test
{
public:
    AnimatorTest() :
        runner(argc, argv),
        queue { std::make_shared<ImmediateServerActionQueue>() },
        config { std::make_shared<FilesystemConfiguration>(runner, path) }
    {
    }
    miral::MirRunner runner;
    std::shared_ptr<mir::ServerActionQueue> queue;
    std::shared_ptr<Config> config;
};

TEST_F(AnimatorTest, CanStepLinearSlideAnimation)
{
    YAML::Node node;
    YAML::Node item;
    item["event"] = "window_move";
    item["type"] = "slide";
    item["function"] = "linear";
    item["duration"] = 1;
    node["animations"].push_back(item);
    std::fstream file(path, std::ios::app);
    file << node;

    Animator animator(queue, config);
    auto handle = animator.register_animateable();
    bool was_called = false;
    animator.window_move(
        handle,
        mir::geometry::Rectangle(
            mir::geometry::Point(0, 0),
            mir::geometry::Size(0, 0)),
        mir::geometry::Rectangle(
            mir::geometry::Point(600, 0),
            mir::geometry::Size(0, 0)),
        mir::geometry::Rectangle(
            mir::geometry::Point(0, 0),
            mir::geometry::Size(0, 0)),
        [&](auto const& asr)
    {
        was_called = true;
    });
    animator.step();
    EXPECT_EQ(was_called, true);
}

TEST_F(AnimatorTest, LinearSlideResultsInCorrectNewPoint)
{
    YAML::Node node;
    YAML::Node item;
    item["event"] = "window_move";
    item["type"] = "slide";
    item["function"] = "linear";
    item["duration"] = 1;
    node["animations"].push_back(item);
    std::fstream file(path, std::ios::app);
    file << node;

    Animator animator(queue, config);
    auto handle = animator.register_animateable();
    mir::geometry::Point point;
    animator.window_move(
        handle,
        mir::geometry::Rectangle(
            mir::geometry::Point(0, 0),
            mir::geometry::Size(0, 0)),
        mir::geometry::Rectangle(
            mir::geometry::Point(600, 0),
            mir::geometry::Size(0, 0)),
        mir::geometry::Rectangle(
            mir::geometry::Point(0, 0),
            mir::geometry::Size(0, 0)),
        [&](AnimationStepResult const& asr)
    {
        if (asr.position)
            EXPECT_EQ(asr.position.value().x, 600 * Animator::timestep_seconds);
    });
    animator.step();
}

class AnimationTest : public testing::Test
{
};

TEST_F(AnimationTest, InterruptingSlideResultsInModifiedAnimationDuration)
{
    AnimationHandle handle = 0;
    AnimationDefinition definition;
    definition.duration_seconds = 6;
    definition.type = AnimationType::slide;
    definition.function = EaseFunction::linear;
    Animation animation(
        handle,
        definition,
        mir::geometry::Rectangle(
            mir::geometry::Point(0, 0),
            mir::geometry::Size(0, 0)),
        mir::geometry::Rectangle(
            mir::geometry::Point(600, 600),
            mir::geometry::Size(0, 0)),
        mir::geometry::Rectangle(
            mir::geometry::Point(200, 200),
            mir::geometry::Size(0, 0)),
        [](auto const& asr) {});
    ASSERT_NEAR(animation.get_runtime_seconds(), 2, 0.05);
}
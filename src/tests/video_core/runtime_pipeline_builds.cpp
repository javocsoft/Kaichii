// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <chrono>
#include <future>
#include <memory>
#include <stdexcept>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include "video_core/renderer_vulkan/vk_pipeline_runtime_builds.h"

namespace {
struct Dummy {
    int value;
};
using Builds = Vulkan::RuntimePipelineBuilds<int, Dummy>;
using State = Builds::State;

std::unique_ptr<Dummy> Make(int value) {
    return std::make_unique<Dummy>(Dummy{value});
}

/// Polls until the key is no longer pending (each worker build is quick here).
State PollUntilDone(Builds& builds, int key, std::unique_ptr<Dummy>& out) {
    for (int i = 0; i < 2000; ++i) {
        const State state = builds.Poll(key, out);
        if (state != State::Pending) {
            return state;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    return State::Pending;
}
} // namespace

TEST_CASE("Runtime pipeline builds hand over results once", "[video_core][pipeline_build]") {
    Builds builds{2};
    std::unique_ptr<Dummy> out;
    REQUIRE(builds.Poll(1, out) == State::None);
    REQUIRE(builds.Submit(1, [] { return Make(10); }));
    REQUIRE_FALSE(builds.Submit(1, [] { return Make(11); })); // queued already
    REQUIRE(PollUntilDone(builds, 1, out) == State::Ready);
    REQUIRE(out != nullptr);
    REQUIRE(out->value == 10);
    REQUIRE(builds.Poll(1, out) == State::None); // taken
    // A key can be queued again once taken.
    REQUIRE(builds.Submit(1, [] { return Make(12); }));
    REQUIRE(builds.Wait(1)->value == 12);
    REQUIRE(builds.PendingCount() == 0);
}

TEST_CASE("Runtime pipeline builds report failures as empty results",
          "[video_core][pipeline_build]") {
    Builds builds{1};
    REQUIRE(builds.Submit(1, [] { return std::unique_ptr<Dummy>{}; }));
    REQUIRE(builds.Submit(2, []() -> std::unique_ptr<Dummy> { throw std::runtime_error{"bad"}; }));
    std::unique_ptr<Dummy> out = Make(0);
    REQUIRE(PollUntilDone(builds, 1, out) == State::Ready);
    REQUIRE(out == nullptr);
    REQUIRE(builds.Wait(2) == nullptr);
    REQUIRE(builds.Wait(3) == nullptr); // never queued
}

TEST_CASE("Runtime pipeline builds stay pending while building, and Wait builds what no worker "
          "started",
          "[video_core][pipeline_build]") {
    Builds builds{1};
    std::promise<void> started;
    std::promise<void> release;
    std::shared_future<void> released = release.get_future().share();
    // The only worker is held by key 1.
    REQUIRE(builds.Submit(1, [&started, released] {
        started.set_value();
        released.wait();
        return Make(1);
    }));
    started.get_future().wait();
    std::unique_ptr<Dummy> out;
    REQUIRE(builds.Poll(1, out) == State::Pending);
    // Key 2 waits behind it in the queue: Wait runs it on this thread instead.
    REQUIRE(builds.Submit(2, [] { return Make(2); }));
    REQUIRE(builds.Poll(2, out) == State::Pending);
    REQUIRE(builds.Wait(2)->value == 2);
    REQUIRE(builds.Poll(1, out) == State::Pending);
    release.set_value();
    REQUIRE(builds.Wait(1)->value == 1);
    REQUIRE(builds.PendingCount() == 0);
}

TEST_CASE("Runtime pipeline builds take nothing after Stop", "[video_core][pipeline_build]") {
    Builds builds{1};
    REQUIRE(builds.Submit(1, [] { return Make(1); }));
    builds.Stop();
    std::unique_ptr<Dummy> out;
    REQUIRE(builds.Poll(1, out) == State::None);
    REQUIRE_FALSE(builds.Submit(2, [] { return Make(2); }));
}

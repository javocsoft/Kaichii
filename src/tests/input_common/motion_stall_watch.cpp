// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <chrono>
#include <catch2/catch_test_macros.hpp>

#include "input_common/drivers/motion_stall_watch.h"

using namespace std::chrono_literals;
using InputCommon::MotionStallWatch;

TEST_CASE("Reporting sensors are never restarted", "[motion_stall_watch]") {
    MotionStallWatch watch;
    MotionStallWatch::Clock::time_point now{};
    watch.Enabled(now);
    for (int report = 0; report < 1000; ++report) {
        now += 4ms;
        watch.Reported(now);
        CHECK_FALSE(watch.Stalled(now));
    }
}

TEST_CASE("Silent sensors are restarted after one second, then less often",
          "[motion_stall_watch]") {
    MotionStallWatch watch;
    MotionStallWatch::Clock::time_point now{};
    watch.Enabled(now);
    for (const auto gap : {1000ms, 2000ms, 4000ms, 8000ms, 8000ms}) {
        CHECK_FALSE(watch.Stalled(now + gap - 1ms));
        now += gap;
        REQUIRE(watch.Stalled(now));
        watch.Enabled(now);
    }
}

TEST_CASE("A report after restarts brings the wait back to one second", "[motion_stall_watch]") {
    MotionStallWatch watch;
    MotionStallWatch::Clock::time_point now{};
    watch.Enabled(now);
    now += 1s;
    REQUIRE(watch.Stalled(now));
    watch.Enabled(now);
    now += 2s;
    REQUIRE(watch.Stalled(now));
    watch.Enabled(now);
    now += 10ms;
    watch.Reported(now);
    CHECK_FALSE(watch.Stalled(now + 999ms));
    CHECK(watch.Stalled(now + 1s));
}

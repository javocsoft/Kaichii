// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "common/frame_pacing.h"

using Catch::Approx;
using Common::ComputeFramePacing;

TEST_CASE("Frame pacing of a steady 30 fps", "[common][frame_pacing]") {
    const std::vector<double> steady(300, 1000.0 / 30.0);
    const auto pacing = ComputeFramePacing(steady);
    CHECK(pacing.frames == 300);
    CHECK(pacing.fps == Approx(30.0));
    CHECK(pacing.average_fps == Approx(30.0));
    CHECK(pacing.median_ms == Approx(33.333).epsilon(0.001));
    CHECK(pacing.low_1pct_fps == Approx(30.0));
    CHECK(pacing.stutters == 0);
}

TEST_CASE("Frame pacing shows stalls an average hides", "[common][frame_pacing]") {
    // 10 s at 30 fps with three stalls of 300 ms: the average barely moves.
    std::vector<double> intervals(297, 1000.0 / 30.0);
    intervals[100] = intervals[200] = intervals[250] = 300.0;
    const auto pacing = ComputeFramePacing(intervals);
    CHECK(pacing.average_fps > 27.0);
    CHECK(pacing.stutters == 3);
    CHECK(pacing.max_ms == Approx(300.0));
    CHECK(pacing.low_1pct_fps == Approx(1000.0 / 300.0));
    CHECK(pacing.median_ms == Approx(33.333).epsilon(0.001));
}

TEST_CASE("Frame pacing's current rate looks at the recent frames only", "[common][frame_pacing]") {
    // 5 s at 60 fps, then 1 s at 30 fps.
    std::vector<double> intervals(300, 1000.0 / 60.0);
    intervals.insert(intervals.end(), 30, 1000.0 / 30.0);
    const auto pacing = ComputeFramePacing(intervals);
    CHECK(pacing.fps == Approx(30.0));
    CHECK(pacing.average_fps > 50.0);
    // A 60 fps game's slow 30 fps frames are not stutters: under 40 ms.
    CHECK(pacing.stutters == 0);
    CHECK(ComputeFramePacing({}).frames == 0);
}

TEST_CASE("The generated frame rate comes from two counter readings", "[common][frame_pacing]") {
    using Common::GeneratedFrameRate;
    REQUIRE(GeneratedFrameRate(1000, 1320, 10.0) == 32.0);
    REQUIRE(GeneratedFrameRate(1000, 1000, 10.0) == 0.0);
    REQUIRE(GeneratedFrameRate(1000, 40, 10.0) == 0.0); // a new game started the counter again
    REQUIRE(GeneratedFrameRate(0, 300, 0.0) == 0.0);
}

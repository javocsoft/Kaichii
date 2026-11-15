// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "input_common/drivers/motion_sample_clock.h"

TEST_CASE("A reconnected motion sensor resumes after its timestamp restarts", "[input][motion]") {
    InputCommon::MotionSampleClock clock;
    CHECK_FALSE(clock.Advance(10'000'000));
    CHECK(clock.Advance(10'004'000) == 4);
    CHECK_FALSE(clock.Advance(1'000));
    CHECK(clock.Advance(5'000) == 4);
    clock.Reset();
    CHECK_FALSE(clock.Advance(2'000));
    CHECK(clock.Advance(7'000) == 5);
}

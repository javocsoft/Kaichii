// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "common/literals.h"
#include "video_core/texture_cache/collector_escalation.h"

using namespace Common::Literals;
using VideoCommon::EscalateCollector;

TEST_CASE("The texture collector only escalates when its cache runs away",
          "[video_core][texture_cache]") {
    constexpr u64 critical = 2915_MiB;
    // A 4 GB card in Kokiri Forest: past the critical memory, within the margin.
    STATIC_REQUIRE(EscalateCollector(true, critical + 210_MiB, critical, 0).extra_iterations == 0);
    STATIC_REQUIRE(EscalateCollector(true, critical + 512_MiB, critical, 0).extra_iterations == 0);
    // Not in aggressive mode: nothing changes however far past.
    STATIC_REQUIRE(EscalateCollector(false, critical + 4_GiB, critical, 0).extra_iterations == 0);

    // Running away: one more deletion per 2 MiB over, images unused 3 frames from 1 GiB over.
    constexpr auto runaway = EscalateCollector(true, critical + 600_MiB, critical, 0);
    STATIC_REQUIRE(runaway.extra_iterations == 300);
    STATIC_REQUIRE(runaway.ticks_to_destroy == 0);
    constexpr auto far = EscalateCollector(true, critical + 1_GiB, critical, 0);
    STATIC_REQUIRE(far.extra_iterations == 512);
    STATIC_REQUIRE(far.ticks_to_destroy == 3);
    // Capped at 1024 more, and a longer minimum age still holds.
    constexpr auto capped = EscalateCollector(true, critical + 5_GiB, critical, 120);
    STATIC_REQUIRE(capped.extra_iterations == 1024);
    STATIC_REQUIRE(capped.ticks_to_destroy == 120);
}

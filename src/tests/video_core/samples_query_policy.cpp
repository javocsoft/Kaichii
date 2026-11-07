// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "video_core/renderer_vulkan/samples_query_policy.h"

TEST_CASE("Samples queries keep their count through gameplay windows",
          "[video_core][query_cache]") {
    using Vulkan::DropsUnreportedSamples;
    // Ocarina of Time near water and windows, past one 256-slot bank.
    STATIC_REQUIRE_FALSE(DropsUnreportedSamples(257));
    STATIC_REQUIRE_FALSE(DropsUnreportedSamples(368));
    STATIC_REQUIRE_FALSE(DropsUnreportedSamples(Vulkan::MaxUnreportedSamplesSlots));
    // Its title screen and file select, which count without ever resetting.
    STATIC_REQUIRE(DropsUnreportedSamples(Vulkan::MaxUnreportedSamplesSlots + 1));
    STATIC_REQUIRE(DropsUnreportedSamples(16384));
}

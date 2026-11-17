// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "video_core/renderer_vulkan/present/frame_gen_pacer.h"

TEST_CASE("Frame generation reserves a present slot for the next game frame",
          "[video_core][frame_gen_pacer]") {
    using Vulkan::GenerationsWithoutWaiting;
    REQUIRE(GenerationsWithoutWaiting(1, 0) == 0);
    REQUIRE(GenerationsWithoutWaiting(1, 1) == 0);
    REQUIRE(GenerationsWithoutWaiting(1, 2) == 1);
    REQUIRE(GenerationsWithoutWaiting(3, 3) == 2);
    REQUIRE(GenerationsWithoutWaiting(3, 4) == 3);
    REQUIRE(GenerationsWithoutWaiting(0, 4) == 0);
}

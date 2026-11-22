// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "video_core/depth_bias.h"

// The slope bias is per host pixel: a rescaled target with a rescaled viewport needs it scaled to
// guest pixels, as at 1x (zortos293/Lichii#2057, Link's Awakening's striped shadows above 1x).
TEST_CASE("The slope depth bias follows the resolution scale", "[video_core][depth_bias]") {
    using VideoCore::RescaledSlopeScale;
    CHECK(RescaledSlopeScale(true, true, 2.0f) == 2.0f);
    CHECK(RescaledSlopeScale(true, true, 0.5f) == 0.5f); // Kaichii's Performance preset
    // An unscaled target or a viewport the game gives in host pixels keeps the game's value.
    CHECK(RescaledSlopeScale(false, true, 2.0f) == 1.0f);
    CHECK(RescaledSlopeScale(true, false, 2.0f) == 1.0f);
    CHECK(RescaledSlopeScale(true, true, 1.0f) == 1.0f);
}

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "common/overlay_position.h"

TEST_CASE("The overlay's saved offset stays inside the window", "[common][overlay]") {
    using Common::ClampOverlayOffset;
    using Common::OverlayOffset;
    // A 1650x910 window and a 220x250 overlay.
    REQUIRE(ClampOverlayOffset(Common::DefaultOverlayOffset, 1650, 910, 220, 250) ==
            Common::DefaultOverlayOffset);
    REQUIRE(ClampOverlayOffset({1065, 140}, 1650, 910, 220, 250) == OverlayOffset{1065, 140});
    // Dragged out on a larger window, or to another screen: back inside.
    REQUIRE(ClampOverlayOffset({3000, 2000}, 1650, 910, 220, 250) == OverlayOffset{1430, 660});
    REQUIRE(ClampOverlayOffset({-400, -50}, 1650, 910, 220, 250) == OverlayOffset{0, 0});
    // A window smaller than the overlay.
    REQUIRE(ClampOverlayOffset({50, 50}, 200, 100, 220, 250) == OverlayOffset{0, 0});
}

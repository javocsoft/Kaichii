// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "common/qt_platform_choice.h"

TEST_CASE("The Qt frontend runs through X11 unless the player chose otherwise",
          "[common][qt_platform]") {
    using Common::QtPlatformOverride;
    // On by default: X11 first, Wayland when no X server answers.
    STATIC_REQUIRE(Common::DefaultForceX11);
    CHECK(QtPlatformOverride(Common::DefaultForceX11, "") == "xcb;wayland");
    // The player chose Wayland in the settings.
    CHECK(QtPlatformOverride(false, "").empty());
    // QT_QPA_PLATFORM set by the player always wins.
    CHECK(QtPlatformOverride(true, "wayland").empty());
    CHECK(QtPlatformOverride(true, "xcb").empty());
}

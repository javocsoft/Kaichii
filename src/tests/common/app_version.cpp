// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "common/app_version.h"

// The window title names the version a player can remember; About adds the details.
TEST_CASE("A release is named by its version", "[frontend][app_version]") {
    CHECK(Common::AppTitle("Kaichii", "1.0.49", "49") == "Kaichii v1.0.49");
    CHECK(Common::AppDetails("Kaichii", "1.0.49", "49", "build-49", "Clang 23.1.6") ==
          "Kaichii v1.0.49 | build 49 | Clang 23.1.6");
}

TEST_CASE("A build that wasn't released says so, with its commit", "[frontend][app_version]") {
    CHECK(Common::AppTitle("Kaichii", "1.0", "") == "Kaichii v1.0 (dev)");
    CHECK(Common::AppDetails("Kaichii", "1.0", "", "1a2b3c4d5e-nx2", "GNU 13.3.0") ==
          "Kaichii v1.0 (dev) | 1a2b3c4d5e-nx2 | GNU 13.3.0");
}

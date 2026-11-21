// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "yuzu/menu_levels.h"

using Yuzu::MenuLevel;
using Yuzu::MenuShows;

// The menus hide entries for the advanced options and developer mode, as the settings do.
TEST_CASE("A player sees only the basic menu entries", "[frontend][menu_levels]") {
    CHECK(MenuShows(MenuLevel::Basic, false, false));
    CHECK_FALSE(MenuShows(MenuLevel::Advanced, false, false));
    CHECK_FALSE(MenuShows(MenuLevel::Developer, false, false));
}

TEST_CASE("Advanced options show the advanced entries, developer mode the rest",
          "[frontend][menu_levels]") {
    CHECK(MenuShows(MenuLevel::Advanced, true, false));
    CHECK_FALSE(MenuShows(MenuLevel::Developer, true, false));
    CHECK(MenuShows(MenuLevel::Developer, true, true));
    // Developer mode left on while the advanced options are off shows nothing more, as in the
    // settings, where its box is disabled then.
    CHECK_FALSE(MenuShows(MenuLevel::Advanced, false, true));
    CHECK_FALSE(MenuShows(MenuLevel::Developer, false, true));
}

TEST_CASE("Playing and setting up stay basic", "[frontend][menu_levels]") {
    for (const auto& [name, level] : Yuzu::MainMenuLevels) {
        INFO(name);
        CHECK(level != MenuLevel::Basic);
        CHECK(name != "action_Configure");
        CHECK(name != "action_Install_Keys");
        CHECK(name != "menu_Game_List_Mode");
        CHECK(name != "action_Open_UserHandbook");
    }
}

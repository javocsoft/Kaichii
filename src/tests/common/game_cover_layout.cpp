// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib>

#include <catch2/catch_test_macros.hpp>

#include "yuzu/game/cover_layout.h"

using Yuzu::ComputeBoxLayout;
using Yuzu::ComputeCoverLayout;

// The cover view ("View > Game List Mode > Cover View") lays the game list out as a shelf.
TEST_CASE("The cover shelf fills the width with whole columns", "[frontend][cover_view]") {
    for (const int width : {300, 640, 1024, 1280, 1920, 2560, 3840}) {
        const auto layout = ComputeCoverLayout(width);
        INFO("width " << width);
        // Every column fits, and no wider than the view: a wider row would wrap.
        CHECK(layout.columns * layout.cell_width < width);
        // Less than one more column is left over.
        CHECK(width - layout.columns * layout.cell_width <= layout.cell_width);
        // The cover sits inside its cell with room for its shadow.
        CHECK(layout.cover + Yuzu::CoverGap <= layout.cell_width);
        CHECK(layout.cover >= Yuzu::MinCover);
        CHECK(layout.cover <= Yuzu::MaxCover);
    }
}

TEST_CASE("The cover shelf keeps covers near their target size", "[frontend][cover_view]") {
    // A full HD window: seven covers a row, each a little over the target.
    const auto full_hd = ComputeCoverLayout(1920);
    CHECK(full_hd.columns == 7);
    CHECK(full_hd.cover >= Yuzu::TargetCover);
    CHECK(full_hd.cover < Yuzu::TargetCover + Yuzu::TargetCover / 4);
    // Widening the window adds columns rather than growing the covers without end.
    CHECK(ComputeCoverLayout(3840).columns > full_hd.columns);
    CHECK(ComputeCoverLayout(3840).cover <= Yuzu::MaxCover);
}

TEST_CASE("The cover shelf stays usable in a tiny or empty view", "[frontend][cover_view]") {
    for (const int width : {0, 1, 50, 160}) {
        const auto layout = ComputeCoverLayout(width);
        INFO("width " << width);
        CHECK(layout.columns == 1);
        CHECK(layout.cover == Yuzu::MinCover);
    }
}

// The full cover view ("View > Game List Mode > Full Cover View") shows each game's case front.
TEST_CASE("The box shelf fills the width with cases of a Switch case's shape", "[frontend][cover_view]") {
    for (const int width : {0, 300, 640, 1024, 1280, 1920, 2560, 3840}) {
        const auto layout = ComputeBoxLayout(width);
        INFO("width " << width);
        CHECK(layout.columns >= 1);
        if (width > Yuzu::MinBox + Yuzu::CoverGap) {
            CHECK(layout.columns * layout.cell_width < width);
            CHECK(layout.width + Yuzu::CoverGap <= layout.cell_width);
        }
        CHECK(layout.width >= Yuzu::MinBox);
        CHECK(layout.width <= Yuzu::MaxBox);
        // 105 by 170, to the nearest pixel.
        CHECK(std::abs(layout.height * Yuzu::BoxAspectWidth -
                       layout.width * Yuzu::BoxAspectHeight) <= Yuzu::BoxAspectWidth / 2);
    }
    // A full HD window holds nine cases a row, a little wider than the target.
    const auto full_hd = ComputeBoxLayout(1920);
    CHECK(full_hd.columns == 9);
    CHECK(full_hd.width >= Yuzu::TargetBox);
}

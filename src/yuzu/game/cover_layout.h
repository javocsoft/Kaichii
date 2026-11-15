// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>

namespace Yuzu {

/// The cover view's shelf for a width: whole columns sharing the width, each cover filling its
/// cell less the gap around it.
struct CoverLayout {
    int columns;
    int cell_width;
    int cover;
};

/// The cover side the shelf aims for, and the limits a very narrow or wide window keeps it in.
inline constexpr int TargetCover = 220;
inline constexpr int MinCover = 140;
inline constexpr int MaxCover = 360;
/// Room around each cover: its shadow and the space between covers.
inline constexpr int CoverGap = 22;

[[nodiscard]] constexpr CoverLayout ComputeCoverLayout(int view_width) {
    // One pixel less than the view, so the last column never wraps onto the next row.
    const int width = std::max(1, view_width - 1);
    const int columns = std::max(1, width / (TargetCover + CoverGap));
    const int cell_width = width / columns;
    return {columns, cell_width, std::clamp(cell_width - CoverGap, MinCover, MaxCover)};
}

} // namespace Yuzu

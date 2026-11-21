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

/// The full cover view's shelf: the same columns, each holding a game case's front.
struct BoxLayout {
    int columns;
    int cell_width;
    int width;
    int height;
};

/// A Switch case's front is 105 mm wide and 170 mm high.
inline constexpr int BoxAspectWidth = 105;
inline constexpr int BoxAspectHeight = 170;
/// The box width the shelf aims for, and its limits.
inline constexpr int TargetBox = 180;
inline constexpr int MinBox = 110;
inline constexpr int MaxBox = 300;

[[nodiscard]] constexpr int BoxHeight(int box_width) {
    return (box_width * BoxAspectHeight + BoxAspectWidth / 2) / BoxAspectWidth;
}

[[nodiscard]] constexpr BoxLayout ComputeBoxLayout(int view_width) {
    const int width = std::max(1, view_width - 1);
    const int columns = std::max(1, width / (TargetBox + CoverGap));
    const int cell_width = width / columns;
    const int box_width = std::clamp(cell_width - CoverGap, MinBox, MaxBox);
    return {columns, cell_width, box_width, BoxHeight(box_width)};
}

} // namespace Yuzu

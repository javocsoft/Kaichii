// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <span>
#include <vector>

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

/// A game on a shelf: its model row, and whether it is a Switch 2 title.
struct ShelfEntry {
    int row;
    bool switch2;
};

/// Where a game's cell sits, in the shelf's own coordinates.
struct ShelfSpot {
    int row;
    int x;
    int y;
};

/// A section's heading: the platform it gathers, how many games, and where it sits.
struct ShelfHeading {
    bool switch2;
    int count;
    int y;
};

struct Shelf {
    /// In reading order: left to right, top to bottom, section by section.
    std::vector<ShelfSpot> spots;
    std::vector<ShelfHeading> headings;
    int height = 0;
};

/// The height of a section's heading.
inline constexpr int ShelfHeadingHeight = 40;

/// Lays the games out in rows of `columns` cells, in the order given. With `sections`, Switch 2
/// games come first under their heading, then Switch games under theirs, each section starting a
/// new row; an empty section has no heading.
[[nodiscard]] inline Shelf ComputeShelf(std::span<const ShelfEntry> entries, int columns,
                                        int cell_width, int cell_height, bool sections) {
    columns = std::max(1, columns);
    Shelf shelf;
    int y = 0;
    const auto place = [&](bool only_switch2, bool filter) {
        int column = 0;
        for (const ShelfEntry& entry : entries) {
            if (filter && entry.switch2 != only_switch2) {
                continue;
            }
            shelf.spots.push_back({entry.row, column * cell_width, y});
            if (++column == columns) {
                column = 0;
                y += cell_height;
            }
        }
        if (column != 0) {
            y += cell_height;
        }
    };
    if (!sections) {
        place(false, false);
    } else {
        for (const bool switch2 : {true, false}) {
            const auto count = std::count_if(entries.begin(), entries.end(),
                                             [switch2](const ShelfEntry& entry) {
                                                 return entry.switch2 == switch2;
                                             });
            if (count == 0) {
                continue;
            }
            shelf.headings.push_back({switch2, static_cast<int>(count), y});
            y += ShelfHeadingHeight;
            place(switch2, true);
        }
    }
    shelf.height = y;
    return shelf;
}

} // namespace Yuzu

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>

namespace Common {

struct OverlayOffset {
    int x;
    int y;

    constexpr bool operator==(const OverlayOffset&) const = default;
};

/// Where the performance overlay sits by default, from the main window's top left corner: under
/// the menu bar, clear of the window's edge.
inline constexpr OverlayOffset DefaultOverlayOffset{25, 75};

/// A saved offset brought back inside the window, so an overlay dragged somewhere a smaller
/// window (or another screen) no longer reaches can still be seen and grabbed.
[[nodiscard]] constexpr OverlayOffset ClampOverlayOffset(OverlayOffset offset, int window_width,
                                                         int window_height, int overlay_width,
                                                         int overlay_height) {
    const int max_x = (std::max)(0, window_width - overlay_width);
    const int max_y = (std::max)(0, window_height - overlay_height);
    return {std::clamp(offset.x, 0, max_x), std::clamp(offset.y, 0, max_y)};
}

} // namespace Common

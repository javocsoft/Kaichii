// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace VideoCore {

/// The slope term of a depth bias, factor * max(|dz/dx|, |dz/dy|), is measured per pixel. A target
/// drawn at `up_factor` times the guest's resolution has `up_factor` host pixels per guest pixel,
/// so its slope per pixel, and the game's slope bias with it, changes by 1 / up_factor: halved at
/// 2x (fine dark stripes where a shadow pass relies on it), doubled at 0.5x. This is the factor
/// that keeps the guest's bias. Only when the viewport is rescaled with the target.
[[nodiscard]] constexpr float RescaledSlopeScale(bool rescaling, bool viewport_scaled,
                                                 float up_factor) {
    return rescaling && viewport_scaled ? up_factor : 1.0f;
}

} // namespace VideoCore

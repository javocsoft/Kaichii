// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>

namespace Vulkan {

/// Slots a samples query may span without a reset or report before its count is dropped, as a
/// reset would. Ocarina of Time's title screen and file select reached 16,384 slots over 64 banks;
/// its gameplay near water and windows passes a single 256-slot bank (about 300 slots), where
/// dropping the count made the water and the castle windows vanish every few seconds.
inline constexpr std::size_t MaxUnreportedSamplesSlots = 256 * 32;

constexpr bool DropsUnreportedSamples(std::size_t slots) {
    return slots > MaxUnreportedSamplesSlots;
}

} // namespace Vulkan

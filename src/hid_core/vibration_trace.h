// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdlib>

#include "common/common_types.h"

/// EDEN_NX2_VIBRATION_TRACE: logs the vibration values a game sends and what the emulator passes on
/// to the controller.
namespace Core::HID::VibrationTrace {

/// Whether EDEN_NX2_VIBRATION_TRACE is set; read once.
[[nodiscard]] inline bool Enabled() {
    static const bool enabled = std::getenv("EDEN_NX2_VIBRATION_TRACE") != nullptr;
    return enabled;
}

/// Whether the `count`th line (from 1) is logged: all of the first 4,000, then one in 100.
[[nodiscard]] constexpr bool Keep(u64 count) {
    return count <= 4000 || count % 100 == 0;
}

} // namespace Core::HID::VibrationTrace

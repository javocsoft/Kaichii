// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>

#include "common/common_types.h"

namespace Common {

struct MemoryInfo {
    u64 TotalPhysicalMemory{};
    u64 TotalSwapMemory{};
};

/**
 * Gets the memory info of the host system
 * @return Reference to a MemoryInfo struct with the physical and swap memory sizes in bytes
 */
[[nodiscard]] const MemoryInfo& GetMemInfo();

/// Physical RAM the host can currently make available without swapping (zero if unknown).
[[nodiscard]] u64 GetAvailablePhysicalMemory();

[[nodiscard]] constexpr bool IsLowOnMemory(u64 available, u64 total) {
    return available != 0 && available < (std::min<u64>)(total / 4, u64{4} << 30);
}

/// Cached for one second so a burst of texture uploads does not query the OS for each range.
[[nodiscard]] bool IsHostLowOnMemory();

} // namespace Common

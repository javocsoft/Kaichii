// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>

#include "common/common_types.h"

namespace VideoCommon {

/// Device memory in use and the driver's budget for it, published by the texture cache once a
/// frame for the frontend to read (0 until a renderer that reports them has run).
struct VideoMemoryStatus {
    std::atomic<u64> used{};
    std::atomic<u64> budget{};
    /// Allocated outside video memory (system memory), where images go once video memory is full.
    std::atomic<u64> host_used{};
};

inline VideoMemoryStatus g_video_memory_status;

} // namespace VideoCommon

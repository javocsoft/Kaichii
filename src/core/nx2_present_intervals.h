// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <chrono>
#include <vector>

#include "common/common_types.h"
#include "core/core.h"

namespace Core {

/// A Switch 2 game's present-to-present intervals (ms) over the last `window_ms`, oldest first.
/// Empty for other games, which don't fill the present history, and when the newest present is
/// more than two seconds old (paused or loading).
inline std::vector<double> RecentPresentIntervals(double window_ms) {
    std::vector<double> intervals;
    u64 last = Core::g_nx2_present_count.load(std::memory_order_acquire);
    if (last >= 1 && Core::Nx2PresentTime(last).load(std::memory_order_relaxed) == 0) {
        --last; // the newest present may not have stored its time yet
    }
    if (last < 2) {
        return intervals;
    }
    u64 newer = Core::Nx2PresentTime(last).load(std::memory_order_relaxed);
    const u64 now = static_cast<u64>(
        std::chrono::steady_clock::now().time_since_epoch() / std::chrono::nanoseconds{1});
    if (newer == 0 || now - newer > 2'000'000'000ULL) {
        return intervals;
    }
    const u64 first = last > Core::Nx2PresentHistory - 1 ? last - (Core::Nx2PresentHistory - 1) : 1;
    double total = 0.0;
    for (u64 number = last - 1; number >= first && total < window_ms; --number) {
        const u64 older = Core::Nx2PresentTime(number).load(std::memory_order_relaxed);
        if (older == 0 || older >= newer) {
            break;
        }
        const double ms = static_cast<double>(newer - older) / 1e6;
        intervals.push_back(ms);
        total += ms;
        newer = older;
        if (number == first) {
            break;
        }
    }
    std::ranges::reverse(intervals);
    return intervals;
}


} // namespace Core

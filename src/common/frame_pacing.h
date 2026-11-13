// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/common_types.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <span>
#include <vector>

namespace Common {

/// How evenly frames arrive, from present-to-present intervals. An average frame rate hides the
/// 100-400 ms stalls a player notices; these numbers show them.
struct FramePacing {
    size_t frames = 0;
    double fps = 0.0;          ///< Over the last `recent_ms` of the intervals
    double average_fps = 0.0;  ///< Over all of them
    double median_ms = 0.0;
    double p99_ms = 0.0;       ///< 99th percentile frame time
    double low_1pct_fps = 0.0; ///< 1000 / p99_ms: the frame rate of the slowest 1% of frames
    double min_ms = 0.0;
    double max_ms = 0.0;
    size_t stutters = 0;       ///< Frames longer than twice the median and than 40 ms
};

/// @param intervals_ms Present-to-present intervals in milliseconds, oldest first.
/// @param recent_ms    How far back `fps` looks.
inline FramePacing ComputeFramePacing(std::span<const double> intervals_ms,
                                      double recent_ms = 1000.0) {
    FramePacing pacing;
    if (intervals_ms.empty()) {
        return pacing;
    }
    pacing.frames = intervals_ms.size();
    const double total_ms = std::accumulate(intervals_ms.begin(), intervals_ms.end(), 0.0);
    pacing.average_fps = total_ms > 0.0 ? 1000.0 * static_cast<double>(pacing.frames) / total_ms
                                        : 0.0;
    double recent_total = 0.0;
    size_t recent_frames = 0;
    for (auto it = intervals_ms.rbegin(); it != intervals_ms.rend() && recent_total < recent_ms;
         ++it) {
        recent_total += *it;
        ++recent_frames;
    }
    pacing.fps = recent_total > 0.0 ? 1000.0 * static_cast<double>(recent_frames) / recent_total
                                    : 0.0;
    std::vector<double> sorted(intervals_ms.begin(), intervals_ms.end());
    std::ranges::sort(sorted);
    // Nearest rank: the smallest value with at least `fraction` of the frames at or below it, so
    // the slowest 1% of 297 frames (3 of them) sets the 99th percentile.
    const auto rank = [&sorted](double fraction) {
        const double position = std::ceil(fraction * static_cast<double>(sorted.size()));
        const size_t index = position < 1.0 ? 0 : static_cast<size_t>(position) - 1;
        return sorted[(std::min)(index, sorted.size() - 1)];
    };
    pacing.min_ms = sorted.front();
    pacing.max_ms = sorted.back();
    pacing.median_ms = rank(0.5);
    pacing.p99_ms = rank(0.99);
    pacing.low_1pct_fps = pacing.p99_ms > 0.0 ? 1000.0 / pacing.p99_ms : 0.0;
    const double stutter_ms = (std::max)(2.0 * pacing.median_ms, 40.0);
    pacing.stutters = static_cast<size_t>(
        std::ranges::count_if(intervals_ms, [stutter_ms](double ms) { return ms > stutter_ms; }));
    return pacing;
}

/// Frames generated a second between two readings of a renderer's generated-frame counter, taken
/// `seconds` apart. Zero when the counter went back (a new game) or no time passed.
[[nodiscard]] constexpr double GeneratedFrameRate(u64 before, u64 after, double seconds) {
    if (after < before || seconds <= 0.0) {
        return 0.0;
    }
    return static_cast<double>(after - before) / seconds;
}

} // namespace Common

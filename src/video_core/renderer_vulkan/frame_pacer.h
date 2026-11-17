// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <optional>

#include "common/common_types.h"

namespace Vulkan {

class FramePacer {
public:
    using Clock = std::chrono::steady_clock;

    [[nodiscard]] std::optional<Clock::time_point> Next(Clock::time_point now, double target_fps,
                                                        u32 game_frame_presents) {
        if (target_fps != last_target_fps) {
            frame_interval = std::chrono::duration_cast<Clock::duration>(
                std::chrono::duration<double>{1.0 / target_fps});
            max_frames = static_cast<u64>(target_fps * 0.1);
            last_target_fps = target_fps;
            elapsed_frames = 0.0;
            start_time = now;
        }

        elapsed_frames += 1.0 / static_cast<double>(game_frame_presents);
        const auto deadline = start_time +
                              std::chrono::duration_cast<Clock::duration>(frame_interval * elapsed_frames);
        if (deadline >= now) {
            return deadline;
        }
        if (elapsed_frames > static_cast<double>(max_frames)) {
            elapsed_frames = 0.0;
            start_time = now;
        }
        return std::nullopt;
    }

private:
    Clock::duration frame_interval{};
    Clock::time_point start_time{};
    double last_target_fps{};
    double elapsed_frames{};
    u64 max_frames{};
};

} // namespace Vulkan

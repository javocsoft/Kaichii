// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <chrono>

#include <catch2/catch_test_macros.hpp>

#include "video_core/renderer_vulkan/frame_pacer.h"

TEST_CASE("Frame pacing counts game frames with generated presents", "[video_core][frame_pacer]") {
    using Clock = Vulkan::FramePacer::Clock;
    constexpr Clock::time_point start{std::chrono::seconds{1}};

    for (const double target_fps : {30.0, 60.0, 90.0, 120.0}) {
        const auto interval = std::chrono::duration_cast<Clock::duration>(
            std::chrono::duration<double>{1.0 / target_fps});
        for (const u32 presents : {1U, 2U, 3U, 4U}) {
            Vulkan::FramePacer pacer;
            auto now = start;
            for (u32 frame = 0; frame < 90; ++frame) {
                for (u32 present = 0; present < presents; ++present) {
                    const auto deadline = pacer.Next(now, target_fps, presents);
                    REQUIRE(deadline);
                    now = *deadline;
                }
            }
            REQUIRE(now - start >= interval * 90 - std::chrono::microseconds{1});
            REQUIRE(now - start <= interval * 91);
        }
    }
}

TEST_CASE("Frame pacing keeps its schedule when generation count changes",
          "[video_core][frame_pacer]") {
    using Clock = Vulkan::FramePacer::Clock;
    constexpr Clock::time_point start{std::chrono::seconds{1}};
    Vulkan::FramePacer pacer;
    auto now = start;
    for (const u32 presents : {1U, 3U, 2U}) {
        for (u32 frame = 0; frame < 3; ++frame) {
            for (u32 present = 0; present < presents; ++present) {
                const auto deadline = pacer.Next(now, 60.0, presents);
                REQUIRE(deadline);
                now = *deadline;
            }
        }
    }
    const auto expected = std::chrono::duration_cast<Clock::duration>(std::chrono::seconds{1}) * 9 / 60;
    REQUIRE(now - start >= expected - std::chrono::microseconds{1});
    REQUIRE(now - start <= expected + std::chrono::microseconds{1});
}

TEST_CASE("Late frame pacing restarts after its catch-up window", "[video_core][frame_pacer]") {
    using Clock = Vulkan::FramePacer::Clock;
    constexpr Clock::time_point start{std::chrono::seconds{1}};
    Vulkan::FramePacer pacer;
    REQUIRE(pacer.Next(start, 60.0, 1));
    const auto late = start + std::chrono::seconds{1};
    for (int frame = 0; frame < 6; ++frame) {
        REQUIRE_FALSE(pacer.Next(late, 60.0, 1));
    }
    REQUIRE(pacer.Next(late, 60.0, 1) ==
            late + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>{1.0 / 60.0}));
}

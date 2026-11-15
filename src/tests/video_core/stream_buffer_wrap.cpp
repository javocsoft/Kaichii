// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "video_core/renderer_vulkan/stream_buffer_wrap.h"

TEST_CASE("A refused stream buffer wrap leaves the next upload behind the active first region",
          "[video_core][staging]") {
    const Vulkan::StreamBufferPosition before{96, 120, 112};
    const auto refused = Vulkan::AdvanceStreamBuffer(before, 16, 128, true);
    REQUIRE_FALSE(refused);

    const auto granted = Vulkan::AdvanceStreamBuffer(before, 16, 128, false);
    REQUIRE(granted);
    CHECK(granted->used == 0);
    CHECK(granted->next == 0);
    CHECK(granted->free == 16);

    const auto unwrapped = Vulkan::AdvanceStreamBuffer({32, 48, 64}, 16, 128, false);
    REQUIRE(unwrapped);
    CHECK(unwrapped->used == 32);
    CHECK(unwrapped->next == 48);
    CHECK(unwrapped->free == 64);
}

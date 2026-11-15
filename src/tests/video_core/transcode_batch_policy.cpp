// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "video_core/renderer_vulkan/transcode_batch_policy.h"

TEST_CASE("Integrated GPUs decode large ASTC arrays within the scratch budget",
          "[video_core][transcode]") {
    constexpr u64 budget = u64{256} << 20;
    CHECK(Vulkan::SelectAstcBatchLayers(1365, 1365, 11, 90, budget, false) == 90);
    CHECK(Vulkan::SelectAstcBatchLayers(1365, 1365, 11, 90, budget, true) == 23);
    CHECK(Vulkan::SelectAstcBatchLayers(16, 16, 1, 90, budget, true) == 90);
    CHECK(Vulkan::SelectAstcBatchLayers(8192, 8192, 1, 1, budget, true) == 1);
}

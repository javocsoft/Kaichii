// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "video_core/renderer_vulkan/descriptor_frame_policy.h"

TEST_CASE("Descriptor frames submit before reusing chunks from the same command buffer",
          "[video_core][descriptor_buffer]") {
    CHECK_FALSE(Vulkan::DescriptorFrameNeedsSubmit(7, 8));
    CHECK(Vulkan::DescriptorFrameNeedsSubmit(8, 8));
    CHECK(Vulkan::DescriptorFrameNeedsSubmit(9, 8));
}

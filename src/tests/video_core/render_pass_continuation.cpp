// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "video_core/renderer_vulkan/render_pass_continuation.h"

namespace {
struct Extent {
    unsigned width;
    unsigned height;
};
const int handles[4]{};
const int* const base_pass = &handles[0];
const int* const clear_variant = &handles[1];
const int* const first = &handles[2];
const int* const second = &handles[3];
const int* const no_pass = nullptr;
constexpr Extent Area{1280, 720};
} // namespace

TEST_CASE("Tilers keep drawing in the pass a deferred clear began", "[render_pass_continuation]") {
    CHECK(Vulkan::ContinuesRenderPass(true, clear_variant, first, Area, base_pass, first, Area));
    CHECK(Vulkan::ContinuesRenderPass(true, base_pass, first, Area, base_pass, first, Area));
}

TEST_CASE("Immediate-mode GPUs begin the framebuffer's own pass after a deferred clear",
          "[render_pass_continuation]") {
    CHECK_FALSE(
        Vulkan::ContinuesRenderPass(false, clear_variant, first, Area, base_pass, first, Area));
    CHECK(Vulkan::ContinuesRenderPass(false, base_pass, first, Area, base_pass, first, Area));
}

TEST_CASE("Another framebuffer or area begins a new pass", "[render_pass_continuation]") {
    for (const bool tiler : {false, true}) {
        CHECK_FALSE(
            Vulkan::ContinuesRenderPass(tiler, base_pass, first, Area, base_pass, second, Area));
        CHECK_FALSE(Vulkan::ContinuesRenderPass(tiler, base_pass, first, Area, base_pass, first,
                                                Extent{640, 720}));
        CHECK_FALSE(Vulkan::ContinuesRenderPass(tiler, base_pass, first, Area, base_pass, first,
                                                Extent{1280, 360}));
    }
}

TEST_CASE("A pass that ended is begun again for the same framebuffer",
          "[render_pass_continuation]") {
    // Ending a pass clears only the open pass, so the framebuffer and area it had are stale.
    for (const bool tiler : {false, true}) {
        CHECK_FALSE(Vulkan::ContinuesRenderPass(tiler, no_pass, first, Area, base_pass, first, Area));
    }
}

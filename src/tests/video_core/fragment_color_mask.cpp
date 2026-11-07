// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <catch2/catch_test_macros.hpp>

#include "video_core/renderer_vulkan/fragment_color_mask.h"

TEST_CASE("Fragment output masks hold the components each render target stores",
          "[fragment_color_mask]") {
    std::array<u8, 8> components{};
    // No fragment program, or one without color exports (a depth-only draw): nothing is written.
    CHECK(Vulkan::FragmentColorOutputMask(components) == 0);
    components[0] = 0b0111; // RGB of RT0
    components[2] = 0b1000; // A of RT2
    components[7] = 0b1111;
    CHECK(Vulkan::FragmentColorOutputMask(components) == 0xF000'0807U);
    // Bits past A are not components.
    components.fill(0xFF);
    CHECK(Vulkan::FragmentColorOutputMask(components) == 0xFFFF'FFFFU);
}

TEST_CASE("Static and dynamic fragment masks agree for every guest component mask",
          "[fragment_color_mask]") {
    Vulkan::FragmentColorMaskState state;
    Tegra::Engines::Maxwell3D::DirtyState::Flags flags{};
    for (const u32 outputs : {~0U, 0U, 0x0000'0807U, ~0U}) {
        state.Update(outputs, flags);
        for (size_t render_target = 0; render_target < 8; ++render_target) {
            for (u32 mask = 0; mask < 16; ++mask) {
                CAPTURE(outputs, render_target, mask);
                const u32 expected = mask & ((outputs >> (render_target * 4)) & 0xF);
                CHECK(Vulkan::FragmentColorWriteMask(mask, outputs, render_target) == expected);
                CHECK(state.Apply(mask, render_target) == expected);
            }
        }
    }
}

TEST_CASE("Fragment output changes invalidate both blending and color masks",
          "[fragment_color_mask]") {
    Vulkan::FragmentColorMaskState state;
    Tegra::Engines::Maxwell3D::DirtyState::Flags flags{};
    // Without a pipeline every component the guest enables is written.
    state.Update(~0U, flags);
    CHECK(flags.none());
    CHECK(state.Apply(15, 3) == 15);

    // The guest masks stay fixed while pipelines output different components.
    for (const u32 outputs : {0U, 0x0000'0001U, 0U, 0x0000'0008U}) {
        flags.reset();
        flags[Vulkan::Dirty::Scissors] = true;
        state.Update(outputs, flags);
        CHECK(flags[Vulkan::Dirty::Blending]);
        CHECK(flags[Vulkan::Dirty::ColorMask]);
        CHECK(flags[Vulkan::Dirty::Scissors]);
        CHECK(flags.count() == 3);
        CHECK(state.Apply(15, 0) == outputs);
        CHECK(state.Apply(15, 1) == 0);

        // After those flags have been consumed, rebinding the same outputs is a no-op.
        flags.reset();
        state.Update(outputs, flags);
        CHECK(flags.none());
    }
}

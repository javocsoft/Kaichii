// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <span>

#include "common/common_types.h"
#include "video_core/renderer_vulkan/vk_state_tracker.h"

namespace Vulkan {

/// Color components of each render target the fragment program writes, 4 bits per target (bit 0 = R
/// ... bit 3 = A), from the components each one stores. Nothing without a fragment program.
inline u32 FragmentColorOutputMask(std::span<const u8> components) {
    u32 mask = 0;
    for (size_t render_target = 0; render_target < (std::min)(components.size(), size_t{8});
         ++render_target) {
        mask |= u32{components[render_target] & 0xFU} << (render_target * 4);
    }
    return mask;
}

/// The guest leaves render target components the fragment program doesn't output untouched, while
/// Vulkan writes undefined values to them (AMD writes whatever its export registers hold).
inline u32 FragmentColorWriteMask(u32 guest_mask, u32 output_mask, size_t render_target) {
    return guest_mask & ((output_mask >> (render_target * 4)) & 0xFU);
}

/// The output mask the dynamic color write masks were last recorded with.
class FragmentColorMaskState {
public:
    void Update(u32 output_mask, Tegra::Engines::Maxwell3D::DirtyState::Flags& flags) {
        if (output_mask == last_output_mask) {
            return;
        }
        last_output_mask = output_mask;
        // A pipeline change can require new dynamic masks without a guest register write.
        flags[Dirty::Blending] = true;
        flags[Dirty::ColorMask] = true;
    }

    u32 Apply(u32 guest_mask, size_t render_target) const {
        return FragmentColorWriteMask(guest_mask, last_output_mask, render_target);
    }

private:
    u32 last_output_mask{~0U};
};

} // namespace Vulkan

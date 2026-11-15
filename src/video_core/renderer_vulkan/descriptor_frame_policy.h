// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/common_types.h"

namespace Vulkan {

[[nodiscard]] constexpr bool DescriptorFrameNeedsSubmit(u64 next_frame_tick,
                                                         u64 current_tick) {
    return next_frame_tick >= current_tick;
}

} // namespace Vulkan

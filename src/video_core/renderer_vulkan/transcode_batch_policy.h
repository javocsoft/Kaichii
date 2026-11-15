// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>

#include "common/common_types.h"

namespace Vulkan {

[[nodiscard]] constexpr u32 SelectAstcBatchLayers(u32 width, u32 height, u32 levels, u32 layers,
                                                   u64 budget_bytes, bool integrated) {
    if (!integrated || layers <= 1) {
        return layers;
    }
    u64 bytes_per_layer{};
    for (u32 level = 0; level < levels; ++level) {
        bytes_per_layer += u64{(std::max)(width >> level, 1U)} *
                           (std::max)(height >> level, 1U) * 4;
    }
    u32 batch = layers;
    while (batch > 1 && bytes_per_layer * batch > budget_bytes) {
        batch = batch / 2 + batch % 2;
    }
    return batch;
}

} // namespace Vulkan

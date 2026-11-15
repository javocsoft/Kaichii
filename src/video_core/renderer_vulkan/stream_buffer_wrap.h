// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>

namespace Vulkan {

struct StreamBufferPosition {
    size_t used;
    size_t next;
    size_t free;
};

[[nodiscard]] constexpr std::optional<StreamBufferPosition> AdvanceStreamBuffer(
    StreamBufferPosition position, size_t size, size_t capacity, bool first_regions_active) {
    if (position.next + size >= capacity) {
        if (first_regions_active) {
            return std::nullopt;
        }
        return StreamBufferPosition{0, 0, size};
    }
    position.free = (std::max)(position.free, position.next + size);
    return position;
}

} // namespace Vulkan

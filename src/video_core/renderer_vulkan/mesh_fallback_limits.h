// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>

namespace Vulkan {

/// Whether a graphics pipeline's storage buffers fit its descriptor set. The mesh fallback binds
/// its output buffer, and for task draws the payload, offset and argument buffers too, into the
/// same set as the guest's. A limit of zero means none.
[[nodiscard]] constexpr bool FitsStorageBufferSetLimit(std::size_t guest_buffers,
                                                       std::size_t auxiliary_buffers,
                                                       std::size_t set_limit) noexcept {
    return set_limit == 0 ||
           (guest_buffers <= set_limit && auxiliary_buffers <= set_limit - guest_buffers);
}

} // namespace Vulkan

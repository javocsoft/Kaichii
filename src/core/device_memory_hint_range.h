// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <cstddef>

#include "common/common_types.h"

namespace Core {

[[nodiscard]] constexpr size_t ClipColdHintSize(DAddr address, size_t size, DAddr end) noexcept {
    if (address >= end) {
        return 0;
    }
    return static_cast<size_t>((std::min)(DAddr{size}, end - address));
}

} // namespace Core

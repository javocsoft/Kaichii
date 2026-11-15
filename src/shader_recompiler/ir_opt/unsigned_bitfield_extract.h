// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/common_types.h"

namespace Shader::Optimization {

[[nodiscard]] constexpr u32 ExtractUnsignedBitfield(u32 base, u32 shift, u32 count) {
    if (count == 0) {
        return 0;
    }
    const u32 shifted = base >> shift;
    const u32 mask = count == 32 ? ~u32{} : (u32{1} << count) - 1;
    return shifted & mask;
}

} // namespace Shader::Optimization

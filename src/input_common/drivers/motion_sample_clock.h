// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>

#include "common/common_types.h"

namespace InputCommon {

class MotionSampleClock {
public:
    [[nodiscard]] std::optional<u64> Advance(u64 nanoseconds) {
        if (previous == 0 || nanoseconds < previous) {
            previous = nanoseconds;
            return std::nullopt;
        }
        const u64 microseconds = (nanoseconds - previous) / 1000;
        previous = nanoseconds;
        return microseconds;
    }

    void Reset() { previous = 0; }

private:
    u64 previous{};
};

} // namespace InputCommon

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <cstddef>

#include "common/common_types.h"
#include "common/literals.h"

namespace VideoCommon {

/// How much harder the texture collector works when its cache runs away past the critical memory.
struct CollectorEscalation {
    size_t extra_iterations = 0; ///< Images it may delete this frame beyond the usual 40
    u64 ticks_to_destroy = 0;    ///< When nonzero, delete images unused for this many frames
};

/// Breaking pots in Ocarina of Time's Hyrule Castle makes ~55 images a frame (Zortos PR 1306): the
/// cache grew 4 GiB in two seconds until the driver crashed. Past `RunawayMargin` beyond the
/// critical memory the collector deletes one more image per 2 MiB over (up to 1024 more), and from
/// 1 GiB over it also takes images unused for 3 frames instead of 10. Within the margin nothing
/// changes: a 4 GB card sits 150-210 MiB past the critical memory in Kokiri Forest, where deleting
/// more lost walking frames for nothing.
inline constexpr u64 RunawayMargin = u64{512} << 20;

constexpr CollectorEscalation EscalateCollector(bool aggressive, u64 used, u64 critical,
                                                u64 min_age) {
    using namespace Common::Literals;
    if (!aggressive || used <= critical + RunawayMargin) {
        return {};
    }
    const u64 over = used - critical;
    CollectorEscalation escalation;
    escalation.extra_iterations = static_cast<size_t>((std::min)(over / 2_MiB, u64{1024}));
    if (over >= 1_GiB) {
        escalation.ticks_to_destroy = (std::max)(min_age, u64{3});
    }
    return escalation;
}

} // namespace VideoCommon

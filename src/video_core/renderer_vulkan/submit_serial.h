// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/common_types.h"

namespace Vulkan {

/// Counts the commands recorded into the scheduler's command buffers, to tell whether a submit
/// would send anything.
class SubmitSerial {
public:
    void Recorded() noexcept {
        ++recorded;
    }

    void Submitted() noexcept {
        submitted = recorded;
    }

    /// Whether anything was recorded since the last submit, or is held to be recorded later
    /// (`held_back`: a deferred clear reaches the command buffer only at the next record).
    [[nodiscard]] bool RecordedSinceSubmit(bool held_back) const noexcept {
        return recorded != submitted || held_back;
    }

private:
    u64 recorded{};
    u64 submitted{};
};

} // namespace Vulkan

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <chrono>

namespace InputCommon {

/// Some controllers stop sending sensor reports while they stay connected: the 2026 Steam
/// Controller after it reconnects, and when Steam Input changes its sensor mode. Their sensors
/// are restarted after a second of silence, backing off to eight seconds while they stay silent.
class MotionStallWatch {
public:
    using Clock = std::chrono::steady_clock;
    static constexpr std::chrono::milliseconds InitialRetry{1000};
    static constexpr std::chrono::milliseconds MaxRetry{8000};

    /// A sensor report arrived.
    void Reported(Clock::time_point now) {
        last_report = now;
        retry = InitialRetry;
    }

    /// The sensors were enabled: give them a full retry period to report.
    void Enabled(Clock::time_point now) {
        last_report = now;
    }

    /// Whether the sensors have been silent long enough to restart them now.
    [[nodiscard]] bool Stalled(Clock::time_point now) {
        if (now - last_report < retry) {
            return false;
        }
        retry = (std::min)(retry * 2, MaxRetry);
        return true;
    }

private:
    Clock::time_point last_report{};
    std::chrono::milliseconds retry{InitialRetry};
};

} // namespace InputCommon

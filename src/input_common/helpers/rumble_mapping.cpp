// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <cmath>

#include "input_common/helpers/rumble_mapping.h"

namespace InputCommon {

namespace {

f32 FrequencyScale(f32 frequency, f32 start, f32 width) {
    if (frequency <= start) {
        return 1.0f;
    }
    return (std::max)(1.0f - (frequency - start) / width, 0.3f);
}

} // Anonymous namespace

f32 ShapeRumbleAmplitude(f32 amplitude, f32 curve_factor, bool silence_faint) {
    const f32 curved = (amplitude + std::pow(amplitude, curve_factor)) * 0.5f;
    if (!silence_faint || curve_factor >= 1.0f) {
        return curved;
    }
    const f32 fade = std::clamp((amplitude - FaintRumbleStart) / (FaintRumbleEnd - FaintRumbleStart),
                                0.0f, 1.0f);
    return curved * fade;
}

MotorStrengths MapRumbleByBand(const Common::Input::VibrationStatus& vibration, f32 curve_factor,
                               bool silence_faint) {
    return {
        .low = ShapeRumbleAmplitude(vibration.low_amplitude, curve_factor, silence_faint) *
               FrequencyScale(vibration.low_frequency, 140.0f, 400.0f),
        .high = ShapeRumbleAmplitude(vibration.high_amplitude, curve_factor, silence_faint) *
                FrequencyScale(vibration.high_frequency, 200.0f, 700.0f),
    };
}

MotorStrengths StrongerPerMotor(const MotorStrengths& left, const MotorStrengths& right) {
    return {
        .low = (std::max)(left.low, right.low),
        .high = (std::max)(left.high, right.high),
    };
}

} // namespace InputCommon

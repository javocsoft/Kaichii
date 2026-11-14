// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/common_types.h"
#include "common/input.h"

namespace InputCommon {

// Strengths in [0, 1] for the low frequency (heavy) and high frequency (light) motors of a
// controller with plain two motor rumble.
struct MotorStrengths {
    f32 low{};
    f32 high{};
};

// Shapes an HD rumble amplitude for a motor. curve_factor below 1 lifts small amplitudes so a
// motor's weight starts to turn; 1 leaves the amplitude as it is (HD rumble). With a lifting curve,
// amplitudes up to FaintRumbleEnd fade in from nothing: games keep up a faint background rumble
// (Donkey Kong Bananza, near lava, mostly under 0.02) that HD rumble barely makes felt, and the
// lift would turn it into a constant buzz on a two motor pad.
f32 ShapeRumbleAmplitude(f32 amplitude, f32 curve_factor);

// Amplitudes at or below this are silent with a lifting curve.
inline constexpr f32 FaintRumbleStart = 0.015f;
// Amplitudes at or above this get the full curve.
inline constexpr f32 FaintRumbleEnd = 0.035f;

// Each band drives its own motor, weakened as its frequency rises so some of the frequency can
// still be felt.
MotorStrengths MapRumbleByBand(const Common::Input::VibrationStatus& vibration, f32 curve_factor);

// The stronger of two sides' strengths on each motor, for a pad that gets both sides of the
// emulated controller.
MotorStrengths StrongerPerMotor(const MotorStrengths& left, const MotorStrengths& right);

} // namespace InputCommon

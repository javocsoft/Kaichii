// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cmath>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "input_common/helpers/rumble_mapping.h"

namespace {

Common::Input::VibrationStatus Bands(f32 low_amplitude, f32 low_frequency, f32 high_amplitude,
                                     f32 high_frequency) {
    return {
        .low_amplitude = low_amplitude,
        .low_frequency = low_frequency,
        .high_amplitude = high_amplitude,
        .high_frequency = high_frequency,
        .type = Common::Input::VibrationAmplificationType::Linear,
    };
}

f32 Curve(f32 amplitude, f32 factor) {
    return (amplitude + std::pow(amplitude, factor)) * 0.5f;
}

} // Anonymous namespace

TEST_CASE("Rumble by band keeps the band to motor mapping", "[input_common]") {
    using Catch::Approx;

    const auto strengths = InputCommon::MapRumbleByBand(Bands(0.5f, 160.0f, 0.4f, 900.0f), 1.0f);
    REQUIRE(strengths.low == Approx(0.5f * 0.95f));
    REQUIRE(strengths.high == Approx(0.4f * 0.3f));
}

TEST_CASE("Faint background rumble stays silent on a two motor pad", "[input_common]") {
    using Catch::Approx;
    using InputCommon::FaintRumbleEnd;
    using InputCommon::FaintRumbleStart;
    using InputCommon::ShapeRumbleAmplitude;

    // Donkey Kong Bananza's background rumble near lava: the old curve made 0.005 about 8%.
    REQUIRE(Curve(0.005f, 0.35f) > 0.07f);
    REQUIRE(ShapeRumbleAmplitude(0.005f, 0.35f) == 0.0f);
    REQUIRE(ShapeRumbleAmplitude(FaintRumbleStart, 0.35f) == 0.0f);
    REQUIRE(ShapeRumbleAmplitude(0.0f, 0.5f) == 0.0f);

    // It fades in, and from FaintRumbleEnd up the curve is as before.
    const f32 middle = (FaintRumbleStart + FaintRumbleEnd) / 2;
    REQUIRE(ShapeRumbleAmplitude(middle, 0.35f) == Approx(Curve(middle, 0.35f) * 0.5f));
    REQUIRE(ShapeRumbleAmplitude(FaintRumbleEnd, 0.35f) == Approx(Curve(FaintRumbleEnd, 0.35f)));
    REQUIRE(ShapeRumbleAmplitude(0.3f, 0.35f) == Approx(Curve(0.3f, 0.35f)));
    REQUIRE(ShapeRumbleAmplitude(1.0f, 0.5f) == Approx(1.0f));

    // An HD rumble pad gets the amplitude as it is, faint or not.
    REQUIRE(ShapeRumbleAmplitude(0.005f, 1.0f) == Approx(0.005f));
}

TEST_CASE("A pad with both sides plays the stronger side on each motor", "[input_common]") {
    using Catch::Approx;

    const auto both = InputCommon::StrongerPerMotor({.low = 0.6f, .high = 0.1f},
                                                    {.low = 0.2f, .high = 0.4f});
    REQUIRE(both.low == Approx(0.6f));
    REQUIRE(both.high == Approx(0.4f));
}

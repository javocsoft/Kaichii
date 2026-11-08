// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <limits>
#include <random>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "audio_core/renderer/command/mix/mix_ramp.h"
#include "audio_core/renderer/command/mix/mix_sample.h"
#include "common/fixed_point.h"

namespace {

using namespace AudioCore::Renderer;

// The loops the mixers ran before, on Common::FixedPoint.
template <size_t Q>
s32 ReferenceMix(s32 output, s32 input, f32 volume_) {
    const Common::FixedPoint<64 - Q, Q> volume{volume_};
    return (output + input * volume).to_int();
}

template <size_t Q>
s32 ReferenceMixRamp(std::vector<s32>& output, const std::vector<s32>& input, f32 volume_,
                     f32 ramp_) {
    Common::FixedPoint<64 - Q, Q> volume{volume_};
    Common::FixedPoint<64 - Q, Q> sample{0};
    const Common::FixedPoint<64 - Q, Q> ramp{ramp_};
    for (size_t i = 0; i < output.size(); i++) {
        sample = input[i] * volume;
        output[i] = (output[i] + sample).to_int();
        if (ramp_ != 0.0f) {
            volume += ramp;
        }
    }
    return sample.to_int();
}

constexpr std::array<s32, 9> EdgeSamples{std::numeric_limits<s32>::min(), -32769, -32768, -1, 0,
                                        1, 32767, 32768, std::numeric_limits<s32>::max()};
constexpr std::array<f32, 9> EdgeVolumes{0.0f,  1.0f,      -1.0f,  0.5f,   0.70710678f,
                                         2.0f, 1.0f / 3.0f, -0.25f, 1e-6f};

template <size_t Q>
void CheckSamples() {
    std::mt19937 rng{Q};
    std::uniform_int_distribution<s32> samples{-(1 << 24), 1 << 24};
    std::uniform_real_distribution<f32> volumes{-4.0f, 4.0f};
    for (const s32 output : EdgeSamples) {
        for (const s32 input : EdgeSamples) {
            for (const f32 volume : EdgeVolumes) {
                CAPTURE(Q, output, input, volume);
                REQUIRE(MixSample<Q>(output, input, MixVolume<Q>(volume)) ==
                        ReferenceMix<Q>(output, input, volume));
            }
        }
    }
    for (int i = 0; i < 200'000; ++i) {
        const s32 output = samples(rng);
        const s32 input = samples(rng);
        const f32 volume = volumes(rng);
        CAPTURE(Q, output, input, volume);
        REQUIRE(MixSample<Q>(output, input, MixVolume<Q>(volume)) ==
                ReferenceMix<Q>(output, input, volume));
    }
}

template <size_t Q>
void CheckRamps() {
    std::mt19937 rng{Q + 1};
    std::uniform_int_distribution<s32> samples{-(1 << 24), 1 << 24};
    std::uniform_real_distribution<f32> volumes{-2.0f, 2.0f};
    for (int round = 0; round < 500; ++round) {
        std::vector<s32> input(240);
        std::vector<s32> output(240);
        for (size_t i = 0; i < input.size(); ++i) {
            input[i] = samples(rng);
            output[i] = samples(rng);
        }
        const f32 volume = volumes(rng);
        const f32 ramp = round % 3 == 0 ? 0.0f : volumes(rng) / 240.0f;
        std::vector<s32> expected = output;
        const s32 expected_last = ReferenceMixRamp<Q>(expected, input, volume, ramp);
        CAPTURE(Q, round, volume, ramp);
        REQUIRE(ApplyMixRamp<Q>(output, input, volume, ramp, static_cast<u32>(output.size())) ==
                expected_last);
        REQUIRE(output == expected);
    }
}

} // namespace

TEST_CASE("Mix samples match the fixed point arithmetic bit for bit", "[audio_core][mix]") {
    CheckSamples<15>();
    CheckSamples<23>();
}

TEST_CASE("Mix ramps match the fixed point arithmetic bit for bit", "[audio_core][mix]") {
    CheckRamps<15>();
    CheckRamps<23>();
}

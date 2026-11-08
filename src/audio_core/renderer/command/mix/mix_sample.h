// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/common_types.h"
#include "common/fixed_point.h"

namespace AudioCore::Renderer {

// The mixers' Q-bit fixed point arithmetic, as Common::FixedPoint<64 - Q, Q> does it per sample but
// in 64 bits: FixedPoint multiplies through a 128-bit type and the mix loops spent most of their
// time there.

/// A volume as FixedPoint<64 - Q, Q> stores it.
template <size_t Q>
s64 MixVolume(f32 volume) {
    return Common::FixedPoint<64 - Q, Q>{volume}.to_raw();
}

/// FixedPoint::to_int of a Q-bit value: rounds half up at the fraction's top bit.
template <size_t Q>
constexpr s32 MixToInt(s64 value) {
    constexpr s64 fraction = (s64{1} << Q) - 1;
    value += (value & fraction) >> 1;
    return static_cast<s32>(value >> Q);
}

/// `input * volume` as a Q-bit value, wrapping as FixedPoint's 128-bit product does when cut.
constexpr s64 MixProduct(s32 input, s64 volume) {
    return static_cast<s64>(static_cast<u64>(static_cast<s64>(input)) * static_cast<u64>(volume));
}

/// `(output + input * volume).to_int()` with `volume` from MixVolume.
template <size_t Q>
constexpr s32 MixSample(s32 output, s32 input, s64 volume) {
    return MixToInt<Q>(static_cast<s64>(output) * (s64{1} << Q) + MixProduct(input, volume));
}

} // namespace AudioCore::Renderer

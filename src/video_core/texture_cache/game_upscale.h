// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/common_types.h"

/// A game upscaling its own render targets with a compute shader, as Donkey Kong Bananza does with
/// its dynamic resolution scene. Below 1x the resolution scale would stack on the game's reduction,
/// and a compute shader's output can't be scaled, so the upscale would read a source at half the
/// size its filter taps assume and write blocks. While a game upscales, the texture cache keeps
/// the images smaller than the upscale's output at their native size.
namespace VideoCommon::GameUpscale {

/// What the texture cache knows about an image a compute dispatch reads.
struct Source {
    bool rescaleable; ///< ImageInfo::rescaleable: a render target, not a texture from the game
    bool colour;      ///< A colour format, not depth or stencil
    bool is_2d;       ///< A 2D image
    u32 width;        ///< Native width
    u32 height;       ///< Native height
};

/// Whether a compute dispatch that reads `source` and writes an image of `written_width` x
/// `written_height` (the largest it writes) upscales a render target.
[[nodiscard]] constexpr bool IsUpscale(const Source& source, u32 written_width,
                                       u32 written_height) {
    return source.rescaleable && source.colour && source.is_2d && source.width < written_width &&
           source.height < written_height;
}

/// While a game upscales to `output_width` x `output_height`, whether a 2D image of `width` x
/// `height` stays at its native size: it is smaller than the output in both directions, as the
/// reduced scene and what is drawn at a fraction of it are.
[[nodiscard]] constexpr bool HeldNative(u32 width, u32 height, u32 output_width,
                                        u32 output_height) {
    return width < output_width && height < output_height;
}

/// How many frames after the last upscale the images stay native, so a game that skips its
/// upscale for a frame doesn't send them back and forth.
inline constexpr u64 HoldFrames = 120;

/// Whether an upscale seen on frame `last_upscale` still holds images native on frame `frame`.
[[nodiscard]] constexpr bool Holding(u64 last_upscale, u64 frame) {
    return frame <= last_upscale + HoldFrames;
}

} // namespace VideoCommon::GameUpscale

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdlib>

#include "common/common_types.h"

/// EDEN_NX2_UPSCALE_SOURCE_NATIVE (experiment): below 1x, the colour images a compute shader
/// upscales from go back to their native size. EDEN_NX2_DRS_NATIVE (experiment): below 1x, while
/// the game upscales its dynamic resolution scene, every image smaller than the upscale's output
/// stays native, so the resolution scale doesn't stack on the game's own.
///
/// A compute shader's storage images can't be scaled, so a game upscaling its dynamic resolution
/// scene with one (Donkey Kong Bananza) writes a native image from a scaled source. Its filter taps
/// are placed in native texels, so on a source at half size neighbouring taps land on one texel and
/// the result comes out in blocks.
namespace VideoCommon::UpscaleSource {

/// What the texture cache knows about an image a compute dispatch reads.
struct Source {
    bool rescaled;      ///< Held at the resolution scale now
    bool colour;        ///< A colour format, not depth or stencil
    bool is_2d;         ///< A plain 2D image
    u32 width;          ///< Native width
    u32 height;         ///< Native height
    bool rescaleable{}; ///< ImageInfo::rescaleable: a render target, not a texture from the game
};

/// Whether a compute dispatch that reads `source` and writes an image of `written_width` x
/// `written_height` (the largest it writes) upscales it.
[[nodiscard]] constexpr bool IsUpscale(const Source& source, u32 written_width,
                                       u32 written_height) {
    return source.colour && source.is_2d && source.width < written_width &&
           source.height < written_height;
}

/// Whether `source` goes back to its native size when such a dispatch reads it.
[[nodiscard]] constexpr bool KeepNative(const Source& source, u32 written_width,
                                        u32 written_height) {
    return source.rescaled && IsUpscale(source, written_width, written_height);
}

/// EDEN_NX2_DRS_NATIVE: while the game upscales a render target to `output_width` x
/// `output_height`, whether a 2D image of `width` x `height` stays at its native size. Smaller
/// images are its dynamic resolution scene and what is drawn at a fraction of it.
[[nodiscard]] constexpr bool BelowUpscaleOutput(u32 width, u32 height, u32 output_width,
                                                u32 output_height) {
    return width < output_width && height < output_height;
}

/// How many frames after the last upscale EDEN_NX2_DRS_NATIVE still keeps those images native.
inline constexpr u64 DrsHoldFrames = 120;

/// Whether EDEN_NX2_UPSCALE_SOURCE_NATIVE is set; read once.
[[nodiscard]] inline bool Enabled() {
    static const bool enabled = std::getenv("EDEN_NX2_UPSCALE_SOURCE_NATIVE") != nullptr;
    return enabled;
}

/// Whether EDEN_NX2_DRS_NATIVE is set; read once.
[[nodiscard]] inline bool DrsNativeEnabled() {
    static const bool enabled = std::getenv("EDEN_NX2_DRS_NATIVE") != nullptr;
    return enabled;
}

} // namespace VideoCommon::UpscaleSource

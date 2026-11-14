// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdlib>

#include "common/common_types.h"

/// EDEN_NX2_UPSCALE_SOURCE_NATIVE (experiment): below 1x, the colour images a compute shader
/// upscales from go back to their native size.
///
/// A compute shader's storage images can't be scaled, so a game upscaling its dynamic resolution
/// scene with one (Donkey Kong Bananza) writes a native image from a scaled source. Its filter taps
/// are placed in native texels, so on a source at half size neighbouring taps land on one texel and
/// the result comes out in blocks.
namespace VideoCommon::UpscaleSource {

/// What the texture cache knows about an image a compute dispatch reads.
struct Source {
    bool rescaled; ///< Held at the resolution scale now
    bool colour;   ///< A colour format, not depth or stencil
    bool is_2d;    ///< A plain 2D image
    u32 width;     ///< Native width
    u32 height;    ///< Native height
};

/// Whether `source` goes back to its native size when a compute dispatch reads it and writes an
/// image of `written_width` x `written_height` (the largest it writes): the dispatch upscales it.
[[nodiscard]] constexpr bool KeepNative(const Source& source, u32 written_width,
                                        u32 written_height) {
    return source.rescaled && source.colour && source.is_2d && source.width < written_width &&
           source.height < written_height;
}

/// Whether EDEN_NX2_UPSCALE_SOURCE_NATIVE is set; read once.
[[nodiscard]] inline bool Enabled() {
    static const bool enabled = std::getenv("EDEN_NX2_UPSCALE_SOURCE_NATIVE") != nullptr;
    return enabled;
}

} // namespace VideoCommon::UpscaleSource

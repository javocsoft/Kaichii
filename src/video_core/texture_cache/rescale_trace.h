// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdlib>
#include <string_view>

#include "common/common_types.h"

/// EDEN_NX2_RESCALE_TRACE: with resolution scaling, logs every image the texture cache scales up
/// or down, why, and what keeps an image from following the scale.
namespace VideoCommon::RescaleTrace {

/// What made the texture cache scale an image up or down.
enum class Cause : u8 {
    RenderTargets,   ///< Bound as a render target with others.
    WrittenByShader, ///< Written by a shader as a storage image, which can't be scaled.
    Blit,            ///< A copy between a scaled and an unscaled image.
    BlitResolve,     ///< A multisample resolve, which needs both images at one size.
    Upscaler,        ///< The stand-in for a game's own upscaling network.
    JoinedImage,     ///< Joined into a larger image.
    NewImage,        ///< Created over images that were or weren't scaled.
    AliasedImages,   ///< Kept in step with images sharing its memory.
    UpscaleSource,   ///< Read by a compute shader that writes a larger image from it.
};

[[nodiscard]] constexpr std::string_view Name(Cause cause) {
    switch (cause) {
    case Cause::RenderTargets:
        return "render targets";
    case Cause::WrittenByShader:
        return "written by a shader";
    case Cause::Blit:
        return "blit";
    case Cause::BlitResolve:
        return "multisample resolve";
    case Cause::Upscaler:
        return "upscaler stand-in";
    case Cause::JoinedImage:
        return "joined image";
    case Cause::NewImage:
        return "new image";
    case Cause::AliasedImages:
        return "aliased images";
    case Cause::UpscaleSource:
        return "compute upscale source";
    }
    return "?";
}

/// What an image's own description says about scaling it.
struct Facts {
    bool rescaleable;      ///< ImageInfo::rescaleable
    bool downscaleable;    ///< ImageInfo::downscaleable
    bool downscaling;      ///< The resolution scale is below 1x
    bool keeps_mip_levels; ///< The scaled size still has room for all its mip levels
};

/// Why an image can't follow the resolution scale by its own description, or empty when nothing
/// in it stops it (an image sharing its memory still may).
[[nodiscard]] constexpr std::string_view Blocker(const Facts& facts) {
    if (!facts.rescaleable) {
        return "not rescaleable (linear, 3D, mipmapped, or a colour image 288 or fewer rows tall)";
    }
    if (facts.downscaling && !facts.downscaleable) {
        return "not downscaleable (512 or fewer rows tall)";
    }
    if (facts.downscaling && !facts.keeps_mip_levels) {
        return "its mip levels don't fit the scaled size";
    }
    return {};
}

/// Whether EDEN_NX2_RESCALE_TRACE is set; read once.
[[nodiscard]] inline bool Enabled() {
    static const bool enabled = std::getenv("EDEN_NX2_RESCALE_TRACE") != nullptr;
    return enabled;
}

} // namespace VideoCommon::RescaleTrace

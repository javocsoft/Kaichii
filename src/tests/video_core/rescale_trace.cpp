// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "video_core/texture_cache/rescale_trace.h"

using VideoCommon::RescaleTrace::Blocker;
using VideoCommon::RescaleTrace::Cause;
using VideoCommon::RescaleTrace::Facts;
using VideoCommon::RescaleTrace::Name;

TEST_CASE("Rescale trace names what keeps an image from the scale", "[video_core][rescale]") {
    const Facts scalable{.rescaleable = true,
                         .downscaleable = true,
                         .downscaling = true,
                         .keeps_mip_levels = true};
    CHECK(Blocker(scalable).empty());

    Facts linear = scalable;
    linear.rescaleable = false;
    CHECK(Blocker(linear).starts_with("not rescaleable"));

    // A short image follows an upscale but not a downscale.
    Facts short_image = scalable;
    short_image.downscaleable = false;
    CHECK(Blocker(short_image).starts_with("not downscaleable"));
    short_image.downscaling = false;
    CHECK(Blocker(short_image).empty());

    Facts mipmapped = scalable;
    mipmapped.keeps_mip_levels = false;
    CHECK(Blocker(mipmapped).starts_with("its mip levels"));
    mipmapped.downscaling = false;
    CHECK(Blocker(mipmapped).empty());

    // Not rescaleable comes first: it holds at any scale.
    Facts both = linear;
    both.downscaleable = false;
    CHECK(Blocker(both).starts_with("not rescaleable"));
}

TEST_CASE("Rescale trace names every cause", "[video_core][rescale]") {
    for (const Cause cause :
         {Cause::RenderTargets, Cause::SampledByPixel, Cause::Blit, Cause::BlitResolve,
          Cause::Upscaler, Cause::JoinedImage, Cause::NewImage, Cause::AliasedImages}) {
        CHECK(Name(cause) != "?");
        CHECK(!Name(cause).empty());
    }
}

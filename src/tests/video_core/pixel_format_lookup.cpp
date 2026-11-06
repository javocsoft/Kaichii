// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "video_core/surface.h"
#include "video_core/texture_cache/format_lookup_table.h"

namespace {

using Tegra::Texture::ComponentType;
using Tegra::Texture::TextureFormat;
using VideoCommon::PixelFormatFromTextureInfo;
using VideoCore::Surface::MaxPixelFormat;
using VideoCore::Surface::PixelFormat;
namespace Surface = VideoCore::Surface;

constexpr auto UNORM = ComponentType::UNORM;
constexpr auto SNORM = ComponentType::SNORM;
constexpr auto FLOAT = ComponentType::FLOAT;

PixelFormat Lookup(TextureFormat format, ComponentType component, bool srgb = false) {
    return PixelFormatFromTextureInfo(format, component, component, component, component, srgb);
}

} // namespace

TEST_CASE("Texture formats map to pixel formats by component and colour space",
          "[video_core][pixel_format]") {
    CHECK(Lookup(TextureFormat::A8B8G8R8, UNORM) == PixelFormat::A8B8G8R8_UNORM);
    CHECK(Lookup(TextureFormat::A8B8G8R8, UNORM, true) == PixelFormat::A8B8G8R8_SRGB);
    CHECK(Lookup(TextureFormat::A8B8G8R8, SNORM) == PixelFormat::A8B8G8R8_SNORM);
    CHECK(Lookup(TextureFormat::DXT1, UNORM) == PixelFormat::BC1_RGBA_UNORM);
    CHECK(Lookup(TextureFormat::DXT1, UNORM, true) == PixelFormat::BC1_RGBA_SRGB);
    CHECK(Lookup(TextureFormat::BC7U, UNORM, true) == PixelFormat::BC7_SRGB);
    CHECK(Lookup(TextureFormat::ASTC_2D_8X6, UNORM) == PixelFormat::ASTC_2D_8X6_UNORM);
    CHECK(Lookup(TextureFormat::ASTC_2D_8X6, UNORM, true) == PixelFormat::ASTC_2D_8X6_SRGB);
    CHECK(Lookup(TextureFormat::ASTC_2D_12X12, UNORM, true) == PixelFormat::ASTC_2D_12X12_SRGB);
    CHECK(Lookup(TextureFormat::B10G11R11, FLOAT) == PixelFormat::B10G11R11_FLOAT);
}

TEST_CASE("Mixed component types are distinct formats", "[video_core][pixel_format]") {
    // Depth formats mix component types, so all four fields have to take part in the lookup.
    CHECK(PixelFormatFromTextureInfo(TextureFormat::Z24S8, ComponentType::UINT, UNORM, UNORM, UNORM,
                                     false) == PixelFormat::S8_UINT_D24_UNORM);
    CHECK(PixelFormatFromTextureInfo(TextureFormat::S8Z24, UNORM, ComponentType::UINT,
                                     ComponentType::UINT, ComponentType::UINT, false) ==
          PixelFormat::D24_UNORM_S8_UINT);
}

TEST_CASE("Unknown texture formats fall back to RGBA8", "[video_core][pixel_format]") {
    CHECK(Lookup(TextureFormat::ASTC_2D_8X6, FLOAT) == PixelFormat::A8B8G8R8_UNORM);
}

TEST_CASE("Every pixel format has a usable block description", "[video_core][pixel_format]") {
    for (std::size_t i = 0; i < MaxPixelFormat; ++i) {
        const auto format = static_cast<PixelFormat>(i);
        INFO("pixel format " << i);
        CHECK(Surface::DefaultBlockWidth(format) >= 1);
        CHECK(Surface::DefaultBlockHeight(format) >= 1);
        CHECK(Surface::BitsPerBlock(format) >= 8);
        CHECK(Surface::BytesPerBlock(format) == Surface::BitsPerBlock(format) / 8);
    }
}

TEST_CASE("Compressed pixel format families are consistent", "[video_core][pixel_format]") {
    for (std::size_t i = 0; i < MaxPixelFormat; ++i) {
        const auto format = static_cast<PixelFormat>(i);
        INFO("pixel format " << i);
        if (Surface::IsPixelFormatASTC(format)) {
            CHECK_FALSE(Surface::IsPixelFormatBCn(format));
            CHECK(Surface::BitsPerBlock(format) == 128);
        }
        if (Surface::IsPixelFormatBCn(format)) {
            CHECK(Surface::DefaultBlockWidth(format) == 4);
            CHECK(Surface::DefaultBlockHeight(format) == 4);
        }
    }
}

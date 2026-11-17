// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <bit>
#include <random>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "video_core/host1x/codecs/vp9.h"

namespace {

// A separate writer following libvpx's vpx_write, used to check carry propagation.
struct ReferenceWriter {
    std::vector<u8> bytes;
    u32 low{};
    u32 range{255};
    s32 count{-24};
    size_t carries{};

    void Write(bool bit, s32 probability) {
        const u32 split = 1 + (((range - 1) * static_cast<u32>(probability)) >> 8);
        u32 next_range = split;
        if (bit) {
            low += split;
            next_range = range - split;
        }
        s32 shift = std::countl_zero(next_range) - 24;
        next_range <<= shift;
        count += shift;
        if (count >= 0) {
            const s32 offset = shift - count;
            if ((low << (offset - 1)) & 0x80000000) {
                ++carries;
                size_t index = bytes.size();
                while (index > 0 && bytes[index - 1] == 0xff) {
                    bytes[--index] = 0;
                }
                if (index > 0) {
                    ++bytes[index - 1];
                }
            }
            bytes.push_back(static_cast<u8>(low >> (24 - offset)));
            low = (low << offset) & 0xffffff;
            shift = count;
            count -= 8;
        }
        low <<= shift;
        range = next_range;
    }
};

} // namespace

TEST_CASE("VP9 range encoder carries into earlier bytes", "[video_core][vp9]") {
    std::mt19937 rng{757};
    size_t carries{};
    for (int run = 0; run < 2000; ++run) {
        Tegra::Decoders::VpxRangeEncoder encoder;
        ReferenceWriter reference;
        reference.Write(false, 128);
        for (int bit_index = 0; bit_index < 2000; ++bit_index) {
            const s32 probability = static_cast<s32>(1 + rng() % 255);
            const bool bit = rng() % 256 >= static_cast<u32>(probability);
            encoder.Write(bit, probability);
            reference.Write(bit, probability);
        }
        encoder.End();
        for (int bit_index = 0; bit_index < 32; ++bit_index) {
            reference.Write(false, 128);
        }
        REQUIRE(encoder.GetBuffer() == reference.bytes);
        carries += reference.carries;
    }
    REQUIRE(carries > 0);
}

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstring>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/loader/nro.h"
#include "core/loader/nso.h"

namespace {

using Loader::IsNroImageValid;
using Loader::NSOHeader;

constexpr u32 AllCompressed = 0b111;
constexpr std::size_t NroHeaderSize = 0x80;
constexpr std::size_t NroModOffsetField = 0x4;
constexpr std::size_t NroMagicField = 0x10;
constexpr std::size_t NroFileSizeField = 0x18;
constexpr u32 ModOffset = 0x80;

NSOHeader MakeNso() {
    NSOHeader header{};
    header.flags = AllCompressed;
    header.segments[0].location = 0;
    header.segments[0].size = 0x1000;
    header.segments[1].location = 0x1000;
    header.segments[1].size = 0x1000;
    header.segments[2].location = 0x2000;
    header.segments[2].size = 0x1000;
    header.segments_compressed_size = {0x800, 0x800, 0x800};
    return header;
}

template <typename T>
void Put(std::vector<u8>& bytes, std::size_t offset, T value) {
    std::memcpy(bytes.data() + offset, &value, sizeof(T));
}

std::vector<u8> MakeNro() {
    std::vector<u8> bytes(0x1000);
    Put<u32>(bytes, NroModOffsetField, ModOffset);
    std::memcpy(bytes.data() + NroMagicField, "NRO0", 4);
    Put<u32>(bytes, NroFileSizeField, 0x1000);
    std::memcpy(bytes.data() + ModOffset, "MOD0", 4);
    Put<u32>(bytes, ModOffset + 0x8, 0x100);
    Put<u32>(bytes, ModOffset + 0xC, 0x200);
    return bytes;
}

} // namespace

TEST_CASE("NSO accepts segments that fit", "[core][nso]") {
    CHECK(MakeNso().HasValidSegments());
    CHECK(MakeNso().SegmentsEnd() == 0x3000);
}

TEST_CASE("NSO image size covers the segment that ends last, whichever it is", "[core][nso]") {
    auto header = MakeNso();
    header.segments[0].size = 0x8000;
    CHECK(header.SegmentsEnd() == 0x8000);
    CHECK(header.HasValidSegments());
}

TEST_CASE("Stored NSO segment must be as large as it claims", "[core][nso]") {
    auto header = MakeNso();
    header.flags = 0;
    CHECK_FALSE(header.HasValidSegments());
    header.segments_compressed_size = {0x1000, 0x1000, 0x1000};
    CHECK(header.HasValidSegments());
}

TEST_CASE("NSO image must fit in 32 bits", "[core][nso]") {
    SECTION("segment") {
        auto header = MakeNso();
        header.segments[0].location = 0xFFFFFFFF;
        CHECK_FALSE(header.HasValidSegments());
    }
    SECTION(".bss") {
        auto header = MakeNso();
        header.segments[2].bss_size = 0xFFFFFFFF;
        CHECK_FALSE(header.HasValidSegments());
    }
}

TEST_CASE("NRO image accepts what its header describes", "[core][nro]") {
    CHECK(IsNroImageValid(MakeNro()));
}

TEST_CASE("NRO may leave its last page out of the file", "[core][nro]") {
    auto bytes = MakeNro();
    Put<u32>(bytes, NroFileSizeField, 0xFFF);
    bytes.resize(0xFFF);
    CHECK(IsNroImageValid(bytes));
}

TEST_CASE("NRO image is refused when its header lies", "[core][nro]") {
    SECTION("shorter than the header") {
        CHECK_FALSE(IsNroImageValid(std::vector<u8>(NroHeaderSize - 1)));
    }
    SECTION("no magic") {
        auto bytes = MakeNro();
        bytes[NroMagicField] = 'X';
        CHECK_FALSE(IsNroImageValid(bytes));
    }
    SECTION("larger than the file") {
        auto bytes = MakeNro();
        Put<u32>(bytes, NroFileSizeField, 0x2000);
        CHECK_FALSE(IsNroImageValid(bytes));
    }
    SECTION("size that wraps when page aligned") {
        auto bytes = MakeNro();
        Put<u32>(bytes, NroFileSizeField, 0xFFFFFFFF);
        CHECK_FALSE(IsNroImageValid(bytes));
    }
    SECTION("MOD0 past the image") {
        auto bytes = MakeNro();
        Put<u32>(bytes, NroModOffsetField, 0x1000);
        CHECK_FALSE(IsNroImageValid(bytes));
    }
    SECTION(".bss that ends before it starts") {
        auto bytes = MakeNro();
        Put<u32>(bytes, ModOffset + 0xC, 0x80);
        CHECK_FALSE(IsNroImageValid(bytes));
    }
}

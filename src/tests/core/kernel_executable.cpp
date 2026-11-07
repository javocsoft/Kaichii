// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <cstring>
#include <memory>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/file_sys/kernel_executable.h"
#include "core/file_sys/vfs/vfs_vector.h"
#include "core/loader/loader.h"

namespace {

using FileSys::INI;
using FileSys::KIP;
using Loader::ResultStatus;
using Bytes = std::vector<u8>;

FileSys::VirtualFile File(Bytes bytes) {
    return std::make_shared<FileSys::VectorVfsFile>(std::move(bytes), "file");
}

void Append(Bytes& bytes, const void* data, std::size_t size) {
    const auto* begin = static_cast<const u8*>(data);
    bytes.insert(bytes.end(), begin, begin + size);
}

void AppendU32(Bytes& bytes, u32 value) {
    Append(bytes, &value, sizeof(value));
}

/// A KIP whose text section has these bytes, kept as stored unless `decompressed_size` differs.
Bytes MakeKip(std::string_view name, const Bytes& text, u32 decompressed_size) {
    FileSys::KIPHeader header{};
    header.magic = Common::MakeMagic('K', 'I', 'P', '1');
    std::memcpy(header.name.data(), name.data(), name.size());
    header.sections[0].compressed_size = static_cast<u32>(text.size());
    header.sections[0].decompressed_size = decompressed_size;

    Bytes bytes;
    Append(bytes, &header, sizeof(header));
    bytes.insert(bytes.end(), text.begin(), text.end());
    return bytes;
}

Bytes MakeStoredKip(std::string_view name, const Bytes& text) {
    return MakeKip(name, text, static_cast<u32>(text.size()));
}

constexpr std::size_t BlzPatternSize = 18;
constexpr std::size_t BlzOutputSize = BlzPatternSize * 4;

/// BLZ decodes backwards: 18 literals, then three copies of them. It ends up as the pattern 4 times.
Bytes MakeBlz() {
    Bytes bytes{
        0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0, // Three 18-byte copies from 18 bytes above
        17,   16,                           // The last two literals
        0x38,                               // Literal, literal, copy, copy, copy
        15,   14,   13,   12,   11,   10,   9, 8,
        0x00,                               // Eight literals
        7,    6,    5,    4,    3,    2,    1, 0,
        0x00,                               // Eight literals
    };
    const auto stream_size = static_cast<u32>(bytes.size());
    const u32 compressed_size = stream_size + 12;
    AppendU32(bytes, compressed_size);
    AppendU32(bytes, 12);
    AppendU32(bytes, static_cast<u32>(BlzOutputSize) - compressed_size);
    return bytes;
}

ResultStatus LoadBlz(Bytes section) {
    return KIP{File(MakeKip("x", section, static_cast<u32>(BlzOutputSize)))}.GetStatus();
}

Bytes MakeIni(const std::vector<Bytes>& kips, u32 kip_count) {
    FileSys::INIHeader header{};
    header.magic = Common::MakeMagic('I', 'N', 'I', '1');
    header.kip_count = kip_count;

    Bytes bytes;
    Append(bytes, &header, sizeof(header));
    for (const auto& kip : kips) {
        bytes.insert(bytes.end(), kip.begin(), kip.end());
    }
    return bytes;
}

} // namespace

TEST_CASE("KIP keeps a stored section", "[core][kip]") {
    const KIP kip{File(MakeStoredKip("sm", {1, 2, 3}))};
    REQUIRE(kip.GetStatus() == ResultStatus::Success);
    CHECK(kip.GetName() == "sm");
    CHECK(kip.GetSectionDecompressed(0) == Bytes{1, 2, 3});
}

TEST_CASE("KIP decompresses a BLZ section", "[core][kip]") {
    const KIP kip{File(MakeKip("x", MakeBlz(), static_cast<u32>(BlzOutputSize)))};
    REQUIRE(kip.GetStatus() == ResultStatus::Success);

    const auto text = kip.GetSectionDecompressed(0);
    REQUIRE(text.size() == BlzOutputSize);
    for (std::size_t i = 0; i < BlzOutputSize; ++i) {
        CHECK(text[i] == BlzPatternSize - 1 - i % BlzPatternSize);
    }
}

TEST_CASE("Damaged BLZ sections are refused", "[core][kip]") {
    const auto footer_field = [](std::size_t index, u32 value) {
        auto bytes = MakeBlz();
        std::memcpy(bytes.data() + bytes.size() - 12 + index * sizeof(u32), &value, sizeof(value));
        return bytes;
    };

    SECTION("compressed size past the start of the section") {
        CHECK(LoadBlz(footer_field(0, 0x7FFFFFFF)) == ResultStatus::ErrorBLZDecompressionFailed);
    }
    SECTION("start past the compressed size") {
        CHECK(LoadBlz(footer_field(1, 0x1000)) == ResultStatus::ErrorBLZDecompressionFailed);
    }
    SECTION("stream shorter than its output") {
        CHECK(LoadBlz(footer_field(2, 0x1000)) == ResultStatus::ErrorBLZDecompressionFailed);
    }
    SECTION("literal past the start of the stream") {
        // All literals, so the stream runs out at the first literal of a control byte...
        auto bytes = MakeBlz();
        std::memset(bytes.data(), 0, bytes.size() - 12);
        CHECK(LoadBlz(std::move(bytes)) == ResultStatus::ErrorBLZDecompressionFailed);
    }
    SECTION("literal past the start of the stream, in the middle of a control byte") {
        // ... and here, one byte shorter, in the middle of one.
        auto bytes = footer_field(1, 13);
        std::memset(bytes.data(), 0, bytes.size() - 12);
        CHECK(LoadBlz(std::move(bytes)) == ResultStatus::ErrorBLZDecompressionFailed);
    }
    SECTION("shorter than the footer") {
        CHECK(LoadBlz(Bytes(11)) == ResultStatus::ErrorBLZDecompressionFailed);
    }
}

TEST_CASE("INI reads each of its KIPs", "[core][kip]") {
    const auto first = MakeStoredKip("first", {1, 2, 3});
    const auto second = MakeStoredKip("second", {4, 5});
    const INI ini{File(MakeIni({first, second}, 2))};

    REQUIRE(ini.GetStatus() == ResultStatus::Success);
    REQUIRE(ini.GetKIPs().size() == 2);
    CHECK(ini.GetKIPs()[0].GetName() == "first");
    CHECK(ini.GetKIPs()[1].GetName() == "second");
}

TEST_CASE("INI stops at the end of its data", "[core][kip]") {
    SECTION("fewer KIPs than it claims") {
        const INI ini{File(MakeIni({MakeStoredKip("only", {1})}, 3))};
        CHECK(ini.GetStatus() == ResultStatus::Success);
        CHECK(ini.GetKIPs().size() == 1);
    }
    SECTION("a KIP that can't be read") {
        const INI ini{File(MakeIni({MakeStoredKip("first", {1}), Bytes(0x100)}, 2))};
        CHECK(ini.GetKIPs().size() == 1);
    }
}

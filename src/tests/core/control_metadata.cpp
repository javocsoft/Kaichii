// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstring>
#include <memory>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/file_sys/control_metadata.h"
#include "core/file_sys/vfs/vfs_vector.h"

namespace {

using FileSys::LanguageEntry;
using FileSys::NACP;
using FileSys::RawNACP;

/// A raw deflate stream of one stored block, which takes no compression to build.
std::vector<u8> Stored(const std::vector<u8>& payload) {
    const auto size = static_cast<u16>(payload.size());
    const auto inverted = static_cast<u16>(~size);
    std::vector<u8> stream{0x01};
    stream.resize(5);
    std::memcpy(stream.data() + 1, &size, sizeof(size));
    std::memcpy(stream.data() + 3, &inverted, sizeof(inverted));
    stream.insert(stream.end(), payload.begin(), payload.end());
    return stream;
}

NACP Load(const std::vector<u8>& stream, u16 claimed_size) {
    auto raw = std::make_unique<RawNACP>();
    raw->titles_data_format = FileSys::TitleDataFormat::Compressed;
    raw->language_entries.compressed_data.buffer_size = claimed_size;
    std::memcpy(raw->language_entries.compressed_data.buffer, stream.data(), stream.size());

    std::vector<u8> bytes(sizeof(RawNACP));
    std::memcpy(bytes.data(), raw.get(), sizeof(RawNACP));
    return NACP{std::make_shared<FileSys::VectorVfsFile>(std::move(bytes), "control.nacp")};
}

std::vector<u8> Titles(std::size_t size) {
    std::vector<u8> payload(size);
    const std::string_view name{"Title"};
    std::memcpy(payload.data(), name.data(), name.size());
    return payload;
}

} // namespace

TEST_CASE("Compressed NACP reads its titles", "[core][nacp]") {
    const auto stream = Stored(Titles(sizeof(LanguageEntry)));
    CHECK(Load(stream, static_cast<u16>(stream.size())).GetApplicationName() == "Title");
}

TEST_CASE("Compressed NACP ignores a partial entry", "[core][nacp]") {
    const auto stream = Stored(Titles(sizeof(LanguageEntry) + 1));
    CHECK(Load(stream, static_cast<u16>(stream.size())).GetApplicationName() == "Title");
}

TEST_CASE("Compressed NACP size stops at its buffer", "[core][nacp]") {
    const auto stream = Stored(Titles(sizeof(LanguageEntry)));
    CHECK(Load(stream, 0xFFFF).GetApplicationName() == "Title");
}

TEST_CASE("Damaged compressed NACP has no titles", "[core][nacp]") {
    CHECK(Load({0xFF, 0xFF, 0xFF}, 3).GetApplicationName().empty());
}

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <cstddef>
#include <limits>
#include <memory>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/file_sys/vfs/vfs_offset.h"
#include "core/file_sys/vfs/vfs_static.h"
#include "core/file_sys/vfs/vfs_vector.h"

namespace {

using Bytes = std::vector<u8>;

constexpr std::size_t Huge = std::numeric_limits<std::size_t>::max();

void CheckReadsStopAtTheEnd(const FileSys::VfsFile& file, std::size_t size) {
    std::array<u8, 8> out{};
    CHECK(file.Read(out.data(), out.size(), size - 1) == 1);
    CHECK(file.Read(out.data(), out.size(), size) == 0);
    CHECK(file.Read(out.data(), out.size(), size + 1) == 0);
    CHECK(file.Read(out.data(), out.size(), Huge) == 0);
    CHECK_FALSE(file.ReadByte(size).has_value());
    CHECK(file.ReadBytes(1, size).empty());
}

} // namespace

TEST_CASE("VFS files read nothing at or past their end", "[core][vfs]") {
    SECTION("vector") {
        CheckReadsStopAtTheEnd(FileSys::VectorVfsFile{Bytes{1, 2, 3, 4}}, 4);
    }
    SECTION("array") {
        CheckReadsStopAtTheEnd(FileSys::ArrayVfsFile<4>{std::array<u8, 4>{1, 2, 3, 4}}, 4);
    }
    SECTION("static") {
        CheckReadsStopAtTheEnd(FileSys::StaticVfsFile{0xAB, 4}, 4);
    }
    SECTION("offset") {
        const auto parent = std::make_shared<FileSys::VectorVfsFile>(Bytes(16, 0xCD));
        CheckReadsStopAtTheEnd(FileSys::OffsetVfsFile{parent, 4, 2}, 4);
    }
}

TEST_CASE("An empty VFS file reads nothing", "[core][vfs]") {
    std::array<u8, 8> out{};
    CHECK(FileSys::VectorVfsFile{}.Read(out.data(), out.size(), 0) == 0);
    CHECK(FileSys::StaticVfsFile{0xAB, 0}.Read(out.data(), out.size(), 0) == 0);
    CHECK(FileSys::VectorVfsFile{}.ReadAllBytes().empty());
}

TEST_CASE("Offset file never reaches past its slice", "[core][vfs]") {
    const auto parent = std::make_shared<FileSys::VectorVfsFile>(Bytes{0, 1, 2, 3, 4, 5, 6, 7});
    FileSys::OffsetVfsFile slice{parent, 2, 2};

    std::array<u8, 8> out{};
    REQUIRE(slice.Read(out.data(), out.size(), 1) == 1);
    CHECK(out[0] == 3);
    CHECK(slice.ReadBytes(8, 1) == Bytes{3});

    const std::array<u8, 2> data{0xAA, 0xBB};
    CHECK(slice.Write(data.data(), data.size(), 2) == 0);
    CHECK(slice.Write(data.data(), data.size(), 1) == 1);
    CHECK(slice.WriteBytes(Bytes{0xCC}, 100) == 0);
    CHECK(parent->ReadAllBytes() == Bytes{0, 1, 2, 0xAA, 4, 5, 6, 7});
}

TEST_CASE("ReadBytes allocates what the file holds, not what was asked for", "[core][vfs]") {
    const FileSys::VectorVfsFile file{Bytes{1, 2, 3, 4}};
    CHECK(file.ReadBytes(Huge, 0) == Bytes{1, 2, 3, 4});
    CHECK(file.ReadBytes(Huge, 3) == Bytes{4});
    CHECK(file.ReadBytes(Huge, 4).empty());
    CHECK(file.ReadBytes(2, 1) == Bytes{2, 3});
}

TEST_CASE("An empty write does not grow a VectorVfsFile", "[core][vfs]") {
    FileSys::VectorVfsFile file{Bytes{1, 2}};
    const u8 data = 0xAA;
    CHECK(file.Write(&data, 0, 100) == 0);
    CHECK(file.GetSize() == 2);
}

TEST_CASE("VectorVfsFile refuses a write past the largest possible size", "[core][vfs]") {
    FileSys::VectorVfsFile file{Bytes{1, 2}};
    const std::array<u8, 2> data{0xAA, 0xBB};
    CHECK(file.Write(data.data(), data.size(), Huge) == 0);
    CHECK(file.Write(data.data(), data.size(), Huge - 1) == 0);
    CHECK(file.ReadAllBytes() == Bytes{1, 2});
    CHECK(file.Write(data.data(), data.size(), 1) == 2);
    CHECK(file.ReadAllBytes() == Bytes{1, 0xAA, 0xBB});
}

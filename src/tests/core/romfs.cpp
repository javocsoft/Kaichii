// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/file_sys/romfs.h"
#include "core/file_sys/vfs/vfs_vector.h"

namespace {

using FileSys::VectorVfsDirectory;
using FileSys::VectorVfsFile;
using FileSys::VirtualDir;
using FileSys::VirtualFile;

constexpr std::size_t RomFSHeaderSize = 0x50;
constexpr std::size_t DirectoryEntrySize = 0x18;
constexpr std::size_t FileEntrySize = 0x20;
constexpr u32 NoEntry = 0xFFFFFFFF;

struct DirectoryLinks {
    u32 sibling = NoEntry;
    u32 child_dir = NoEntry;
    u32 child_file = NoEntry;
};

template <typename T>
void Put(std::vector<u8>& bytes, std::size_t offset, T value) {
    std::memcpy(bytes.data() + offset, &value, sizeof(T));
}

/// Hand-built tables, for shapes the builder cannot produce. Entries have empty names, so each one
/// is its index times the entry size into its table. `siblings` links the files.
std::vector<u8> MakeTables(const std::vector<DirectoryLinks>& directories,
                           const std::vector<u32>& siblings) {
    const std::size_t directory_size = directories.size() * DirectoryEntrySize;
    const std::size_t file_size = siblings.size() * FileEntrySize;
    std::vector<u8> bytes(RomFSHeaderSize + directory_size + file_size);

    Put<u64>(bytes, 0x00, RomFSHeaderSize);
    Put<u64>(bytes, 0x18, RomFSHeaderSize);
    Put<u64>(bytes, 0x20, directory_size);
    Put<u64>(bytes, 0x38, RomFSHeaderSize + directory_size);
    Put<u64>(bytes, 0x40, file_size);
    Put<u64>(bytes, 0x48, bytes.size());

    for (std::size_t i = 0; i < directories.size(); ++i) {
        const std::size_t entry = RomFSHeaderSize + i * DirectoryEntrySize;
        Put<u32>(bytes, entry + 0x4, directories[i].sibling);
        Put<u32>(bytes, entry + 0x8, directories[i].child_dir);
        Put<u32>(bytes, entry + 0xC, directories[i].child_file);
    }
    for (std::size_t i = 0; i < siblings.size(); ++i) {
        Put<u32>(bytes, RomFSHeaderSize + directory_size + i * FileEntrySize + 0x4, siblings[i]);
    }
    return bytes;
}

/// A line of directories, each the only child of the one before it.
std::vector<DirectoryLinks> Chain(std::size_t length) {
    std::vector<DirectoryLinks> directories(length);
    for (std::size_t i = 0; i + 1 < length; ++i) {
        directories[i].child_dir = static_cast<u32>((i + 1) * DirectoryEntrySize);
    }
    return directories;
}

VirtualFile File(std::string name, std::vector<u8> data) {
    return std::make_shared<VectorVfsFile>(std::move(data), std::move(name));
}

VirtualDir Dir(std::string name, std::vector<VirtualFile> files, std::vector<VirtualDir> dirs = {}) {
    return std::make_shared<VectorVfsDirectory>(std::move(files), std::move(dirs), std::move(name));
}

std::vector<u8> Pattern(std::size_t size) {
    std::vector<u8> data(size);
    for (std::size_t i = 0; i < size; ++i) {
        data[i] = static_cast<u8>(i * 31);
    }
    return data;
}

} // namespace

TEST_CASE("RomFS keeps a tree through building and extraction", "[core][romfs]") {
    // Larger than the builder's 16-byte alignment and block padding, on purpose.
    const auto big = Pattern(0x12345);
    const auto base = Dir("", {File("root.bin", {1, 2, 3}), File("big.bin", big)},
                          {Dir("sub", {File("a.txt", {4})}, {Dir("deep", {File("b.txt", {5, 6})})}),
                           Dir("other", {File("c.txt", {7})})});

    const auto romfs = FileSys::CreateRomFS(base, nullptr);
    REQUIRE(romfs != nullptr);
    const auto root = FileSys::ExtractRomFS(romfs);
    REQUIRE(root != nullptr);

    CHECK(root->GetFiles().size() == 2);
    CHECK(root->GetSubdirectories().size() == 2);
    CHECK(root->GetFile("root.bin")->ReadAllBytes() == std::vector<u8>{1, 2, 3});
    CHECK(root->GetFile("big.bin")->ReadAllBytes() == big);

    const auto sub = root->GetSubdirectory("sub");
    REQUIRE(sub != nullptr);
    CHECK(sub->GetFile("a.txt")->ReadAllBytes() == std::vector<u8>{4});
    const auto deep = sub->GetSubdirectory("deep");
    REQUIRE(deep != nullptr);
    CHECK(deep->GetFile("b.txt")->ReadAllBytes() == std::vector<u8>{5, 6});
    CHECK(root->GetSubdirectory("other")->GetFile("c.txt")->ReadAllBytes() == std::vector<u8>{7});
}

TEST_CASE("RomFS keeps an empty file", "[core][romfs]") {
    const auto root = FileSys::ExtractRomFS(
        FileSys::CreateRomFS(Dir("", {File("empty", {}), File("after", {9})}), nullptr));
    REQUIRE(root != nullptr);
    REQUIRE(root->GetFile("empty") != nullptr);
    CHECK(root->GetFile("empty")->GetSize() == 0);
    CHECK(root->GetFile("after")->ReadAllBytes() == std::vector<u8>{9});
}

TEST_CASE("RomFS mod layer removes and patches files", "[core][romfs]") {
    const auto base =
        Dir("", {File("kept", {1}), File("removed", {2}), File("patched", {0, 0, 0})},
            {Dir("gone", {File("x", {3})}), Dir("sub", {File("removed", {4}), File("kept", {5})})});

    const std::vector<u8> ips{'P', 'A', 'T', 'C', 'H', 0, 0, 1, 0, 1, 0xAA, 'E', 'O', 'F'};
    const auto ext =
        Dir("", {File("removed.stub", {}), File("patched.ips", ips), File("gone.stub", {})},
            {Dir("sub", {File("removed.stub", {})})});

    const auto root = FileSys::ExtractRomFS(FileSys::CreateRomFS(base, ext));
    REQUIRE(root != nullptr);
    CHECK(root->GetFile("kept") != nullptr);
    CHECK(root->GetFile("removed") == nullptr);
    CHECK(root->GetSubdirectory("gone") == nullptr);
    CHECK(root->GetFile("patched")->ReadAllBytes() == std::vector<u8>{0, 0xAA, 0});

    const auto sub = root->GetSubdirectory("sub");
    REQUIRE(sub != nullptr);
    CHECK(sub->GetFile("removed") == nullptr);
    CHECK(sub->GetFile("kept") != nullptr);
}

TEST_CASE("RomFS tables made by hand extract", "[core][romfs]") {
    const auto root = FileSys::ExtractRomFS(File(
        "x", MakeTables({{.child_dir = DirectoryEntrySize, .child_file = 0}, {}}, {FileEntrySize, NoEntry})));
    REQUIRE(root != nullptr);
    CHECK(root->GetFiles().size() == 2);
    CHECK(root->GetSubdirectories().size() == 1);
}

TEST_CASE("RomFS stops at the deepest path the format allows", "[core][romfs]") {
    CHECK(FileSys::ExtractRomFS(File("x", MakeTables(Chain(300), {}))) != nullptr);
    CHECK(FileSys::ExtractRomFS(File("x", MakeTables(Chain(5000), {}))) == nullptr);
}

TEST_CASE("RomFS with broken links is rejected", "[core][romfs]") {
    SECTION("no root directory") {
        CHECK(FileSys::ExtractRomFS(File("x", MakeTables({}, {}))) == nullptr);
    }
    SECTION("directory is its own sibling") {
        CHECK(FileSys::ExtractRomFS(File("x", MakeTables({{.sibling = 0}}, {}))) == nullptr);
    }
    SECTION("directory is its own child") {
        CHECK(FileSys::ExtractRomFS(File("x", MakeTables({{.child_dir = 0}}, {}))) == nullptr);
    }
    SECTION("file is its own sibling") {
        CHECK(FileSys::ExtractRomFS(File("x", MakeTables({{.child_file = 0}}, {0}))) == nullptr);
    }
    SECTION("sibling past the directory table") {
        CHECK(FileSys::ExtractRomFS(File("x", MakeTables({{.sibling = 0x1000}}, {}))) == nullptr);
    }
    SECTION("child past the directory table") {
        CHECK(FileSys::ExtractRomFS(File("x", MakeTables({{.child_dir = 0x1000}}, {}))) == nullptr);
    }
    SECTION("file past the file table") {
        CHECK(FileSys::ExtractRomFS(File("x", MakeTables({{.child_file = 0x1000}}, {}))) == nullptr);
    }
}

TEST_CASE("RomFS table sizes are limited by the file", "[core][romfs]") {
    auto bytes = MakeTables({{}}, {});
    Put<u64>(bytes, 0x20, std::numeric_limits<u64>::max());
    CHECK(FileSys::ExtractRomFS(File("x", std::move(bytes))) != nullptr);
}

TEST_CASE("RomFS rejects files that are not one", "[core][romfs]") {
    SECTION("no file gives an empty directory") {
        const auto root = FileSys::ExtractRomFS(nullptr);
        REQUIRE(root != nullptr);
        CHECK(root->GetFiles().empty());
    }
    SECTION("shorter than the header") {
        CHECK(FileSys::ExtractRomFS(File("x", std::vector<u8>(RomFSHeaderSize - 1))) == nullptr);
    }
    SECTION("wrong header size") {
        std::vector<u8> bytes(RomFSHeaderSize);
        const u64 header_size = RomFSHeaderSize + 8;
        std::memcpy(bytes.data(), &header_size, sizeof(header_size));
        CHECK(FileSys::ExtractRomFS(File("x", std::move(bytes))) == nullptr);
    }
    SECTION("no directory gives no RomFS") {
        CHECK(FileSys::CreateRomFS(nullptr, nullptr) == nullptr);
    }
}

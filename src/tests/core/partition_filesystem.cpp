// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/file_sys/partition_filesystem.h"
#include "core/file_sys/vfs/vfs_vector.h"
#include "core/loader/loader.h"

namespace {

using FileSys::PartitionFilesystem;
using Loader::ResultStatus;
using Entries = std::vector<std::pair<std::string, std::vector<u8>>>;

constexpr std::size_t HeaderSize = 0x10;
constexpr std::size_t PfsEntrySize = 0x18;
constexpr std::size_t HfsEntrySize = 0x40;

template <typename T>
void Put(std::vector<u8>& bytes, std::size_t offset, T value) {
    std::memcpy(bytes.data() + offset, &value, sizeof(T));
}

std::vector<u8> MakePartition(std::string_view magic, std::size_t entry_size,
                              const Entries& entries) {
    std::string strtab;
    for (const auto& [name, data] : entries) {
        strtab += name;
        strtab.push_back('\0');
    }
    const std::size_t strtab_offset = HeaderSize + entries.size() * entry_size;
    const std::size_t content_offset = strtab_offset + strtab.size();

    std::vector<u8> bytes(content_offset);
    std::memcpy(bytes.data(), magic.data(), 4);
    Put<u32>(bytes, 4, static_cast<u32>(entries.size()));
    Put<u32>(bytes, 8, static_cast<u32>(strtab.size()));
    std::memcpy(bytes.data() + strtab_offset, strtab.data(), strtab.size());

    u64 data_offset = 0;
    u32 name_offset = 0;
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& [name, data] = entries[i];
        const std::size_t entry = HeaderSize + i * entry_size;
        Put<u64>(bytes, entry, data_offset);
        Put<u64>(bytes, entry + 8, data.size());
        Put<u32>(bytes, entry + 0x10, name_offset);
        bytes.insert(bytes.end(), data.begin(), data.end());
        data_offset += data.size();
        name_offset += static_cast<u32>(name.size() + 1);
    }
    return bytes;
}

FileSys::VirtualFile File(std::vector<u8> bytes) {
    return std::make_shared<FileSys::VectorVfsFile>(std::move(bytes), "container");
}

const Entries Files{{"main", {1, 2, 3}}, {"main.npdm", {4, 5}}, {"rtld", {6}}};

void CheckContents(std::string_view magic, std::size_t entry_size) {
    const PartitionFilesystem partition{File(MakePartition(magic, entry_size, Files))};
    REQUIRE(partition.GetStatus() == ResultStatus::Success);
    CHECK(partition.GetName() == magic);
    CHECK(partition.GetFiles().size() == Files.size());
    CHECK(partition.GetSubdirectories().empty());

    for (const auto& [name, data] : Files) {
        const auto file = partition.GetFile(name);
        REQUIRE(file != nullptr);
        CHECK(file->ReadAllBytes() == data);
    }

    // Offsets are from the start of the container, past the metadata.
    const std::size_t content_offset =
        HeaderSize + Files.size() * entry_size + std::string_view{"main\0main.npdm\0rtld\0", 20}.size();
    const auto offsets = partition.GetFileOffsets();
    CHECK(offsets.at("main") == content_offset);
    CHECK(offsets.at("main.npdm") == content_offset + 3);
    CHECK(offsets.at("rtld") == content_offset + 5);
    CHECK(partition.GetFileSizes().at("main.npdm") == 2);
}

} // namespace

TEST_CASE("PFS0 lists its files and reads their contents", "[core][partition_filesystem]") {
    CheckContents("PFS0", PfsEntrySize);
}

TEST_CASE("HFS0 uses its wider entries", "[core][partition_filesystem]") {
    CheckContents("HFS0", HfsEntrySize);
}

TEST_CASE("Empty partition is valid", "[core][partition_filesystem]") {
    const PartitionFilesystem partition{File(MakePartition("PFS0", PfsEntrySize, {}))};
    CHECK(partition.GetStatus() == ResultStatus::Success);
    CHECK(partition.GetFiles().empty());
}

TEST_CASE("Partition string table needs no final terminator", "[core][partition_filesystem]") {
    auto bytes = MakePartition("PFS0", PfsEntrySize, {{"name", {7}}});
    const std::size_t strtab_size = 5;
    bytes.erase(bytes.begin() + HeaderSize + PfsEntrySize + strtab_size - 1);
    Put<u32>(bytes, 8, static_cast<u32>(strtab_size - 1));

    const PartitionFilesystem partition{File(std::move(bytes))};
    REQUIRE(partition.GetStatus() == ResultStatus::Success);
    CHECK(partition.GetFile("name") != nullptr);
}

TEST_CASE("Partition lists more entries than a 16-bit counter holds",
          "[core][partition_filesystem]") {
    Entries entries;
    for (std::size_t i = 0; i < 0x10001; ++i) {
        entries.emplace_back(std::to_string(i), std::vector<u8>{});
    }
    const PartitionFilesystem partition{File(MakePartition("PFS0", PfsEntrySize, entries))};
    REQUIRE(partition.GetStatus() == ResultStatus::Success);
    CHECK(partition.GetFiles().size() == entries.size());
}

TEST_CASE("Damaged partition headers are rejected", "[core][partition_filesystem]") {
    SECTION("shorter than the header") {
        CHECK(PartitionFilesystem{File(std::vector<u8>(HeaderSize - 1))}.GetStatus() ==
              ResultStatus::ErrorBadPFSHeader);
    }
    SECTION("unknown magic") {
        auto bytes = MakePartition("PFS1", PfsEntrySize, Files);
        CHECK(PartitionFilesystem{File(std::move(bytes))}.GetStatus() ==
              ResultStatus::ErrorBadPFSHeader);
    }
    SECTION("more entries than the file holds") {
        auto bytes = MakePartition("PFS0", PfsEntrySize, Files);
        Put<u32>(bytes, 4, 1000);
        CHECK(PartitionFilesystem{File(std::move(bytes))}.GetStatus() ==
              ResultStatus::ErrorIncorrectPFSFileSize);
    }
    SECTION("every bit set in the entry count") {
        auto bytes = MakePartition("PFS0", HfsEntrySize, Files);
        Put<u32>(bytes, 4, 0xFFFFFFFF);
        CHECK(PartitionFilesystem{File(std::move(bytes))}.GetStatus() ==
              ResultStatus::ErrorIncorrectPFSFileSize);
    }
    SECTION("name outside the string table") {
        auto bytes = MakePartition("PFS0", PfsEntrySize, Files);
        const auto strtab_size = static_cast<u32>(std::string_view{"main\0main.npdm\0rtld\0", 20}.size());
        Put<u32>(bytes, HeaderSize + 0x10, strtab_size);
        CHECK(PartitionFilesystem{File(std::move(bytes))}.GetStatus() ==
              ResultStatus::ErrorBadPFSHeader);
    }
    SECTION("string table past the end of the file") {
        auto bytes = MakePartition("PFS0", PfsEntrySize, Files);
        Put<u32>(bytes, 8, 0x100000);
        CHECK(PartitionFilesystem{File(std::move(bytes))}.GetStatus() ==
              ResultStatus::ErrorIncorrectPFSFileSize);
    }
}

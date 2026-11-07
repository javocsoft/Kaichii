// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstring>
#include <memory>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/file_sys/program_metadata.h"
#include "core/file_sys/vfs/vfs_vector.h"
#include "core/loader/loader.h"

namespace {

using FileSys::PoolPartition;
using FileSys::ProgramAddressSpaceType;
using FileSys::ProgramMetadata;
using Loader::ResultStatus;

constexpr std::size_t AciOffset = 0x80;
constexpr std::size_t AcidOffset = 0x200;
// FAH and KAC offsets count from the ACI0 block, the FAC offset from the ACID block.
constexpr std::size_t FahOffset = 0x40;
constexpr std::size_t KacOffset = 0x60;
constexpr std::size_t FacOffset = 0x400;
constexpr u32 Switch1SignatureSize = 0x200;
constexpr u32 Switch2SignatureSize = 0x300;

constexpr u64 TitleId = 0x0400C3F00006E000;
constexpr u64 Permissions = 0x8000'0000'0000'0021;
constexpr u32 KernelCapability = 0x11223344;

template <typename T>
void Put(std::vector<u8>& bytes, std::size_t offset, T value) {
    std::memcpy(bytes.data() + offset, &value, sizeof(T));
}

std::vector<u8> MakeNpdm(u32 signature_size) {
    std::vector<u8> bytes(0x800);
    std::memcpy(bytes.data(), "META", 4);
    bytes[0xC] = 1 | (static_cast<u8>(ProgramAddressSpaceType::Is39Bit) << 1);
    bytes[0xE] = 44;
    bytes[0xF] = 1;
    Put<u32>(bytes, 0x14, 0x1000);
    Put<u32>(bytes, 0x1C, 0x100000);
    Put<u32>(bytes, 0x70, AciOffset);
    Put<u32>(bytes, 0x78, AcidOffset);

    std::memcpy(bytes.data() + AciOffset, "ACI0", 4);
    Put<u64>(bytes, AciOffset + 0x10, TitleId);
    Put<u32>(bytes, AciOffset + 0x20, FahOffset);
    Put<u32>(bytes, AciOffset + 0x30, KacOffset);
    Put<u32>(bytes, AciOffset + 0x34, sizeof(u32));
    Put<u64>(bytes, AciOffset + FahOffset + 4, Permissions);
    Put<u32>(bytes, AciOffset + KacOffset, KernelCapability);

    const std::size_t acid_magic = AcidOffset + signature_size;
    std::memcpy(bytes.data() + acid_magic, "ACID", 4);
    Put<u32>(bytes, acid_magic + 0xC, static_cast<u32>(PoolPartition::SystemNonSecure) << 2);
    Put<u32>(bytes, acid_magic + 0x20, FacOffset);
    return bytes;
}

FileSys::VirtualFile File(std::vector<u8> bytes) {
    return std::make_shared<FileSys::VectorVfsFile>(std::move(bytes), "main.npdm");
}

ResultStatus Load(std::vector<u8> bytes) {
    ProgramMetadata metadata;
    return metadata.Load(File(std::move(bytes)));
}

void CheckFields(const ProgramMetadata& metadata) {
    CHECK(metadata.Is64BitProgram());
    CHECK(metadata.GetAddressSpaceType() == ProgramAddressSpaceType::Is39Bit);
    CHECK(metadata.GetMainThreadPriority() == 44);
    CHECK(metadata.GetMainThreadCore() == 1);
    CHECK(metadata.GetMainThreadStackSize() == 0x100000);
    CHECK(metadata.GetSystemResourceSize() == 0x1000);
    CHECK(metadata.GetTitleID() == TitleId);
    CHECK(metadata.GetFilesystemPermissions() == Permissions);
    CHECK(metadata.GetPoolPartition() == PoolPartition::SystemNonSecure);
    REQUIRE(metadata.GetKernelCapabilities().size() == 1);
    CHECK(metadata.GetKernelCapabilities()[0] == KernelCapability);
}

} // namespace

TEST_CASE("NPDM with the Switch 1 ACID layout is not a Switch 2 program",
          "[core][program_metadata]") {
    ProgramMetadata metadata;
    REQUIRE(metadata.Load(File(MakeNpdm(Switch1SignatureSize))) == ResultStatus::Success);
    CHECK_FALSE(metadata.IsSwitch2());
    CheckFields(metadata);
}

TEST_CASE("NPDM with the longer ACID signature area is a Switch 2 program",
          "[core][program_metadata]") {
    // Everything after the ACID magic moves 0x100 bytes: the pool partition and FAC offset only
    // read back correctly if the body was read from its new place.
    ProgramMetadata metadata;
    REQUIRE(metadata.Load(File(MakeNpdm(Switch2SignatureSize))) == ResultStatus::Success);
    CHECK(metadata.IsSwitch2());
    CheckFields(metadata);
}

TEST_CASE("NPDM without an ACID magic in either layout is rejected", "[core][program_metadata]") {
    auto bytes = MakeNpdm(Switch1SignatureSize);
    Put<u32>(bytes, AcidOffset + Switch1SignatureSize, 0);
    CHECK(Load(std::move(bytes)) == ResultStatus::ErrorBadACIDHeader);
}

TEST_CASE("Truncated NPDM blocks report which block is bad", "[core][program_metadata]") {
    const auto valid = MakeNpdm(Switch2SignatureSize);
    // Exactly the end of the file: VectorVfsFile does not bounds-check an offset beyond it
    // (docs/ai/known-issues.md).
    const auto end = static_cast<u32>(valid.size());

    SECTION("header") {
        auto bytes = valid;
        bytes.resize(0x7F);
        CHECK(Load(std::move(bytes)) == ResultStatus::ErrorBadNPDMHeader);
    }
    SECTION("ACID block past the end") {
        auto bytes = valid;
        Put<u32>(bytes, 0x78, end);
        CHECK(Load(std::move(bytes)) == ResultStatus::ErrorBadACIDHeader);
    }
    SECTION("Switch 2 ACID body past the end") {
        auto bytes = valid;
        bytes.resize(AcidOffset + Switch2SignatureSize + 0x20);
        CHECK(Load(std::move(bytes)) == ResultStatus::ErrorBadACIDHeader);
    }
    SECTION("ACI0 block") {
        auto bytes = valid;
        Put<u32>(bytes, 0x70, end);
        CHECK(Load(std::move(bytes)) == ResultStatus::ErrorBadACIHeader);
    }
    SECTION("file access control") {
        auto bytes = valid;
        Put<u32>(bytes, AcidOffset + Switch2SignatureSize + 0x20, end - AcidOffset);
        CHECK(Load(std::move(bytes)) == ResultStatus::ErrorBadFileAccessControl);
    }
    SECTION("file access header") {
        auto bytes = valid;
        Put<u32>(bytes, AciOffset + 0x20, end - AciOffset);
        CHECK(Load(std::move(bytes)) == ResultStatus::ErrorBadFileAccessHeader);
    }
    SECTION("kernel capabilities") {
        auto bytes = valid;
        Put<u32>(bytes, AciOffset + 0x34, end);
        CHECK(Load(std::move(bytes)) == ResultStatus::ErrorBadKernelCapabilityDescriptors);
    }
}

TEST_CASE("NPDM reload takes the update's metadata but keeps the base title id",
          "[core][program_metadata]") {
    ProgramMetadata metadata;
    REQUIRE(metadata.Load(File(MakeNpdm(Switch2SignatureSize))) == ResultStatus::Success);

    auto update = MakeNpdm(Switch2SignatureSize);
    Put<u64>(update, AciOffset + 0x10, TitleId + 0x800);
    Put<u32>(update, 0x1C, 0x200000);
    REQUIRE(metadata.Reload(File(std::move(update))) == ResultStatus::Success);

    CHECK(metadata.GetTitleID() == TitleId);
    CHECK(metadata.GetMainThreadStackSize() == 0x200000);
}

TEST_CASE("Default NPDM is a Switch 1 program", "[core][program_metadata]") {
    CHECK_FALSE(ProgramMetadata::GetDefault().IsSwitch2());
}

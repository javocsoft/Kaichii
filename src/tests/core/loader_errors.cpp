// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "common/common_funcs.h"
#include "core/crypto/key_manager.h"
#include "core/file_sys/content_archive.h"
#include "core/file_sys/vfs/vfs_vector.h"
#include "core/loader/loader.h"

using Loader::MissingKeysCanCause;
using Loader::ResultStatus;

namespace {

template <typename T>
void Put(std::vector<u8>& out, size_t offset, const T& value) {
    if (out.size() < offset + sizeof(T)) {
        out.resize(offset + sizeof(T));
    }
    std::memcpy(out.data() + offset, &value, sizeof(T));
}

FileSys::VirtualFile MakeFile(std::vector<u8> bytes) {
    return std::make_shared<FileSys::VectorVfsFile>(std::move(bytes), "test.nsp");
}

// A PFS0 holding the given files, like an NSP.
std::vector<u8> MakePfs0(const std::vector<std::pair<std::string, std::vector<u8>>>& files) {
    std::string strtab;
    std::vector<u32> name_offsets;
    for (const auto& file : files) {
        name_offsets.push_back(static_cast<u32>(strtab.size()));
        strtab += file.first;
        strtab += '\0';
    }
    const u32 num_entries = static_cast<u32>(files.size());
    std::vector<u8> out;
    Put(out, 0, Common::MakeMagic('P', 'F', 'S', '0'));
    Put(out, 4, num_entries);
    Put(out, 8, static_cast<u32>(strtab.size()));
    out.resize(0x10 + num_entries * 0x18);
    u64 offset = 0;
    for (u32 i = 0; i < num_entries; ++i) {
        Put(out, 0x10 + i * 0x18, offset);
        Put(out, 0x10 + i * 0x18 + 8, static_cast<u64>(files[i].second.size()));
        Put(out, 0x10 + i * 0x18 + 0x10, name_offsets[i]);
        offset += files[i].second.size();
    }
    out.insert(out.end(), strtab.begin(), strtab.end());
    for (const auto& file : files) {
        out.insert(out.end(), file.second.begin(), file.second.end());
    }
    return out;
}

} // Anonymous namespace

TEST_CASE("Missing keys explain an unreadable encrypted game", "[core][loader]") {
    // An NSP without keys: its NCA header can't be decrypted (0007-000C, Tears of the Kingdom).
    CHECK(MissingKeysCanCause(ResultStatus::ErrorBadNCAHeader));
    CHECK(MissingKeysCanCause(ResultStatus::ErrorBadXCIHeader));
    CHECK(MissingKeysCanCause(ResultStatus::ErrorMissingProductionKeyFile));
    CHECK(MissingKeysCanCause(ResultStatus::ErrorMissingHeaderKey));
    CHECK(MissingKeysCanCause(ResultStatus::ErrorMissingTitlekey));
    CHECK(MissingKeysCanCause(ResultStatus::ErrorMissingKeyAreaKey));
}

TEST_CASE("Missing keys don't explain other load errors", "[core][loader]") {
    CHECK_FALSE(MissingKeysCanCause(ResultStatus::Success));
    CHECK_FALSE(MissingKeysCanCause(ResultStatus::ErrorBadNPDMHeader));
    CHECK_FALSE(MissingKeysCanCause(ResultStatus::ErrorNoExeFS));
    CHECK_FALSE(MissingKeysCanCause(ResultStatus::ErrorNUPBadFormat));
    CHECK_FALSE(MissingKeysCanCause(ResultStatus::Error32BitISA));
}

TEST_CASE("NCA without a header key reports the missing key", "[core][loader]") {
    // From Zortos #1457: an encrypted header can't be read without the header key, and that is
    // what the load error says, not a bad header.
    if (Core::Crypto::KeyManager::Instance().HasKey(Core::Crypto::S256KeyType::Header)) {
        SKIP("A header key is installed");
    }
    const FileSys::NCA nca{
        std::make_shared<FileSys::VectorVfsFile>(std::vector<u8>(0xC00, 0xA5))};
    CHECK(nca.GetStatus() == ResultStatus::ErrorMissingHeaderKey);
    CHECK(MissingKeysCanCause(ResultStatus::ErrorMissingHeaderKey));
}

TEST_CASE("NSP that cannot be decrypted reports the missing key", "[core][loader]") {
    // From Zortos #1503: the game list lists such a file instead of hiding it.
    if (Core::Crypto::KeyManager::Instance().HasKey(Core::Crypto::S256KeyType::Header)) {
        SKIP("A header key is installed");
    }
    const auto nsp =
        MakeFile(MakePfs0({{"0123456789abcdef0123456789abcdef.cnmt.nca", std::vector<u8>(0xC00, 0xA5)}}));
    CHECK_FALSE(Loader::IsBootableGameContainer(nsp, Loader::FileType::NSP));
    CHECK(Loader::GetContainerProgramStatus(nsp, Loader::FileType::NSP) ==
          ResultStatus::ErrorMissingHeaderKey);
    CHECK(Loader::IsMissingKeyStatus(ResultStatus::ErrorMissingHeaderKey));
}

TEST_CASE("NSP without programs reports no missing key", "[core][loader]") {
    const auto nsp = MakeFile(MakePfs0({{"readme.txt", std::vector<u8>(0x10)}}));
    CHECK_FALSE(Loader::IsBootableGameContainer(nsp, Loader::FileType::NSP));
    CHECK(Loader::GetContainerProgramStatus(nsp, Loader::FileType::NSP) ==
          ResultStatus::ErrorNSPMissingProgramNCA);
    CHECK_FALSE(Loader::IsMissingKeyStatus(ResultStatus::ErrorNSPMissingProgramNCA));
}

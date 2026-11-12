// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <memory>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/crypto/key_manager.h"
#include "core/file_sys/content_archive.h"
#include "core/file_sys/vfs/vfs_vector.h"
#include "core/loader/loader.h"

using Loader::MissingKeysCanCause;
using Loader::ResultStatus;

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

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

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

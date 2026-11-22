// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <memory>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/file_sys/nca_metadata.h"
#include "core/file_sys/registered_cache.h"
#include "core/file_sys/vfs/vfs_vector.h"

using namespace FileSys;

// The game list's scan empties the provider of the updates and add-ons in the game folders and
// adds them back as it finds them. A boot cancels a running scan (Restart does at once), and the
// game then started without its update: the scan now keeps the old entries aside and swaps them
// back when cancelled (zortos293/Lichii#2298).
TEST_CASE("A content provider's entries can be set aside and put back", "[core][file_sys]") {
    constexpr u64 Base = 0x0400486024DEC000;
    constexpr u64 Update = Base | 0x800;
    const auto file = std::make_shared<VectorVfsFile>(std::vector<u8>{1, 2, 3}, "update.nca");

    ManualContentProvider provider;
    provider.AddEntryWithVersion(TitleType::Update, ContentRecordType::Program, Update, 0x10000,
                                 "1.0.1", file);
    REQUIRE(provider.HasEntry(Update, ContentRecordType::Program));
    REQUIRE(provider.ListUpdateVersions(Update).size() == 1);

    ManualContentProvider previous;
    provider.SwapEntries(previous);
    CHECK_FALSE(provider.HasEntry(Update, ContentRecordType::Program));
    CHECK(provider.ListUpdateVersions(Update).empty());
    CHECK(previous.HasEntry(Update, ContentRecordType::Program));

    // The cancelled scan found nothing yet: swapping back restores what it had.
    provider.SwapEntries(previous);
    CHECK(provider.HasEntry(Update, ContentRecordType::Program));
    CHECK(provider.ListUpdateVersions(Update).size() == 1);
    CHECK(provider.GetEntryUnparsed(Update, ContentRecordType::Program) == file);
    CHECK_FALSE(previous.HasEntry(Update, ContentRecordType::Program));
}

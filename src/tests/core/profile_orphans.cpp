// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <system_error>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "common/fs/path_util.h"
#include "common/settings.h"
#include "core/hle/service/acc/profile_manager.h"

namespace {

using Common::FS::EdenPath;
using Service::Account::ProfileManager;

/// A scratch NAND for the profile manager, restored afterwards.
struct NandFixture {
    NandFixture() {
        std::random_device random;
        do {
            nand = std::filesystem::temp_directory_path() /
                   ("kaichii-profile-orphans-" + std::to_string(random()));
        } while (!std::filesystem::create_directory(nand));
        Common::FS::SetEdenPath(EdenPath::NANDDir, nand);
    }
    ~NandFixture() {
        Common::FS::SetEdenPath(EdenPath::NANDDir, previous_nand);
        Settings::values.current_user = previous_user;
        std::error_code error;
        std::filesystem::remove_all(nand, error);
    }

    const std::filesystem::path previous_nand = Common::FS::GetEdenPath(EdenPath::NANDDir);
    const s32 previous_user = Settings::values.current_user.GetValue();
    std::filesystem::path nand;
};

} // Anonymous namespace

// FindOrphanedProfiles took every folder under save/0000000000000000 for a user ID, so a title
// folder copied in by hand showed the orphaned-profile dialog at every start, and an empty one was
// deleted. Only a 32-hex user ID is a profile candidate now (zortos293/Lichii#2265).
TEST_CASE("Only user ID folders count as orphaned profiles", "[core][acc]") {
    NandFixture fixture;
    Settings::values.current_user = 0;
    ProfileManager manager;
    const auto saves = fixture.nand / "user/save/0000000000000000";
    const auto title = saves / "0400486024DEC000";
    const auto other = saves / "foo";
    const auto orphan = std::string{"11111111111111111111111111111111"};
    const auto empty_orphan = std::string{"2222222222222222222222222222222a"};
    for (const auto& dir : {title / "sub", other / "sub", saves / orphan / "0100000000010000",
                            saves / empty_orphan / "0100000000010000"}) {
        REQUIRE(std::filesystem::create_directories(dir));
    }
    std::ofstream{saves / orphan / "0100000000010000/save.bin"} << "save";
    std::ofstream{title / "save.bin"} << "save";

    const auto orphaned = manager.FindOrphanedProfiles();
    CHECK(orphaned == std::vector{orphan});
    CHECK(std::filesystem::exists(title / "save.bin"));
    CHECK(std::filesystem::exists(other / "sub"));
    CHECK(std::filesystem::exists(saves / orphan / "0100000000010000/save.bin"));
    CHECK_FALSE(std::filesystem::exists(saves / empty_orphan));

    // Empty title and other folders are not removed either.
    std::filesystem::remove(title / "save.bin");
    CHECK(manager.FindOrphanedProfiles() == std::vector{orphan});
    CHECK(std::filesystem::exists(title / "sub"));
    CHECK(std::filesystem::exists(other / "sub"));
}

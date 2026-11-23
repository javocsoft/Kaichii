// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string_view>

#include "common/common_types.h"
#include "core/file_sys/romfs.h"

namespace Loader {

class SceneTracker;

constexpr u64 OcarinaSwitch2ProgramId = 0x0400486024DEC000ULL;
constexpr u64 BreathOfTheWildProgramId = 0x01007EF00011E000ULL;
constexpr u64 TearsOfTheKingdomProgramId = 0x0100F2C0115B6000ULL;

/// The scene a RomFS path belongs to in Ocarina of Time (Switch 2), or empty.
[[nodiscard]] std::string_view OcarinaSceneFromRomFSPath(std::string_view path);

/// The shrine or Divine Beast a RomFS path belongs to in Breath of the Wild, or empty.
[[nodiscard]] std::string_view BreathOfTheWildSceneFromRomFSPath(std::string_view path);

/// The scene a RomFS path belongs to in Tears of the Kingdom, or empty: the scene parameters the
/// game reads when it loads a shrine, a temple or the open world.
[[nodiscard]] std::string_view TearsOfTheKingdomSceneFromRomFSPath(std::string_view path);

/// An observer that notes the scenes a game with a Game Companion reads from its RomFS. Empty for
/// other games.
[[nodiscard]] FileSys::RomFSReadObserver CompanionSceneObserver(
    u64 program_id, std::shared_ptr<SceneTracker> tracker);

} // namespace Loader

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include <QImage>

#include "common/common_types.h"

/// Pictures the player chooses for a game, kept in Kaichii's data folder under the game's program
/// id, never inside the game file: they work for every format and survive moving the game. Files
/// copied there by hand count too. Program id 0 (no id) never has one.

/// The box art the full cover view shows: `box_art/<program id>.png` or `.jpg`.
namespace QtCommon::BoxArt {

/// Larger pictures are scaled down to this height when saved; it covers the biggest cover drawn
/// on a high-density screen.
inline constexpr int MaxStoredHeight = 1024;

/// The file holding a game's box art, if it has one.
[[nodiscard]] std::optional<std::filesystem::path> Find(u64 program_id);
/// The game's box art, or a null image.
[[nodiscard]] QImage Load(u64 program_id);
/// Stores the picture as the game's box art, replacing any earlier one: PNG when it has
/// transparency, JPEG otherwise.
bool Save(u64 program_id, const QImage& picture);
/// Removes the game's box art. True when none is left.
bool Remove(u64 program_id);

} // namespace QtCommon::BoxArt

/// An icon shown instead of the one in the game file: `custom_icons/<program id>.png`. NUP games
/// keep their icon inside the file instead, and don't use these.
namespace QtCommon::CustomIcon {

/// Icons are stored square, at the size a game file's icon has.
inline constexpr int StoredSide = 256;

/// The file holding a game's custom icon, if it has one.
[[nodiscard]] std::optional<std::filesystem::path> Find(u64 program_id);
/// The custom icon's file contents, as the game list takes a game's icon; empty without one.
[[nodiscard]] std::vector<u8> ReadBytes(u64 program_id);
/// The game's custom icon, or a null image.
[[nodiscard]] QImage Load(u64 program_id);
/// The picture as an icon: cropped to a square, the upper part of a tall picture kept, at
/// StoredSide.
[[nodiscard]] QImage Squared(const QImage& picture);
/// Stores the picture, Squared, as the game's icon, in PNG.
bool Save(u64 program_id, const QImage& picture);
/// Removes the game's custom icon. True when none is left.
bool Remove(u64 program_id);

} // namespace QtCommon::CustomIcon

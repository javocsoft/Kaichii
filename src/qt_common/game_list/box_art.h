// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <optional>

#include <QImage>

#include "common/common_types.h"

/// The box art the player chooses for a game, shown by the full cover view. It is kept in
/// Kaichii's data folder (`box_art/<program id>.png` or `.jpg`), never inside the game file, so it
/// works for every format and survives moving the game. Files copied there by hand count too.
namespace QtCommon::BoxArt {

/// Larger pictures are scaled down to this height when saved; it covers the biggest cover drawn
/// on a high-density screen.
inline constexpr int MaxStoredHeight = 1024;

/// The file holding a game's box art, if it has one. Program id 0 (no id) never has one.
[[nodiscard]] std::optional<std::filesystem::path> Find(u64 program_id);
/// The game's box art, or a null image.
[[nodiscard]] QImage Load(u64 program_id);
/// Stores the picture as the game's box art, replacing any earlier one: PNG when it has
/// transparency, JPEG otherwise.
bool Save(u64 program_id, const QImage& picture);
/// Removes the game's box art. True when none is left.
bool Remove(u64 program_id);

} // namespace QtCommon::BoxArt

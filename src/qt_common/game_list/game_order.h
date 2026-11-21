// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

#include "common/common_types.h"

class QStandardItem;

namespace QtCommon {

/// Whether game a sorts before game b in the list: by name as people read it (case and accents
/// aside, numbers by value, so "Mario 2" before "Mario 10"), then by path for two copies of a game.
[[nodiscard]] bool GameSortsBefore(const QString& title_a, const QString& path_a,
                                   const QString& title_b, const QString& path_b);

/// The row a game goes to among `parent`'s games, keeping them sorted by GameSortsBefore.
[[nodiscard]] int SortedGameRow(const QStandardItem& parent, const QString& title,
                                const QString& path);

/// Whether a program is a Switch 2 title: their program ids start with 04 (Switch titles with 01),
/// as the ids in AGENTS.md show. Switch games with a Switch 2 Edition upgrade keep their 01 id.
[[nodiscard]] constexpr bool IsSwitch2Program(u64 program_id) {
    return (program_id >> 56) == 0x04;
}

} // namespace QtCommon

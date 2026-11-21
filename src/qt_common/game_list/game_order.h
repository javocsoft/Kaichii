// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <Qt>
#include <QDateTime>
#include <QString>

#include "common/common_types.h"
#include "common/settings_enums.h"

class QStandardItem;
class QStandardItemModel;

namespace QtCommon {

/// How the flat views (grid, carousel and covers) order games.
struct GameSort {
    Settings::GameListSort key = Settings::GameListSort::Name;
    bool descending = false;
};

/// The order the player chose in View > Sort Games By.
[[nodiscard]] GameSort CurrentGameSort();

/// The direction a key starts in when the player picks it: names from A, the newest and the most
/// played first.
[[nodiscard]] constexpr bool DescendingByDefault(Settings::GameListSort key) {
    return key != Settings::GameListSort::Name;
}

/// Whether game a's name sorts before game b's: as people read it (case and accents aside, numbers
/// by value, so "Mario 2" before "Mario 10"), then by path for two copies of a game.
[[nodiscard]] bool GameSortsBefore(const QString& title_a, const QString& path_a,
                                   const QString& title_b, const QString& path_b);

/// Whether game a comes before game b in `sort`. The direction applies to the key; games equal in
/// it (never played, say) follow by name from A.
[[nodiscard]] bool GameSortsBefore(const QStandardItem& a, const QStandardItem& b, GameSort sort);

/// The row a game goes to among `parent`'s games, keeping them in `sort`.
[[nodiscard]] int SortedGameRow(const QStandardItem& parent, const QStandardItem& game,
                                GameSort sort);

/// Puts the model's games, a flat list, in `sort`.
void SortGames(QStandardItemModel& model, GameSort sort);

/// Sorts the game list for the view showing it. The tree sorts by the column the player picked;
/// the flat views share the model and keep the order games were inserted in, which a tree
/// column's order (say, descending) must not undo.
void SortForView(QStandardItemModel& model, bool tree_view, int column, Qt::SortOrder order);

/// When a game file arrived on disk: its creation time, or its last change where the file system
/// keeps none. Invalid for a path that isn't a file.
[[nodiscard]] QDateTime GameFileAdded(const QString& path);

/// Whether a program is a Switch 2 title: their program ids start with 04 (Switch titles with 01),
/// as the ids in AGENTS.md show. Switch games with a Switch 2 Edition upgrade keep their 01 id.
[[nodiscard]] constexpr bool IsSwitch2Program(u64 program_id) {
    return (program_id >> 56) == 0x04;
}

} // namespace QtCommon

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

// Declared, not included: uisettings.h puts names like Category in the global namespace,
// which clash with the Objective-C runtime's next to qt_common.cpp on macOS.
namespace UISettings {
struct GameDir;
}

/// The folders the game list shows games from, and the files it shows.
namespace QtCommon::GameFolders {

/// Whether the game list already shows the games in `folder`: it is one of `dirs`, or inside one
/// scanned with its subfolders. The console's own entries (SDMC, UserNAND, SysNAND) don't count.
[[nodiscard]] bool Covers(const QVector<UISettings::GameDir>& dirs, const QString& folder);

/// The file extensions the game list shows (QtCommon::supported_file_extensions).
[[nodiscard]] const QStringList& ListableExtensions();

/// Whether the game list would show `file`, by its extension.
[[nodiscard]] bool IsListable(const QString& file);

/// Whether `folder` holds, directly, a file the game list would show.
[[nodiscard]] bool HoldsGames(const QString& folder);

} // namespace QtCommon::GameFolders

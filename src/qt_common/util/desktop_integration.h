// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QString>

/// Adds Kaichii to the desktop's applications menu when it runs as an AppImage. Linux desktops
/// show a window's icon from the desktop entry that matches it, and an AppImage started from a
/// folder installs none, so without one the taskbar shows a generic icon.
namespace QtCommon::DesktopIntegration {

/// The line that marks an entry as written here; only such an entry is ever removed.
inline constexpr char Marker[] = "X-Kaichii-AppImage=true";

/// The entry's and the icon's file names under the data folder.
inline constexpr char EntryPath[] = "applications/io.github.javocsoft.kaichii.desktop";
inline constexpr char IconPath[] = "icons/hicolor/256x256/apps/io.github.javocsoft.kaichii.png";

/// `path` as one argument of a desktop entry's Exec key: quoted, with the characters the
/// specification reserves escaped.
[[nodiscard]] QString ExecArgument(const QString& path);

/// The desktop entry `shipped` (Kaichii's own, from dist/) for the AppImage at `appimage`: its
/// Exec and TryExec start that file, and it carries the Marker.
[[nodiscard]] QString Entry(const QString& shipped, const QString& appimage);

enum class Result {
    Unchanged, ///< Both files were already as they should be.
    Written,   ///< One or both were written.
    Failed,    ///< A file couldn't be written.
};

/// Writes the entry for `appimage` and the `icon` (a PNG) under `data_home`, the user's data
/// folder, unless they're already there as they should be.
Result Install(const QString& shipped, const QString& appimage, const QByteArray& icon,
               const QString& data_home);

/// Removes the entry and its icon from `data_home` if the entry carries the Marker. Returns
/// whether it removed them.
bool Remove(const QString& data_home);

/// The AppImage Kaichii runs from ($APPIMAGE), or empty when it doesn't run from one.
[[nodiscard]] QString CurrentAppImage();

/// The user's data folder: $XDG_DATA_HOME, or ~/.local/share.
[[nodiscard]] QString DataHome();

} // namespace QtCommon::DesktopIntegration

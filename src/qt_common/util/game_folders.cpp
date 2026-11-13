// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "qt_common/util/game_folders.h"

#include <QDir>
#include <QFileInfo>

namespace QtCommon::GameFolders {

namespace {

/// A folder path to compare: absolute, cleaned, without a trailing separator.
QString Normalized(const QString& folder) {
    return QDir::cleanPath(QFileInfo(folder).absoluteFilePath());
}

} // Anonymous namespace

bool Covers(const QVector<UISettings::GameDir>& dirs, const QString& folder) {
    const QString target = Normalized(folder);
    for (const auto& dir : dirs) {
        const QString path = QString::fromStdString(dir.path);
        if (path == QStringLiteral("SDMC") || path == QStringLiteral("UserNAND") ||
            path == QStringLiteral("SysNAND") || path.isEmpty()) {
            continue;
        }
        const QString root = Normalized(path);
        if (target == root) {
            return true;
        }
        if (dir.deep_scan && target.startsWith(root + QLatin1Char('/'))) {
            return true;
        }
    }
    return false;
}

const QStringList& ListableExtensions() {
    // Here rather than in qt_common.cpp, so code that only reads it doesn't pull in the core.
    static const QStringList extensions{
        QStringLiteral("nro"), QStringLiteral("nso"), QStringLiteral("nca"),
        QStringLiteral("xci"), QStringLiteral("xcz"), QStringLiteral("nsp"),
        QStringLiteral("nsz"), QStringLiteral("nup"), QStringLiteral("kip"),
    };
    return extensions;
}

bool IsListable(const QString& file) {
    return ListableExtensions().contains(QFileInfo(file).suffix(), Qt::CaseInsensitive);
}

bool HoldsGames(const QString& folder) {
    const QDir dir(folder);
    for (const QString& name : dir.entryList(QDir::Files)) {
        if (IsListable(name)) {
            return true;
        }
    }
    return false;
}

} // namespace QtCommon::GameFolders

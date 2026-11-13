// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "qt_common/util/desktop_integration.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStringList>

namespace QtCommon::DesktopIntegration {

namespace {

/// Writes `contents` to `path` unless it already holds them, and returns whether it wrote. Clears
/// `ok` when writing fails.
bool WriteIfChanged(const QString& path, const QByteArray& contents, bool& ok) {
    QFile current(path);
    if (current.open(QIODevice::ReadOnly) && current.readAll() == contents) {
        return false;
    }
    current.close();
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        ok = false;
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(contents) != contents.size() ||
        !file.commit()) {
        ok = false;
        return false;
    }
    return true;
}

} // Anonymous namespace

QString ExecArgument(const QString& path) {
    // Inside quotes, ", `, $ and \ take a backslash; the file's own string escaping then doubles
    // every backslash. A % starts a field code, so it is written twice.
    QString argument = QStringLiteral("\"");
    for (const QChar c : path) {
        if (c == QLatin1Char('"') || c == QLatin1Char('`') || c == QLatin1Char('$')) {
            argument += QStringLiteral("\\\\") + c;
        } else if (c == QLatin1Char('\\')) {
            argument += QStringLiteral("\\\\\\\\");
        } else if (c == QLatin1Char('%')) {
            argument += QStringLiteral("%%");
        } else {
            argument += c;
        }
    }
    return argument + QLatin1Char('"');
}

QString Entry(const QString& shipped, const QString& appimage) {
    QStringList lines;
    for (const QString& line : shipped.split(QLatin1Char('\n'))) {
        if (line.startsWith(QStringLiteral("Exec="))) {
            lines << QStringLiteral("Exec=%1 %f").arg(ExecArgument(appimage));
        } else if (line.startsWith(QStringLiteral("TryExec="))) {
            // TryExec is a plain path: the entry disappears from menus if the AppImage does.
            lines << QStringLiteral("TryExec=") + appimage;
        } else {
            lines << line;
        }
    }
    while (!lines.isEmpty() && lines.back().isEmpty()) {
        lines.removeLast();
    }
    lines << QString::fromLatin1(Marker);
    return lines.join(QLatin1Char('\n')) + QLatin1Char('\n');
}

Result Install(const QString& shipped, const QString& appimage, const QByteArray& icon,
               const QString& data_home) {
    const QDir home(data_home);
    bool ok = true;
    const bool entry = WriteIfChanged(home.filePath(QString::fromLatin1(EntryPath)),
                                      Entry(shipped, appimage).toUtf8(), ok);
    const bool image =
        !icon.isEmpty() && WriteIfChanged(home.filePath(QString::fromLatin1(IconPath)), icon, ok);
    if (!ok) {
        return Result::Failed;
    }
    return entry || image ? Result::Written : Result::Unchanged;
}

bool Remove(const QString& data_home) {
    const QDir home(data_home);
    QFile entry(home.filePath(QString::fromLatin1(EntryPath)));
    if (!entry.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QStringList lines = QString::fromUtf8(entry.readAll()).split(QLatin1Char('\n'));
    entry.close();
    if (!lines.contains(QString::fromLatin1(Marker))) {
        return false;
    }
    QFile::remove(home.filePath(QString::fromLatin1(IconPath)));
    return entry.remove();
}

QString CurrentAppImage() {
#ifdef __linux__
    const QString path = qEnvironmentVariable("APPIMAGE");
    const QFileInfo info(path);
    if (!path.isEmpty() && info.isAbsolute() && info.isFile()) {
        return info.absoluteFilePath();
    }
#endif
    return {};
}

QString DataHome() {
    const QString xdg = qEnvironmentVariable("XDG_DATA_HOME");
    if (!xdg.isEmpty() && QDir::isAbsolutePath(xdg)) {
        return xdg;
    }
    return QDir::home().filePath(QStringLiteral(".local/share"));
}

} // namespace QtCommon::DesktopIntegration

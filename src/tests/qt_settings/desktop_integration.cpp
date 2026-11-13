// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// Adding Kaichii to the applications menu when it runs as an AppImage: the entry made from the
// shipped one, its Exec quoting, writing only on change, and removing only what was written here.

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

#include "qt_common/util/desktop_integration.h"

namespace Desktop = QtCommon::DesktopIntegration;

namespace {

QString Shipped() {
    QFile file(QStringLiteral(SHIPPED_DESKTOP_ENTRY));
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString{};
}

QByteArray Read(const QString& path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{};
}

const QString Image =
    QStringLiteral("/mnt/games/Emulator Linux (Kaichii build 92)/kaichii-linux-x86_64.AppImage");

} // Anonymous namespace

class DesktopIntegrationTest : public QObject {
    Q_OBJECT

private slots:
    void cleanup();
    void QuotesTheExecArgument();
    void MakesTheEntryFromTheShippedOne();
    void WritesOnlyWhatChanged();
    void RemovesOnlyItsOwnEntry();
    void ReportsAFolderItCannotWrite();
    void FindsTheDataFolder();
    void FindsTheAppImage();
};

void DesktopIntegrationTest::cleanup() {
    qunsetenv("XDG_DATA_HOME");
    qunsetenv("APPIMAGE");
}

void DesktopIntegrationTest::QuotesTheExecArgument() {
    QCOMPARE(Desktop::ExecArgument(Image), QLatin1Char('"') + Image + QLatin1Char('"'));
    // ", `, $ and \ take a backslash, which the file's string escaping doubles; % is doubled.
    QCOMPARE(Desktop::ExecArgument(QStringLiteral("/a\"b`c$d\\e%f")),
             QStringLiteral("\"/a\\\\\"b\\\\`c\\\\$d\\\\\\\\e%%f\""));
}

void DesktopIntegrationTest::MakesTheEntryFromTheShippedOne() {
    const QString shipped = Shipped();
    QVERIFY2(!shipped.isEmpty(), SHIPPED_DESKTOP_ENTRY);
    // The window's X11 class is the application name: the entry must name it to match.
    QVERIFY(shipped.contains(QStringLiteral("\nStartupWMClass=Kaichii\n")));
    const QStringList lines = Desktop::Entry(shipped, Image).split(QLatin1Char('\n'));
    QVERIFY(lines.contains(QStringLiteral("Exec=\"%1\" %f").arg(Image)));
    QVERIFY(lines.contains(QStringLiteral("TryExec=") + Image));
    QCOMPARE(lines.at(lines.size() - 2), QString::fromLatin1(Desktop::Marker));
    // Everything else is the shipped entry, line for line.
    for (const QString& line : shipped.split(QLatin1Char('\n'))) {
        if (!line.isEmpty() && !line.startsWith(QStringLiteral("Exec=")) &&
            !line.startsWith(QStringLiteral("TryExec="))) {
            QVERIFY2(lines.contains(line), qPrintable(line));
        }
    }
}

void DesktopIntegrationTest::WritesOnlyWhatChanged() {
    QTemporaryDir home;
    QVERIFY(home.isValid());
    const QByteArray icon("\x89PNG synthetic icon");
    const QString shipped = Shipped();
    QCOMPARE(Desktop::Install(shipped, Image, icon, home.path()), Desktop::Result::Written);
    const QString entry = QDir(home.path()).filePath(QString::fromLatin1(Desktop::EntryPath));
    const QString image = QDir(home.path()).filePath(QString::fromLatin1(Desktop::IconPath));
    QCOMPARE(Read(entry), Desktop::Entry(shipped, Image).toUtf8());
    QCOMPARE(Read(image), icon);
    QCOMPARE(Desktop::Install(shipped, Image, icon, home.path()), Desktop::Result::Unchanged);
    // A newer AppImage somewhere else: the entry follows it.
    const QString moved = QStringLiteral("/opt/kaichii/kaichii-linux-x86_64-pgo.AppImage");
    QCOMPARE(Desktop::Install(shipped, moved, icon, home.path()), Desktop::Result::Written);
    QVERIFY(QString::fromUtf8(Read(entry)).contains(QStringLiteral("TryExec=") + moved));
}

void DesktopIntegrationTest::RemovesOnlyItsOwnEntry() {
    QTemporaryDir home;
    QVERIFY(home.isValid());
    const QString entry = QDir(home.path()).filePath(QString::fromLatin1(Desktop::EntryPath));
    const QString image = QDir(home.path()).filePath(QString::fromLatin1(Desktop::IconPath));
    QVERIFY(!Desktop::Remove(home.path()));

    QCOMPARE(Desktop::Install(Shipped(), Image, QByteArray("icon"), home.path()),
             Desktop::Result::Written);
    QVERIFY(Desktop::Remove(home.path()));
    QVERIFY(!QFile::exists(entry));
    QVERIFY(!QFile::exists(image));

    // An entry the player wrote by hand, without the marker, stays.
    QVERIFY(QDir().mkpath(QFileInfo(entry).absolutePath()));
    QFile own(entry);
    QVERIFY(own.open(QIODevice::WriteOnly));
    own.write("[Desktop Entry]\nName=Kaichii\nExec=kaichii\n");
    own.close();
    QVERIFY(!Desktop::Remove(home.path()));
    QVERIFY(QFile::exists(entry));
}

void DesktopIntegrationTest::ReportsAFolderItCannotWrite() {
    QTemporaryDir home;
    QVERIFY(home.isValid());
    // The data folder is a file: nothing can be written under it.
    const QString blocked = QDir(home.path()).filePath(QStringLiteral("not-a-folder"));
    QFile file(blocked);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
    QCOMPARE(Desktop::Install(Shipped(), Image, QByteArray("icon"), blocked),
             Desktop::Result::Failed);
}

void DesktopIntegrationTest::FindsTheDataFolder() {
    qputenv("XDG_DATA_HOME", "/data/home");
    QCOMPARE(Desktop::DataHome(), QStringLiteral("/data/home"));
    qputenv("XDG_DATA_HOME", "relative/home");
    QCOMPARE(Desktop::DataHome(), QDir::home().filePath(QStringLiteral(".local/share")));
    qunsetenv("XDG_DATA_HOME");
    QCOMPARE(Desktop::DataHome(), QDir::home().filePath(QStringLiteral(".local/share")));
}

void DesktopIntegrationTest::FindsTheAppImage() {
    QCOMPARE(Desktop::CurrentAppImage(), QString{});
    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    const QString image = QDir(folder.path()).filePath(QStringLiteral("Kaichii.AppImage"));
    QFile file(image);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
    qputenv("APPIMAGE", image.toUtf8());
#ifdef __linux__
    QCOMPARE(Desktop::CurrentAppImage(), image);
#else
    QCOMPARE(Desktop::CurrentAppImage(), QString{});
#endif
    qputenv("APPIMAGE", "Kaichii.AppImage");
    QCOMPARE(Desktop::CurrentAppImage(), QString{});
    qputenv("APPIMAGE", (image + QStringLiteral(".missing")).toUtf8());
    QCOMPARE(Desktop::CurrentAppImage(), QString{});
}

QTEST_GUILESS_MAIN(DesktopIntegrationTest)
#include "desktop_integration.moc"

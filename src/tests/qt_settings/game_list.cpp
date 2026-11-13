// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "qt_common/config/uisettings.h"
#include "qt_common/game_list/game_list_p.h"
#include "qt_common/game_list/placeholder_icon.h"
#include "qt_common/util/game_folders.h"

class GameListTest : public QObject {
    Q_OBJECT

private slots:
    void InitialsComeFromTheFirstTwoWords();
    void IconIsDrawnAtItsSize();
    void AddAndFavoritesRowsAloneAreEmpty();
    void FoldersCoverTheirGamesAndDeepScannedSubfolders();
    void ListsGameFilesByExtension();
};

void GameListTest::InitialsComeFromTheFirstTwoWords() {
    using QtCommon::PlaceholderInitials;
    QCOMPARE(PlaceholderInitials(QStringLiteral("Donkey Kong Bananza")), QStringLiteral("DK"));
    QCOMPARE(PlaceholderInitials(QStringLiteral("the Legend of Zelda: Ocarina of Time")),
             QStringLiteral("TL"));
    QCOMPARE(PlaceholderInitials(QStringLiteral("Ōkami")), QStringLiteral("Ō"));
    QCOMPARE(PlaceholderInitials(QStringLiteral("  -  ")), QString{});
}

void GameListTest::IconIsDrawnAtItsSize() {
    const QImage icon = QtCommon::PlaceholderIcon(QStringLiteral("Donkey Kong Bananza"), 64);
    QCOMPARE(icon.size(), QSize(64, 64));
    // A filled square, transparent only at its rounded corners.
    QVERIFY(qAlpha(icon.pixel(32, 5)) == 255);
    QVERIFY(qAlpha(icon.pixel(0, 0)) < 255);
    // The same name gives the same color.
    const QImage again = QtCommon::PlaceholderIcon(QStringLiteral("Donkey Kong Bananza"), 64);
    QCOMPARE(icon.pixel(32, 5), again.pixel(32, 5));
}

void GameListTest::AddAndFavoritesRowsAloneAreEmpty() {
    QStandardItem root;
    QVERIFY(HoldsNoGames(root));
    root.appendRow(new GameListAddDir());
    root.appendRow(new GameListFavorites());
    QVERIFY(HoldsNoGames(root));
    UISettings::GameDir folder{"/games", false, true};
    root.appendRow(new GameListDir(folder));
    QVERIFY(!HoldsNoGames(root));
}

void GameListTest::FoldersCoverTheirGamesAndDeepScannedSubfolders() {
    using QtCommon::GameFolders::Covers;
    const QVector<UISettings::GameDir> dirs{
        {"SDMC", false, true},
        {"/games/Ocarina of Time", false, true},
        {"/library", true, true},
    };
    QVERIFY(Covers(dirs, QStringLiteral("/games/Ocarina of Time")));
    QVERIFY(Covers(dirs, QStringLiteral("/games/Ocarina of Time/")));
    QVERIFY(Covers(dirs, QStringLiteral("/games/./Ocarina of Time")));
    // Only a folder scanned with its subfolders covers them.
    QVERIFY(!Covers(dirs, QStringLiteral("/games/Ocarina of Time/extras")));
    QVERIFY(Covers(dirs, QStringLiteral("/library/switch2/dk")));
    QVERIFY(!Covers(dirs, QStringLiteral("/library-old")));
    QVERIFY(!Covers(dirs, QStringLiteral("/games/donkey_kong_bananza")));
    // The console's own entries are not folders on disk.
    QVERIFY(!Covers(dirs, QStringLiteral("SDMC")));
    QVERIFY(!Covers({}, QStringLiteral("/games")));
}

void GameListTest::ListsGameFilesByExtension() {
    using namespace QtCommon::GameFolders;
    QVERIFY(IsListable(QStringLiteral("/games/Donkey Kong Bananza.nup")));
    QVERIFY(IsListable(QStringLiteral("game.NSP")));
    QVERIFY(!IsListable(QStringLiteral("readme.txt")));
    QVERIFY(!IsListable(QStringLiteral("/extracted/exefs/main")));

    QTemporaryDir folder;
    QVERIFY(folder.isValid());
    const auto touch = [&](const char* name) {
        QFile file(QDir(folder.path()).filePath(QString::fromLatin1(name)));
        QVERIFY(file.open(QIODevice::WriteOnly));
    };
    touch("readme.txt");
    QVERIFY(!HoldsGames(folder.path()));
    QVERIFY(QDir(folder.path()).mkdir(QStringLiteral("deeper.nup")));
    QVERIFY(!HoldsGames(folder.path())); // A folder named like a game is not one.
    touch("Donkey Kong Bananza.nup");
    QVERIFY(HoldsGames(folder.path()));
}

QTEST_MAIN(GameListTest)
#include "game_list.moc"

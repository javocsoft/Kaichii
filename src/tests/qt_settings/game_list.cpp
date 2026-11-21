// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib>
#include <QDir>
#include <QBuffer>
#include <QFile>
#include <QImageReader>
#include <QPainter>
#include <QStandardItemModel>
#include <QStyleOptionViewItem>
#include <QTemporaryDir>
#include <QtTest>

#include "qt_common/config/uisettings.h"
#include "qt_common/game_list/game_list_p.h"
#include "qt_common/game_list/placeholder_icon.h"
#include "qt_common/util/game_folders.h"
#include "yuzu/game/cover_art.h"
#include "yuzu/game/game_covers.h"

class GameListTest : public QObject {
    Q_OBJECT

private slots:
    void InitialsComeFromTheFirstTwoWords();
    void IconIsDrawnAtItsSize();
    void CoverKeepsThePicturesTransparency();
    void CoverFillsItsSquareWithRoundedCorners();
    void CoverShadowFollowsThePicturesShape();
    void ShelfShowsThroughATransparentCover();
    void WebPPicturesCanBeRead();
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

namespace {

/// A 64x64 picture, transparent but for an opaque red square in its middle.
QImage CutOutPicture() {
    QImage picture(64, 64, QImage::Format_ARGB32);
    picture.fill(Qt::transparent);
    for (int y = 16; y < 48; ++y) {
        for (int x = 16; x < 48; ++x) {
            picture.setPixel(x, y, qRgba(255, 0, 0, 255));
        }
    }
    return picture;
}

} // Anonymous namespace

void GameListTest::CoverKeepsThePicturesTransparency() {
    // A cover with transparent parts (Donkey Kong Bananza's) must show the shelf behind them.
    const QImage cover = Yuzu::FitCover(CutOutPicture(), 64, 6);
    QCOMPARE(cover.size(), QSize(64, 64));
    QCOMPARE(qAlpha(cover.pixel(8, 32)), 0);
    QCOMPARE(qAlpha(cover.pixel(32, 8)), 0);
    QCOMPARE(qAlpha(cover.pixel(32, 32)), 255);
    QVERIFY(qRed(cover.pixel(32, 32)) > 250);
}

void GameListTest::CoverFillsItsSquareWithRoundedCorners() {
    // A wide picture is cropped to the square rather than letterboxed.
    QImage picture(128, 64, QImage::Format_RGB32);
    picture.fill(qRgb(0, 0, 255));
    const QImage cover = Yuzu::FitCover(picture, 64, 10);
    QCOMPARE(cover.size(), QSize(64, 64));
    QCOMPARE(qAlpha(cover.pixel(0, 0)), 0);
    QCOMPARE(qAlpha(cover.pixel(63, 63)), 0);
    QCOMPARE(qAlpha(cover.pixel(0, 32)), 255);
    QCOMPARE(qAlpha(cover.pixel(32, 0)), 255);
    QCOMPARE(qAlpha(cover.pixel(32, 32)), 255);
}

void GameListTest::CoverShadowFollowsThePicturesShape() {
    const QImage shadow = Yuzu::CoverShadow(Yuzu::FitCover(CutOutPicture(), 64, 6), 22);
    QCOMPARE(qAlpha(shadow.pixel(8, 32)), 0);
    QVERIFY(std::abs(qAlpha(shadow.pixel(32, 32)) - 22) <= 1);
    QCOMPARE(qRed(shadow.pixel(32, 32)), 0);
}

void GameListTest::ShelfShowsThroughATransparentCover() {
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    QVERIFY(CutOutPicture().save(&buffer, "PNG"));

    QStandardItemModel model;
    auto* item = new QStandardItem;
    item->setData(static_cast<int>(GameListItemType::Game), GameListItem::TypeRole);
    item->setData(QStringLiteral("/games/cut-out.nsp"), GameListItemPath::FullPathRole);
    item->setData(QStringLiteral("Cut Out"), GameListItemPath::TitleRole);
    item->setData(png, GameListItemPath::CoverRole);
    model.appendRow(item);

    GameCoverDelegate delegate;
    delegate.SetSizes(64, QSize(80, 80));
    QStyleOptionViewItem option;
    option.rect = QRect(0, 0, 80, 80);
    QImage shelf(80, 80, QImage::Format_ARGB32);
    shelf.fill(Qt::white);
    {
        QPainter painter(&shelf);
        delegate.paint(&painter, option, model.index(0, 0));
    }
    const QRect cover = delegate.CoverRect(option.rect);
    // The transparent left of the picture shows the shelf, not a grey square.
    QCOMPARE(shelf.pixel(cover.left() + 8, cover.top() + 32), qRgb(255, 255, 255));
    QCOMPARE(shelf.pixel(cover.center()), qRgb(255, 0, 0));
    // The shadow sits just under the picture's own bottom edge.
    QVERIFY(qGray(shelf.pixel(cover.left() + 32, cover.top() + 49)) < 255);
}

void GameListTest::WebPPicturesCanBeRead() {
    // Icons and box art are often WebP. Qt reads it only with its image formats plugin
    // (qtimageformats), which every build must install and ship.
    QVERIFY2(QImageReader::supportedImageFormats().contains("webp"),
             "Qt's WebP plugin is missing: install the qtimageformats module");
    QByteArray webp;
    QBuffer buffer(&webp);
    buffer.open(QIODevice::WriteOnly);
    QVERIFY(CutOutPicture().save(&buffer, "WEBP", 100));
    buffer.close();
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer);
    QCOMPARE(reader.format(), QByteArray("webp"));
    const QImage read = reader.read();
    QCOMPARE(read.size(), QSize(64, 64));
    QCOMPARE(qAlpha(read.pixel(8, 32)), 0);
    QCOMPARE(qAlpha(read.pixel(32, 32)), 255);
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

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
#include "qt_common/game_list/game_order.h"
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
    void GamesSortByNameAsPeopleReadIt();
    void NewGamesTakeTheirPlaceByName();
    void ATreeColumnOrderLeavesTheFlatViewsByName();
    void Switch2TitlesAreToldByTheirId();
    void ShelfKeepsTheListsOrder();
    void SectionsPutSwitch2GamesFirst();
    void ArrowKeysMoveAcrossTheShelf();
    void FilteredGamesLeaveTheShelf();
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
    const QImage cover = Yuzu::FitCover(CutOutPicture(), QSize(64, 64), 6);
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
    const QImage cover = Yuzu::FitCover(picture, QSize(64, 64), 10);
    QCOMPARE(cover.size(), QSize(64, 64));
    QCOMPARE(qAlpha(cover.pixel(0, 0)), 0);
    QCOMPARE(qAlpha(cover.pixel(63, 63)), 0);
    QCOMPARE(qAlpha(cover.pixel(0, 32)), 255);
    QCOMPARE(qAlpha(cover.pixel(32, 0)), 255);
    QCOMPARE(qAlpha(cover.pixel(32, 32)), 255);
}

void GameListTest::CoverShadowFollowsThePicturesShape() {
    const QImage shadow = Yuzu::CoverShadow(Yuzu::FitCover(CutOutPicture(), QSize(64, 64), 6), 22);
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
    delegate.SetSizes(QSize(64, 64), QSize(80, 80));
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

void GameListTest::GamesSortByNameAsPeopleReadIt() {
    using QtCommon::GameSortsBefore;
    const QString path = QStringLiteral("/games/a.nsp");
    // Case aside, and numbers by value.
    QVERIFY(GameSortsBefore(QStringLiteral("animal Crossing"), path,
                            QStringLiteral("Bananza"), path));
    QVERIFY(GameSortsBefore(QStringLiteral("Mario Kart 8"), path,
                            QStringLiteral("mario kart 10"), path));
    QVERIFY(!GameSortsBefore(QStringLiteral("Zelda"), path, QStringLiteral("Tunic"), path));
    QVERIFY(GameSortsBefore(QStringLiteral("Ōkami"), path, QStringLiteral("Overcooked"), path));
    QVERIFY(GameSortsBefore(QStringLiteral("Game 007"), path, QStringLiteral("Game 8"), path));
    // Two copies of a game: by path.
    QVERIFY(GameSortsBefore(QStringLiteral("Tunic"), QStringLiteral("/a/tunic.nsp"),
                            QStringLiteral("Tunic"), QStringLiteral("/b/tunic.xci")));
    // A game without a title sorts by its file name.
    QVERIFY(GameSortsBefore(QString{}, QStringLiteral("/games/Astral Chain.xci"),
                            QStringLiteral("Bayonetta"), path));
}

namespace {

QStandardItem* Game(const QString& title, u64 program_id = 0x0100000000010000ULL) {
    auto* const item = new QStandardItem;
    item->setData(static_cast<int>(GameListItemType::Game), GameListItem::TypeRole);
    item->setData(title, GameListItemPath::TitleRole);
    item->setData(QStringLiteral("/games/%1.nsp").arg(title), GameListItemPath::FullPathRole);
    item->setData(qulonglong(program_id), GameListItemPath::ProgramIdRole);
    return item;
}

QStringList Titles(const QStandardItemModel& model) {
    QStringList titles;
    for (int row = 0; row < model.rowCount(); ++row) {
        titles << model.item(row)->data(GameListItemPath::TitleRole).toString();
    }
    return titles;
}

constexpr u64 Switch2Id = 0x0400486024DEC000ULL;

} // Anonymous namespace

void GameListTest::NewGamesTakeTheirPlaceByName() {
    QStandardItemModel model;
    for (const char* title : {"Tunic", "Donkey Kong Bananza", "astral chain", "Zelda", "Mario 10",
                              "Mario 2"}) {
        const QString name = QString::fromLatin1(title);
        model.insertRow(QtCommon::SortedGameRow(*model.invisibleRootItem(), name,
                                                QStringLiteral("/games/%1.nsp").arg(name)),
                        Game(name));
    }
    QCOMPARE(Titles(model), (QStringList{QStringLiteral("astral chain"),
                                         QStringLiteral("Donkey Kong Bananza"),
                                         QStringLiteral("Mario 2"), QStringLiteral("Mario 10"),
                                         QStringLiteral("Tunic"), QStringLiteral("Zelda")}));
}

void GameListTest::ATreeColumnOrderLeavesTheFlatViewsByName() {
    // The tree's header remembered a descending order: the cover views, which share the model,
    // showed games from Z to A.
    QStandardItemModel model;
    for (const char* title : {"A Short Hike", "Axiom Verge", "Tunic"}) {
        QStandardItem* const game = Game(QString::fromLatin1(title));
        game->setText(QString::fromLatin1(title));
        model.appendRow(game);
    }
    const QStringList by_name = Titles(model);
    QtCommon::SortForView(model, false, 0, Qt::DescendingOrder);
    QCOMPARE(Titles(model), by_name);
    // The tree itself still follows its column.
    QtCommon::SortForView(model, true, 0, Qt::DescendingOrder);
    QCOMPARE(Titles(model), (QStringList{QStringLiteral("Tunic"), QStringLiteral("Axiom Verge"),
                                         QStringLiteral("A Short Hike")}));
}

void GameListTest::Switch2TitlesAreToldByTheirId() {
    QVERIFY(QtCommon::IsSwitch2Program(0x0400486024DEC000ULL)); // Ocarina of Time
    QVERIFY(QtCommon::IsSwitch2Program(0x040074A01BF12000ULL)); // Donkey Kong Bananza
    QVERIFY(!QtCommon::IsSwitch2Program(0x010015100B514000ULL)); // Mario Wonder, a Switch program
    QVERIFY(!QtCommon::IsSwitch2Program(0));
}

namespace {

/// A shelf of square covers, shown so it lays out at a known width.
struct TestShelf {
    QStandardItemModel model;
    GameCovers view{CoverShape::Square};

    TestShelf() {
        // Name order: Astral (Switch 2), Bayonetta, Celeste (Switch 2), Dredge, Eastward.
        model.appendRow(Game(QStringLiteral("Astral"), Switch2Id));
        model.appendRow(Game(QStringLiteral("Bayonetta")));
        model.appendRow(Game(QStringLiteral("Celeste"), Switch2Id + 0x1000));
        model.appendRow(Game(QStringLiteral("Dredge")));
        model.appendRow(Game(QStringLiteral("Eastward")));
        view.SetModel(&model);
        // Room for two columns of covers.
        view.resize(2 * (Yuzu::TargetCover + Yuzu::CoverGap) + 40, 900);
        view.show();
        view.UpdateCoverSize();
    }

    QRect Rect(int row) {
        return view.visualRect(model.index(row, 0));
    }
};

} // Anonymous namespace

void GameListTest::ShelfKeepsTheListsOrder() {
    TestShelf shelf;
    // Two per row, in the model's order.
    QCOMPARE(shelf.Rect(0).top(), shelf.Rect(1).top());
    QVERIFY(shelf.Rect(0).left() < shelf.Rect(1).left());
    QVERIFY(shelf.Rect(2).top() > shelf.Rect(1).top());
    QCOMPARE(shelf.Rect(2).left(), shelf.Rect(0).left());
    // A click on a cover picks that game, and the gap around it picks nothing.
    const QRect cover = shelf.Rect(3);
    QCOMPARE(shelf.view.indexAt(cover.center()), shelf.model.index(3, 0));
    QVERIFY(!shelf.view.indexAt(cover.topLeft()).isValid());
}

void GameListTest::SectionsPutSwitch2GamesFirst() {
    TestShelf shelf;
    shelf.view.SetSections(true);
    // Astral and Celeste, the Switch 2 games, share the first row, below their heading.
    QCOMPARE(shelf.Rect(0).top(), shelf.Rect(2).top());
    QVERIFY(shelf.Rect(0).top() >= Yuzu::ShelfHeadingHeight);
    // Bayonetta, Dredge and Eastward follow under the second heading, starting a row.
    QVERIFY(shelf.Rect(1).top() >= shelf.Rect(0).bottom() + Yuzu::ShelfHeadingHeight);
    QCOMPARE(shelf.Rect(1).left(), shelf.Rect(0).left());
    QCOMPARE(shelf.Rect(1).top(), shelf.Rect(3).top());
    QVERIFY(shelf.Rect(4).top() > shelf.Rect(3).top());
    // Turned off, the list's order is back.
    shelf.view.SetSections(false);
    QCOMPARE(shelf.Rect(0).top(), shelf.Rect(1).top());
}

void GameListTest::ArrowKeysMoveAcrossTheShelf() {
    TestShelf shelf;
    shelf.view.SetSections(true);
    shelf.view.setCurrentIndex(shelf.model.index(0, 0));
    QTest::keyClick(&shelf.view, Qt::Key_Right);
    QCOMPARE(shelf.view.currentIndex().row(), 2);
    // Down from Celeste, in the second column, lands on Dredge under it in the next section.
    QTest::keyClick(&shelf.view, Qt::Key_Down);
    QCOMPARE(shelf.view.currentIndex().row(), 3);
    QTest::keyClick(&shelf.view, Qt::Key_Left);
    QCOMPARE(shelf.view.currentIndex().row(), 1);
    QTest::keyClick(&shelf.view, Qt::Key_Up);
    QCOMPARE(shelf.view.currentIndex().row(), 0);
    QTest::keyClick(&shelf.view, Qt::Key_End);
    QCOMPARE(shelf.view.currentIndex().row(), 4);
}

void GameListTest::FilteredGamesLeaveTheShelf() {
    TestShelf shelf;
    shelf.view.ApplyFilter(QStringLiteral("dredge"), &shelf.model);
    QVERIFY(shelf.Rect(0).isNull());
    QVERIFY(!shelf.Rect(3).isNull());
    // Dredge, the only one left, takes the first place.
    QCOMPARE(shelf.Rect(3).topLeft(), QPoint(0, 0));
    shelf.view.ApplyFilter(QString{}, &shelf.model);
    QVERIFY(!shelf.Rect(0).isNull());
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

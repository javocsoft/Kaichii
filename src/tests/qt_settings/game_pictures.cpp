// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>

#include <QBuffer>
#include <QPainter>
#include <QPushButton>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QStyleOptionViewItem>
#include <QTemporaryDir>
#include <QtTest>

#include "common/fs/path_util.h"
#include "qt_common/game_list/game_pictures.h"
#include "qt_common/game_list/game_list_p.h"
#include "yuzu/configuration/box_art_editor.h"
#include "yuzu/configuration/custom_icon_editor.h"
#include "yuzu/game/cover_layout.h"
#include "yuzu/game/game_covers.h"

namespace {

constexpr u64 GameId = 0x0400486024DEC000ULL;

QImage Filled(QSize size, QColor color, QImage::Format format = QImage::Format_RGB32) {
    QImage image(size, format);
    image.fill(color);
    return image;
}

QString NoFile(QWidget*) {
    return {};
}

QImage NoImage(const QString&) {
    return {};
}

std::filesystem::path StoredFile(const char* extension,
                                 Common::FS::EdenPath dir = Common::FS::EdenPath::BoxArtDir) {
    return Common::FS::GetEdenPath(dir) / (std::string("0400486024DEC000") + extension);
}

/// A tall picture: red in its upper 170 rows, blue below.
QImage RedOverBlue() {
    QImage picture(100, 300, QImage::Format_RGB32);
    picture.fill(Qt::blue);
    for (int y = 0; y < 170; ++y) {
        for (int x = 0; x < 100; ++x) {
            picture.setPixel(x, y, qRgb(255, 0, 0));
        }
    }
    return picture;
}

} // Anonymous namespace

class GamePicturesTest : public QObject {
    Q_OBJECT

private slots:
    void init();

    void OpaqueArtIsStoredAsJpegAndScaledDown();
    void TransparentArtIsStoredAsPngReplacingTheOldOne();
    void ArtCopiedByHandIsFound();
    void RemovingLeavesNoArt();
    void AGameWithoutAnIdHasNoArt();

    void EditorSavesOnlyWhenAsked();
    void EditorRemovesTheArt();
    void EditorOffersNothingWithoutAnId();

    void FullCoverViewDrawsTheBoxArt();
    void FullCoverViewPutsTheIconOnACaseWithoutArt();

    void CustomIconIsStoredSquareAsPng();
    void CustomIconKeepsATallPicturesUpperPart();
    void CustomIconBytesAreWhatTheListLoads();
    void IconEditorPreviewsSavesAndRestores();
    void IconEditorOffersNothingWithoutAnId();

private:
    void PaintBox(QStandardItemModel& model, QImage& shelf, QRect& box);

    QTemporaryDir data_dir;
};

void GamePicturesTest::init() {
    QVERIFY(data_dir.isValid());
    for (const auto& [kind, name] : {std::pair{Common::FS::EdenPath::BoxArtDir, "box_art"},
                                     std::pair{Common::FS::EdenPath::CustomIconsDir,
                                               "custom_icons"}}) {
        const std::filesystem::path dir = std::filesystem::path(data_dir.path().toStdString()) / name;
        std::filesystem::remove_all(dir);
        // The path is only taken when it exists.
        std::filesystem::create_directories(dir);
        Common::FS::SetEdenPath(kind, dir);
        QCOMPARE(Common::FS::GetEdenPath(kind), dir);
    }
}

void GamePicturesTest::OpaqueArtIsStoredAsJpegAndScaledDown() {
    QVERIFY(QtCommon::BoxArt::Save(GameId, Filled({1050, 1700}, Qt::red)));
    QVERIFY(std::filesystem::exists(StoredFile(".jpg")));
    const QImage loaded = QtCommon::BoxArt::Load(GameId);
    QCOMPARE(loaded.height(), QtCommon::BoxArt::MaxStoredHeight);
    QCOMPARE(loaded.width(), 1050 * QtCommon::BoxArt::MaxStoredHeight / 1700);
}

void GamePicturesTest::TransparentArtIsStoredAsPngReplacingTheOldOne() {
    QVERIFY(QtCommon::BoxArt::Save(GameId, Filled({105, 170}, Qt::red)));
    QImage cut_out = Filled({105, 170}, Qt::blue, QImage::Format_ARGB32);
    cut_out.setPixel(0, 0, qRgba(0, 0, 0, 0));
    QVERIFY(QtCommon::BoxArt::Save(GameId, cut_out));
    QVERIFY(std::filesystem::exists(StoredFile(".png")));
    QVERIFY(!std::filesystem::exists(StoredFile(".jpg")));
    const QImage loaded = QtCommon::BoxArt::Load(GameId);
    QCOMPARE(qAlpha(loaded.pixel(0, 0)), 0);
    QCOMPARE(loaded.pixel(50, 80), qRgb(0, 0, 255));
}

void GamePicturesTest::ArtCopiedByHandIsFound() {
    std::filesystem::create_directories(StoredFile(".png").parent_path());
    QVERIFY(Filled({105, 170}, Qt::green)
                .save(QString::fromStdString(StoredFile(".jpeg").string()), "JPEG"));
    const auto found = QtCommon::BoxArt::Find(GameId);
    QVERIFY(found.has_value());
    QCOMPARE(QtCommon::BoxArt::Load(GameId).size(), QSize(105, 170));
}

void GamePicturesTest::RemovingLeavesNoArt() {
    QVERIFY(QtCommon::BoxArt::Save(GameId, Filled({105, 170}, Qt::red)));
    QVERIFY(QtCommon::BoxArt::Remove(GameId));
    QVERIFY(!QtCommon::BoxArt::Find(GameId).has_value());
    QVERIFY(QtCommon::BoxArt::Load(GameId).isNull());
}

void GamePicturesTest::AGameWithoutAnIdHasNoArt() {
    QVERIFY(!QtCommon::BoxArt::Save(0, Filled({105, 170}, Qt::red)));
    QVERIFY(!QtCommon::BoxArt::Find(0).has_value());
}

void GamePicturesTest::EditorSavesOnlyWhenAsked() {
    BoxArtEditor editor(GameId, NoFile, NoImage);
    QVERIFY(!editor.HasChanges());
    editor.SetPicture(Filled({105, 170}, Qt::red));
    QVERIFY(editor.HasChanges());
    QVERIFY(!QtCommon::BoxArt::Find(GameId).has_value());
    QVERIFY(editor.findChild<QPushButton*>(QStringLiteral("removeBoxArt"))->isEnabled());
    QVERIFY(editor.Save());
    QVERIFY(!editor.HasChanges());
    QVERIFY(QtCommon::BoxArt::Find(GameId).has_value());
}

void GamePicturesTest::EditorRemovesTheArt() {
    QVERIFY(QtCommon::BoxArt::Save(GameId, Filled({105, 170}, Qt::red)));
    BoxArtEditor editor(GameId, NoFile, NoImage);
    auto* const remove = editor.findChild<QPushButton*>(QStringLiteral("removeBoxArt"));
    QVERIFY(remove->isEnabled());
    remove->click();
    QVERIFY(editor.HasChanges());
    QVERIFY(!remove->isEnabled());
    QVERIFY(QtCommon::BoxArt::Find(GameId).has_value());
    QVERIFY(editor.Save());
    QVERIFY(!QtCommon::BoxArt::Find(GameId).has_value());
}

void GamePicturesTest::EditorOffersNothingWithoutAnId() {
    BoxArtEditor editor(0, NoFile, NoImage);
    QVERIFY(!editor.findChild<QPushButton*>(QStringLiteral("chooseBoxArt"))->isEnabled());
    QVERIFY(!editor.findChild<QPushButton*>(QStringLiteral("removeBoxArt"))->isEnabled());
}

void GamePicturesTest::PaintBox(QStandardItemModel& model, QImage& shelf, QRect& box) {
    GameCoverDelegate delegate(CoverShape::Box);
    const auto layout = Yuzu::ComputeBoxLayout(200);
    const QSize cell{layout.cell_width, layout.height + Yuzu::CoverGap};
    delegate.SetSizes({layout.width, layout.height}, cell);
    QStyleOptionViewItem option;
    option.rect = QRect({0, 0}, cell);
    shelf = QImage(cell, QImage::Format_ARGB32);
    shelf.fill(Qt::white);
    {
        QPainter painter(&shelf);
        delegate.paint(&painter, option, model.index(0, 0));
    }
    box = delegate.CoverRect(option.rect);
    QCOMPARE(box.size(), QSize(layout.width, layout.height));
}

void GamePicturesTest::FullCoverViewDrawsTheBoxArt() {
    QVERIFY(QtCommon::BoxArt::Save(GameId, Filled({105, 170}, Qt::red)));
    QStandardItemModel model;
    auto* const item = new QStandardItem;
    item->setData(static_cast<int>(GameListItemType::Game), GameListItem::TypeRole);
    item->setData(qulonglong(GameId), GameListItemPath::ProgramIdRole);
    item->setData(QStringLiteral("/games/ocarina.nup"), GameListItemPath::FullPathRole);
    model.appendRow(item);

    QImage shelf;
    QRect box;
    PaintBox(model, shelf, box);
    const QRgb middle = shelf.pixel(box.center());
    QVERIFY(qRed(middle) > 240 && qGreen(middle) < 30 && qBlue(middle) < 30);
}

void GamePicturesTest::FullCoverViewPutsTheIconOnACaseWithoutArt() {
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    QVERIFY(Filled({64, 64}, Qt::green).save(&buffer, "PNG"));

    QStandardItemModel model;
    auto* const item = new QStandardItem;
    item->setData(static_cast<int>(GameListItemType::Game), GameListItem::TypeRole);
    item->setData(qulonglong(GameId), GameListItemPath::ProgramIdRole);
    item->setData(QStringLiteral("/games/ocarina.nup"), GameListItemPath::FullPathRole);
    item->setData(png, GameListItemPath::CoverRole);
    model.appendRow(item);

    QImage shelf;
    QRect box;
    PaintBox(model, shelf, box);
    // The icon in the middle of the case...
    const QRgb middle = shelf.pixel(box.center());
    QVERIFY(qGreen(middle) > 240 && qRed(middle) < 30);
    // ...over a darker blur of itself that fills the case, rather than the white shelf.
    const QRgb top = shelf.pixel(box.center().x(), box.top() + 4);
    QVERIFY(qGreen(top) < 240 && qRed(top) < 30 && qBlue(top) < 30);
}

void GamePicturesTest::CustomIconIsStoredSquareAsPng() {
    QVERIFY(QtCommon::CustomIcon::Save(GameId, Filled({640, 480}, Qt::red)));
    QVERIFY(std::filesystem::exists(StoredFile(".png", Common::FS::EdenPath::CustomIconsDir)));
    const QImage icon = QtCommon::CustomIcon::Load(GameId);
    QCOMPARE(icon.size(), QSize(QtCommon::CustomIcon::StoredSide, QtCommon::CustomIcon::StoredSide));
    QVERIFY(QtCommon::CustomIcon::Remove(GameId));
    QVERIFY(!QtCommon::CustomIcon::Find(GameId).has_value());
    // Box art and icons are kept apart.
    QVERIFY(!QtCommon::BoxArt::Find(GameId).has_value());
}

void GamePicturesTest::CustomIconKeepsATallPicturesUpperPart() {
    // A square from rows 66 to 166 of 300: all red. A centred square would reach the blue.
    const QImage icon = QtCommon::CustomIcon::Squared(RedOverBlue());
    QCOMPARE(icon.size(), QSize(256, 256));
    QCOMPARE(qRed(icon.pixel(128, 250)), 255);
    QCOMPARE(qBlue(icon.pixel(128, 250)), 0);
}

void GamePicturesTest::CustomIconBytesAreWhatTheListLoads() {
    QVERIFY(QtCommon::CustomIcon::ReadBytes(GameId).empty());
    QVERIFY(QtCommon::CustomIcon::Save(GameId, Filled({256, 256}, Qt::green)));
    const std::vector<u8> bytes = QtCommon::CustomIcon::ReadBytes(GameId);
    QPixmap loaded;
    QVERIFY(loaded.loadFromData(bytes.data(), static_cast<uint>(bytes.size())));
    QCOMPARE(loaded.toImage().pixel(128, 128), qRgb(0, 255, 0));
}

void GamePicturesTest::IconEditorPreviewsSavesAndRestores() {
    CustomIconEditor editor(GameId, NoFile, NoImage);
    QSignalSpy changes(&editor, &CustomIconEditor::IconChanged);
    auto* const restore = editor.findChild<QPushButton*>(QStringLiteral("restoreGameIcon"));
    QVERIFY(editor.Icon().isNull());
    QVERIFY(!restore->isEnabled());

    editor.SetPicture(RedOverBlue());
    QCOMPARE(changes.count(), 1);
    // The preview is the icon as it will be stored.
    QCOMPARE(changes.last().at(0).value<QImage>().size(), QSize(256, 256));
    QVERIFY(restore->isEnabled());
    QVERIFY(!QtCommon::CustomIcon::Find(GameId).has_value());
    QVERIFY(editor.Save());
    QVERIFY(QtCommon::CustomIcon::Find(GameId).has_value());

    restore->click();
    QCOMPARE(changes.count(), 2);
    QVERIFY(changes.last().at(0).value<QImage>().isNull());
    QVERIFY(editor.HasChanges());
    QVERIFY(editor.Save());
    QVERIFY(!QtCommon::CustomIcon::Find(GameId).has_value());

    // A new editor shows the stored icon.
    QVERIFY(QtCommon::CustomIcon::Save(GameId, Filled({64, 64}, Qt::red)));
    CustomIconEditor reopened(GameId, NoFile, NoImage);
    QCOMPARE(reopened.Icon().size(), QSize(256, 256));
    QVERIFY(reopened.findChild<QPushButton*>(QStringLiteral("restoreGameIcon"))->isEnabled());
}

void GamePicturesTest::IconEditorOffersNothingWithoutAnId() {
    CustomIconEditor editor(0, NoFile, NoImage);
    QVERIFY(!editor.findChild<QPushButton*>(QStringLiteral("uploadCustomIcon"))->isEnabled());
    QVERIFY(!editor.findChild<QPushButton*>(QStringLiteral("restoreGameIcon"))->isEnabled());
}

QTEST_MAIN(GamePicturesTest)
#include "game_pictures.moc"

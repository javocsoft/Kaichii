// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>

#include <QBuffer>
#include <QPainter>
#include <QPushButton>
#include <QStandardItemModel>
#include <QStyleOptionViewItem>
#include <QTemporaryDir>
#include <QtTest>

#include "common/fs/path_util.h"
#include "qt_common/game_list/box_art.h"
#include "qt_common/game_list/game_list_p.h"
#include "yuzu/configuration/box_art_editor.h"
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

std::filesystem::path StoredFile(const char* extension) {
    return Common::FS::GetEdenPath(Common::FS::EdenPath::BoxArtDir) /
           (std::string("0400486024DEC000") + extension);
}

} // Anonymous namespace

class BoxArtTest : public QObject {
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

private:
    void PaintBox(QStandardItemModel& model, QImage& shelf, QRect& box);

    QTemporaryDir data_dir;
};

void BoxArtTest::init() {
    QVERIFY(data_dir.isValid());
    const std::filesystem::path dir =
        std::filesystem::path(data_dir.path().toStdString()) / "box_art";
    std::filesystem::remove_all(dir);
    // The path is only taken when it exists.
    std::filesystem::create_directories(dir);
    Common::FS::SetEdenPath(Common::FS::EdenPath::BoxArtDir, dir);
    QCOMPARE(Common::FS::GetEdenPath(Common::FS::EdenPath::BoxArtDir), dir);
}

void BoxArtTest::OpaqueArtIsStoredAsJpegAndScaledDown() {
    QVERIFY(QtCommon::BoxArt::Save(GameId, Filled({1050, 1700}, Qt::red)));
    QVERIFY(std::filesystem::exists(StoredFile(".jpg")));
    const QImage loaded = QtCommon::BoxArt::Load(GameId);
    QCOMPARE(loaded.height(), QtCommon::BoxArt::MaxStoredHeight);
    QCOMPARE(loaded.width(), 1050 * QtCommon::BoxArt::MaxStoredHeight / 1700);
}

void BoxArtTest::TransparentArtIsStoredAsPngReplacingTheOldOne() {
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

void BoxArtTest::ArtCopiedByHandIsFound() {
    std::filesystem::create_directories(StoredFile(".png").parent_path());
    QVERIFY(Filled({105, 170}, Qt::green)
                .save(QString::fromStdString(StoredFile(".jpeg").string()), "JPEG"));
    const auto found = QtCommon::BoxArt::Find(GameId);
    QVERIFY(found.has_value());
    QCOMPARE(QtCommon::BoxArt::Load(GameId).size(), QSize(105, 170));
}

void BoxArtTest::RemovingLeavesNoArt() {
    QVERIFY(QtCommon::BoxArt::Save(GameId, Filled({105, 170}, Qt::red)));
    QVERIFY(QtCommon::BoxArt::Remove(GameId));
    QVERIFY(!QtCommon::BoxArt::Find(GameId).has_value());
    QVERIFY(QtCommon::BoxArt::Load(GameId).isNull());
}

void BoxArtTest::AGameWithoutAnIdHasNoArt() {
    QVERIFY(!QtCommon::BoxArt::Save(0, Filled({105, 170}, Qt::red)));
    QVERIFY(!QtCommon::BoxArt::Find(0).has_value());
}

void BoxArtTest::EditorSavesOnlyWhenAsked() {
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

void BoxArtTest::EditorRemovesTheArt() {
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

void BoxArtTest::EditorOffersNothingWithoutAnId() {
    BoxArtEditor editor(0, NoFile, NoImage);
    QVERIFY(!editor.findChild<QPushButton*>(QStringLiteral("chooseBoxArt"))->isEnabled());
    QVERIFY(!editor.findChild<QPushButton*>(QStringLiteral("removeBoxArt"))->isEnabled());
}

void BoxArtTest::PaintBox(QStandardItemModel& model, QImage& shelf, QRect& box) {
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

void BoxArtTest::FullCoverViewDrawsTheBoxArt() {
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

void BoxArtTest::FullCoverViewPutsTheIconOnACaseWithoutArt() {
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

QTEST_MAIN(BoxArtTest)
#include "box_art.moc"

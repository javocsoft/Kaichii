// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "qt_common/game_list/placeholder_icon.h"

class GameListIconTest : public QObject {
    Q_OBJECT

private slots:
    void InitialsComeFromTheFirstTwoWords();
    void IconIsDrawnAtItsSize();
};

void GameListIconTest::InitialsComeFromTheFirstTwoWords() {
    using QtCommon::PlaceholderInitials;
    QCOMPARE(PlaceholderInitials(QStringLiteral("Donkey Kong Bananza")), QStringLiteral("DK"));
    QCOMPARE(PlaceholderInitials(QStringLiteral("the Legend of Zelda: Ocarina of Time")),
             QStringLiteral("TL"));
    QCOMPARE(PlaceholderInitials(QStringLiteral("Ōkami")), QStringLiteral("Ō"));
    QCOMPARE(PlaceholderInitials(QStringLiteral("  -  ")), QString{});
}

void GameListIconTest::IconIsDrawnAtItsSize() {
    const QImage icon = QtCommon::PlaceholderIcon(QStringLiteral("Donkey Kong Bananza"), 64);
    QCOMPARE(icon.size(), QSize(64, 64));
    // A filled square, transparent only at its rounded corners.
    QVERIFY(qAlpha(icon.pixel(32, 5)) == 255);
    QVERIFY(qAlpha(icon.pixel(0, 0)) < 255);
    // The same name gives the same color.
    const QImage again = QtCommon::PlaceholderIcon(QStringLiteral("Donkey Kong Bananza"), 64);
    QCOMPARE(icon.pixel(32, 5), again.pixel(32, 5));
}

QTEST_MAIN(GameListIconTest)
#include "game_list_icon.moc"

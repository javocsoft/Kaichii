// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <set>
#include <string>

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTextBrowser>
#include <QToolButton>
#include <QtTest/QtTest>

#include "yuzu/game_companion_catalog.h"
#include "yuzu/game_companion_widget.h"

class GameCompanionTest final : public QObject {
    Q_OBJECT

private slots:
    void onlyTheSwitch2RemakeIsSupported();
    void catalogHasSourcesAndNoDuplicateEntries();
    void searchMatchesAccentsAndHidesSpoilers();
    void widgetNeedsAnExplicitSpoilerChoice();
    void timelineShowsItsBranchesOnlyOnRequest();
    void appearanceCanBeReusedForAnotherGame();
    void loadedScenesSuggestCardsWithoutRevealingSpoilers();
    void cardStaysClosedUntilAskedAndCanBeClosed();
};

void GameCompanionTest::onlyTheSwitch2RemakeIsSupported() {
    QVERIFY(GameCompanion::SupportsGame(0x0400486024DEC000ULL));
    QVERIFY(!GameCompanion::SupportsGame(0x0100000000000000ULL));
    QVERIFY(!GameCompanion::SupportsGame(0));
}

void GameCompanionTest::catalogHasSourcesAndNoDuplicateEntries() {
    std::set<std::string> ids;
    int remake = 0;
    int spoiler = 0;
    int enemies = 0;
    int bosses = 0;
    int places = 0;
    int dungeons = 0;
    int characters = 0;
    int items = 0;
    int timeline = 0;
    for (const auto& entry : GameCompanion::OcarinaEntries()) {
        QVERIFY(ids.insert(std::string{entry.id}).second);
        QVERIFY(!entry.title.en.empty());
        QVERIFY(!entry.title.es.empty());
        QVERIFY(!entry.summary.en.empty());
        QVERIFY(!entry.summary.es.empty());
        QVERIFY(entry.source_url.starts_with("https://"));
        if (entry.kind == GameCompanion::Kind::Remake) {
            ++remake;
        }
        if (entry.spoiler) {
            ++spoiler;
        }
        switch (entry.kind) {
        case GameCompanion::Kind::Enemy:
            ++enemies;
            break;
        case GameCompanion::Kind::Boss:
            ++bosses;
            break;
        case GameCompanion::Kind::Place:
            ++places;
            break;
        case GameCompanion::Kind::Dungeon:
            ++dungeons;
            break;
        case GameCompanion::Kind::Character:
            ++characters;
            break;
        case GameCompanion::Kind::Item:
            ++items;
            break;
        case GameCompanion::Kind::Timeline:
            ++timeline;
            break;
        case GameCompanion::Kind::All:
        case GameCompanion::Kind::Remake:
            break;
        }
    }
    QVERIFY(ids.size() >= 150);
    QVERIFY(remake >= 4);
    QVERIFY(spoiler >= 4);
    QVERIFY(enemies >= 60);
    QVERIFY(bosses >= 15);
    QVERIFY(enemies + bosses >= 78);
    QVERIFY(places >= 20);
    QVERIFY(dungeons >= 13);
    QVERIFY(characters >= 20);
    QVERIFY(items >= 20);
    QCOMPARE(timeline, 2);
    for (const char* id : {"deku_scrub", "skulltula", "stalfos", "false_door", "flying_tile",
                           "gohma_egg", "iron_knuckle", "dark_link", "gohma", "king_dodongo",
                           "barinade", "phantom_ganon", "volvagia", "morpha", "bongo_bongo",
                           "twinrova", "ganondorf_boss", "ganon_boss", "timeline_position",
                           "timeline_split"}) {
        QVERIFY2(GameCompanion::FindEntry(id), id);
    }
}

void GameCompanionTest::searchMatchesAccentsAndHidesSpoilers() {
    using GameCompanion::Kind;
    auto results = GameCompanion::Search(QStringLiteral("arbol"), Kind::Character, false);
    QVERIFY(std::ranges::any_of(results, [](const auto* entry) {
        return entry->id == "great_deku_tree";
    }));

    results = GameCompanion::Search(QStringLiteral("rey Zora"), Kind::Place, false);
    QVERIFY(std::ranges::any_of(results, [](const auto* entry) {
        return entry->id == "zora_domain";
    }));

    results = GameCompanion::Search(QStringLiteral("seis sabios"), Kind::All, false);
    QVERIFY(results.empty());
    results = GameCompanion::Search(QStringLiteral("seis sabios"), Kind::All, true);
    QCOMPARE(results.size(), 1U);
    QVERIFY(results.front()->id == "sages_remake");

    results = GameCompanion::Search(QStringLiteral("Navi"), Kind::Place, true);
    QVERIFY(!results.empty());
    QVERIFY(std::ranges::all_of(results, [](const auto* entry) {
        return entry->kind == Kind::Place;
    }));
    QVERIFY(std::ranges::none_of(results, [](const auto* entry) {
        return entry->id == "navi";
    }));

    results = GameCompanion::Search(QStringLiteral("cronologia"), Kind::Timeline, false);
    QVERIFY(std::ranges::any_of(results, [](const auto* entry) {
        return entry->id == "timeline_position";
    }));
    QVERIFY(std::ranges::none_of(results, [](const auto* entry) {
        return entry->id == "timeline_split";
    }));
    results = GameCompanion::Search(QStringLiteral("ramas temporales"), Kind::Timeline, true);
    QVERIFY(std::ranges::any_of(results, [](const auto* entry) {
        return entry->id == "timeline_split";
    }));
}

void GameCompanionTest::widgetNeedsAnExplicitSpoilerChoice() {
    GameCompanionWidget widget{true};
    auto* const search = widget.findChild<QLineEdit*>(QStringLiteral("companionSearch"));
    auto* const spoilers = widget.findChild<QCheckBox*>(QStringLiteral("companionSpoilers"));
    auto* const results = widget.findChild<QListWidget*>(QStringLiteral("companionResults"));
    auto* const detail = widget.findChild<QTextBrowser*>(QStringLiteral("companionDetail"));
    QVERIFY(search && spoilers && results && detail);
    QVERIFY(!spoilers->isChecked());
    widget.SelectEntry(QStringLiteral("great_deku_tree"));
    QVERIFY(detail->toPlainText().contains(QStringLiteral("Gran Árbol Deku")));

    search->setText(QStringLiteral("sabios"));
    QCOMPARE(results->count(), 0);
    QVERIFY(!detail->toPlainText().contains(QStringLiteral("Rauru")));
    spoilers->setChecked(true);
    QVERIFY(results->count() > 0);
    widget.SelectEntry(QStringLiteral("sages_remake"));
    QCOMPARE(widget.CurrentEntryId(), QStringLiteral("sages_remake"));
    QVERIFY(detail->toPlainText().contains(QStringLiteral("Rauru")));
    widget.HideSpoilers();
    QVERIFY(!spoilers->isChecked());
    QVERIFY(widget.CurrentEntryId().isEmpty());
    QVERIFY(!detail->toPlainText().contains(QStringLiteral("Rauru")));
}

void GameCompanionTest::timelineShowsItsBranchesOnlyOnRequest() {
    GameCompanionWidget widget{true};
    auto* const spoilers = widget.findChild<QCheckBox*>(QStringLiteral("companionSpoilers"));
    auto* const detail = widget.findChild<QTextBrowser*>(QStringLiteral("companionDetail"));
    QVERIFY(spoilers && detail);
    widget.SelectEntry(QStringLiteral("timeline_position"));
    QCOMPARE(widget.CurrentEntryId(), QStringLiteral("timeline_position"));
    QVERIFY(detail->toPlainText().contains(QStringLiteral("Skyward Sword")));

    spoilers->setChecked(true);
    widget.SelectEntry(QStringLiteral("timeline_split"));
    QCOMPARE(widget.CurrentEntryId(), QStringLiteral("timeline_split"));
    QVERIFY(detail->toPlainText().contains(QStringLiteral("Majora's Mask")));
    QVERIFY(detail->toPlainText().contains(QStringLiteral("The Wind Waker")));
    QVERIFY(detail->toPlainText().contains(QStringLiteral("A Link to the Past")));

    widget.HideSpoilers();
    QVERIFY(widget.CurrentEntryId() != QStringLiteral("timeline_split"));
    QVERIFY(!detail->toPlainText().contains(QStringLiteral("Majora's Mask")));
}

void GameCompanionTest::appearanceCanBeReusedForAnotherGame() {
    auto appearance = GameCompanionAppearance::Ocarina();
    appearance.title = {"Another game", "Otro juego"};
    appearance.eyebrow = {"Field guide", "Guía de juego"};
    appearance.subtitle = {"A different world", "Otro mundo"};
    appearance.search_hint = {"Search elsewhere", "Buscar en otro mundo"};
    appearance.motif = GameCompanionAppearance::Motif::None;
    GameCompanionWidget widget{true, nullptr, appearance};
    auto* const hero = widget.findChild<QWidget*>(QStringLiteral("companionHero"));
    QVERIFY(hero);
    QCOMPARE(hero->accessibleName(), QStringLiteral("Otro juego — Otro mundo"));
    auto* const search = widget.findChild<QLineEdit*>(QStringLiteral("companionSearch"));
    QVERIFY(search);
    QCOMPARE(search->placeholderText(), QStringLiteral("Buscar en otro mundo"));
    widget.SetLanguage(false);
    QCOMPARE(hero->accessibleName(), QStringLiteral("Another game — A different world"));
    QCOMPARE(search->placeholderText(), QStringLiteral("Search elsewhere"));
}

void GameCompanionTest::loadedScenesSuggestCardsWithoutRevealingSpoilers() {
    QVERIFY(GameCompanion::EntryForScene("KokiriForest_Child"));
    QVERIFY(GameCompanion::EntryForScene("KokiriForest_Child")->id == "kokiri_forest");
    QVERIFY(GameCompanion::EntryForScene("KakarikoVillage_Adult_Alternative")->id == "kakariko");
    QVERIFY(GameCompanion::EntryForScene("GohmaStage")->id == "gohma");
    QVERIFY(GameCompanion::EntryForScene("UnknownStage") == nullptr);

    GameCompanionWidget widget{true};
    auto* const panel = widget.findChild<QWidget*>(QStringLiteral("companionContext"));
    auto* const label = widget.findChild<QLabel*>(QStringLiteral("companionContextLabel"));
    auto* const open = widget.findChild<QPushButton*>(QStringLiteral("companionContextOpen"));
    auto* const spoilers = widget.findChild<QCheckBox*>(QStringLiteral("companionSpoilers"));
    QVERIFY(panel && label && open && spoilers);
    QVERIFY(panel->isHidden());

    widget.SetDetectedScene(QStringLiteral("KokiriForest_Child"));
    QVERIFY(!panel->isHidden());
    QVERIFY(label->text().contains(QStringLiteral("Bosque Kokiri")));
    open->click();
    QCOMPARE(widget.CurrentEntryId(), QStringLiteral("kokiri_forest"));
    QCOMPARE(open->text(), QStringLiteral("Opened"));
    QVERIFY(!open->isEnabled());
    QTRY_VERIFY(panel->isHidden());

    widget.SetDetectedScene(QStringLiteral("LostWood_Child"));
    QVERIFY(!panel->isHidden());
    QVERIFY(label->text().contains(QStringLiteral("Bosque Perdido")));
    open->click();
    QCOMPARE(widget.CurrentEntryId(), QStringLiteral("lost_woods"));
    QTRY_VERIFY(panel->isHidden());
    widget.SetDetectedScene(QStringLiteral("KokiriForest_Child"));
    QVERIFY(!panel->isHidden());
    QVERIFY(label->text().contains(QStringLiteral("Bosque Kokiri")));

    open->click();
    widget.SetDetectedScene(QStringLiteral("LostWood_Child"));
    QTest::qWait(300);
    QVERIFY(!panel->isHidden());
    QVERIFY(label->text().contains(QStringLiteral("Bosque Perdido")));

    widget.SetDetectedScene(QStringLiteral("GohmaStage"));
    QVERIFY(panel->isHidden());
    QVERIFY(!label->text().contains(QStringLiteral("Gohma")));
    open->click();
    QCOMPARE(widget.CurrentEntryId(), QStringLiteral("kokiri_forest"));
    spoilers->setChecked(true);
    QVERIFY(!panel->isHidden());
    QVERIFY(label->text().contains(QStringLiteral("Reina Gohma")));
    widget.HideSpoilers();
    QVERIFY(panel->isHidden());
    widget.SetDetectedScene({});
    QVERIFY(panel->isHidden());
}

void GameCompanionTest::cardStaysClosedUntilAskedAndCanBeClosed() {
    GameCompanionWidget widget{false};
    auto* const search = widget.findChild<QLineEdit*>(QStringLiteral("companionSearch"));
    auto* const results = widget.findChild<QListWidget*>(QStringLiteral("companionResults"));
    auto* const heading = widget.findChild<QLabel*>(QStringLiteral("companionResultsHeading"));
    auto* const panel = widget.findChild<QWidget*>(QStringLiteral("companionDetailPanel"));
    auto* const detail = widget.findChild<QTextBrowser*>(QStringLiteral("companionDetail"));
    auto* const close = widget.findChild<QToolButton*>(QStringLiteral("companionDetailClose"));
    QVERIFY(search && results && heading && panel && detail && close);
    QVERIFY(results->count() > 0);
    QVERIFY(widget.CurrentEntryId().isEmpty());
    QVERIFY(panel->isHidden());

    widget.SelectEntry(QStringLiteral("hyrule_field"));
    QVERIFY(!panel->isHidden());
    QVERIFY(detail->toPlainText().contains(QStringLiteral("Hyrule Field")));
    QCOMPARE(close->toolTip(), QStringLiteral("Close card"));

    search->setText(QStringLiteral("hyrule"));
    QCOMPARE(widget.CurrentEntryId(), QStringLiteral("hyrule_field"));
    QVERIFY(!panel->isHidden());
    close->click();
    QVERIFY(widget.CurrentEntryId().isEmpty());
    QVERIFY(results->selectedItems().isEmpty());
    QVERIFY(panel->isHidden());
    QVERIFY(detail->toPlainText().isEmpty());

    search->setText(QStringLiteral("zzzz no such topic"));
    QCOMPARE(results->count(), 0);
    QVERIFY(heading->text().startsWith(QStringLiteral("No matching topic")));
    QVERIFY(panel->isHidden());
    search->clear();
    QVERIFY(widget.CurrentEntryId().isEmpty());
    QVERIFY(panel->isHidden());
}

QTEST_MAIN(GameCompanionTest)
#include "game_companion.moc"

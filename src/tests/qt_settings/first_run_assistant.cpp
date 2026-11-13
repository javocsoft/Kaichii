// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>

#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QtTest>

#include "yuzu/first_run_assistant.h"

namespace {

Settings::HostProfile EntryMachine() {
    return {.gpu_name = "Test GPU",
            .gpu_vendor_id = 0x10DE,
            .gpu_memory_bytes = u64{4} << 30,
            .cpu_threads = 20,
            .ram_bytes = u64{32} << 30};
}

/// What the hooks were asked, and what they answer.
struct FakeWindow {
    QStringList folders;
    QString next_choice;
    bool keys = false;
    int keys_installs = 0;
    int controls_opened = 0;

    FirstRunAssistant::Hooks Hooks() {
        return {
            .choose_folder = [this] { return next_choice; },
            .add_game_folder = [this](const QString& folder) { folders << folder; },
            .game_folders = [this] { return folders; },
            .keys_present = [this] { return keys; },
            .install_keys =
                [this] {
                    ++keys_installs;
                    keys = true;
                },
            .configure_controls = [this] { ++controls_opened; },
        };
    }
};

template <typename T>
T* FindByText(QWidget& root, const QString& text) {
    const auto children = root.findChildren<T*>();
    const auto it = std::ranges::find_if(
        children, [&](const T* child) { return child->text().startsWith(text); });
    return it != children.end() ? *it : nullptr;
}

} // Anonymous namespace

class FirstRunAssistantTest : public QObject {
    Q_OBJECT

private slots:
    void RecommendsThePresetForThisComputer();
    void KeepsTheSettingsWithoutAGraphicsCard();
    void AddsFoldersAndKeysThroughTheWindow();
    void AsksForTheConsoleMode();
};

void FirstRunAssistantTest::RecommendsThePresetForThisComputer() {
    FakeWindow window;
    FirstRunAssistant assistant(nullptr, EntryMachine(), {}, false, window.Hooks());
    QCOMPARE(assistant.ChosenPreset(), Settings::PerformancePreset::Performance);
    QVERIFY(FindByText<QRadioButton>(assistant, QStringLiteral("Performance (recommended")));

    FindByText<QRadioButton>(assistant, QStringLiteral("Quality"))->setChecked(true);
    QCOMPARE(assistant.ChosenPreset(), Settings::PerformancePreset::Quality);
    FindByText<QRadioButton>(assistant, QStringLiteral("Keep my current settings"))
        ->setChecked(true);
    QCOMPARE(assistant.ChosenPreset(), std::nullopt);
}

void FirstRunAssistantTest::KeepsTheSettingsWithoutAGraphicsCard() {
    FakeWindow window;
    FirstRunAssistant assistant(nullptr, std::nullopt, {}, false, window.Hooks());
    QCOMPARE(assistant.ChosenPreset(), std::nullopt);
    QVERIFY(FindByText<QRadioButton>(assistant, QStringLiteral("Quality")) == nullptr);
}

void FirstRunAssistantTest::AddsFoldersAndKeysThroughTheWindow() {
    FakeWindow window;
    window.next_choice = QStringLiteral("/games/switch2");
    FirstRunAssistant assistant(nullptr, EntryMachine(), {}, false, window.Hooks());
    auto* const list = assistant.findChild<QListWidget*>();
    QCOMPARE(list->count(), 1); // "No folder yet."
    QCOMPARE(list->item(0)->flags(), Qt::NoItemFlags);

    FindByText<QPushButton>(assistant, QStringLiteral("Add a games folder"))->click();
    QCOMPARE(window.folders, QStringList{QStringLiteral("/games/switch2")});
    QCOMPARE(list->count(), 1);
    QCOMPARE(list->item(0)->text(), QStringLiteral("/games/switch2"));

    // A cancelled choice adds nothing.
    window.next_choice.clear();
    FindByText<QPushButton>(assistant, QStringLiteral("Add a games folder"))->click();
    QCOMPARE(window.folders.size(), 1);

    // A wizard hides its pages until it is shown: visible to its own page.
    auto* const keys = FindByText<QPushButton>(assistant, QStringLiteral("Install decryption"));
    QVERIFY(keys->isVisibleTo(keys->parentWidget()));
    keys->click();
    QCOMPARE(window.keys_installs, 1);
    QVERIFY(!keys->isVisibleTo(keys->parentWidget()));
}

void FirstRunAssistantTest::AsksForTheConsoleMode() {
    FakeWindow window;
    FirstRunAssistant assistant(nullptr, EntryMachine(), {QStringLiteral("Pro Controller")}, true,
                                window.Hooks());
    QVERIFY(assistant.Docked());
    FindByText<QRadioButton>(assistant, QStringLiteral("Handheld"))->setChecked(true);
    QVERIFY(!assistant.Docked());
    const auto labels = assistant.findChildren<QLabel*>();
    QVERIFY(std::ranges::any_of(labels, [](const QLabel* label) {
        return label->text().contains(QStringLiteral("Pro Controller"));
    }));
    FindByText<QPushButton>(assistant, QStringLiteral("Configure controls"))->click();
    QCOMPARE(window.controls_opened, 1);
}

QTEST_MAIN(FirstRunAssistantTest)
#include "first_run_assistant.moc"

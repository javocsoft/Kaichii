// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <functional>
#include <optional>
#include <vector>

#include <QStringList>
#include <QWizard>

#include "common/performance_presets.h"

class QButtonGroup;
class QLabel;
class QListWidget;
class QPushButton;
class QRadioButton;

/// Three steps for a new installation: where the games are, the preset for this computer, and
/// the console mode and controllers. Shown once on a first start, and from the Help menu.
class FirstRunAssistant final : public QWizard {
    Q_OBJECT

public:
    /// What the assistant asks the main window to do.
    struct Hooks {
        /// Asks for a folder; empty when the player cancels.
        std::function<QString()> choose_folder;
        /// Adds a game folder to the list and scans it.
        std::function<void(const QString&)> add_game_folder;
        /// The folders the game list scans, as they are now.
        std::function<QStringList()> game_folders;
        std::function<bool()> keys_present;
        std::function<void()> install_keys;
        std::function<void()> configure_controls;
    };

    /// `host` is empty when the graphics card couldn't be read. `controllers` are the names of the
    /// input devices found now.
    FirstRunAssistant(QWidget* parent, std::optional<Settings::HostProfile> host,
                      QStringList controllers, bool docked, Hooks hooks);
    ~FirstRunAssistant() override;

    /// The preset chosen, or empty to keep the current settings.
    [[nodiscard]] std::optional<Settings::PerformancePreset> ChosenPreset() const;
    [[nodiscard]] bool Docked() const;

private:
    QWizardPage* GamesPage();
    QWizardPage* ComputerPage();
    QWizardPage* ControlsPage(const QStringList& controllers, bool docked);
    void UpdateGames();

    std::optional<Settings::HostProfile> host;
    Hooks hooks;

    QListWidget* folders = nullptr;
    QLabel* keys_label = nullptr;
    QPushButton* keys_button = nullptr;
    QButtonGroup* presets = nullptr;
    QRadioButton* docked_radio = nullptr;
};

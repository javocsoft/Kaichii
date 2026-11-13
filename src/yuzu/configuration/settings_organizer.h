// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <optional>
#include <span>
#include <vector>

#include <QObject>
#include <QPointer>
#include <QString>

#include "qt_common/config/setting_help.h"

class QGroupBox;
class QWidget;

namespace ConfigurationShared {

/// The row of a setting under `root`: the outermost widget that carries its id (a settings
/// widget, or a group a page wrapped it in); null when there is none.
[[nodiscard]] QWidget* FindSettingRow(QWidget* root, u32 id);

/// One section of a page built from other pages' rows.
struct PageSection {
    const char* title = nullptr; ///< Untranslated, in the ConfigurationShared context
    std::vector<u32> ids;
    /// A group the source page built (with its own logic) used as the whole section.
    QWidget* replacement = nullptr;
    /// Rows that are not settings, after the settings.
    std::vector<QWidget*> extra_rows;
};

/// Arranges the settings dialogs by what options are for, and by who they are for: rows above the
/// basic level show only with advanced options on (developer ones also need developer mode), and
/// options for one game show only in that game's configuration.
///
/// Rows keep their own widgets, signals and apply functions: they are only moved and wrapped, so
/// the pages that built them keep showing and hiding them as before.
class SettingsOrganizer : public QObject {
    Q_OBJECT

public:
    /// `program_id` is the game the dialog configures, or 0 for the global settings.
    SettingsOrganizer(const SettingHelpMap& help, u64 program_id, QObject* parent = nullptr);
    ~SettingsOrganizer() override;

    /// A page of `sections`, with the rows of their settings moved out of `sources`. Sections that
    /// end up empty are left out.
    QWidget* BuildPage(std::span<QWidget* const> sources, std::vector<PageSection> sections);

    /// The Image, Smoothness and GPU emulation pages, from the graphics pages' rows (`sources`) in
    /// the order GraphicsSections gives, with the two groups the Graphics page builds besides rows.
    std::array<QWidget*, 3> BuildGraphicsPages(std::span<QWidget* const> sources,
                                               QWidget* graphics_card_group,
                                               QWidget* background_row);

    /// Puts every setting row under `page` behind the visibility of its level.
    void Gate(QWidget* page);

    void SetShowAdvanced(bool show);
    void SetDeveloperMode(bool on);
    [[nodiscard]] bool ShowAdvanced() const {
        return show_advanced;
    }
    [[nodiscard]] bool DeveloperMode() const {
        return developer_mode;
    }

    /// Whether options of this level show now.
    [[nodiscard]] bool Shows(SettingLevel level) const;

    /// The level `row` (a setting row) shows at; empty when it never shows in this dialog because
    /// it is for another game. Basic for widgets that are not setting rows.
    [[nodiscard]] std::optional<SettingLevel> LevelOf(QWidget* row) const;

    /// The section titles in the current language.
    void Retranslate();

protected:
    /// Follows rows their pages show and hide, so a hidden row's wrapper takes no room.
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    struct Gated {
        QPointer<QWidget> gate;
        QPointer<QWidget> row;
        std::optional<SettingLevel> level; ///< Empty: another game's, never shown
    };
    struct Section {
        QPointer<QGroupBox> box;
        const char* title;
        std::vector<QPointer<QWidget>> rows;
    };

    void Update();
    void UpdateGate(const Gated& gated) const;

    const SettingHelpMap& help;
    const u64 program_id;
    bool show_advanced = false;
    bool developer_mode = false;
    std::vector<Gated> gates;
    std::vector<Section> sections;
};

} // namespace ConfigurationShared

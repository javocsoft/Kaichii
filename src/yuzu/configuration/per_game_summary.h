// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <vector>

#include <QPointer>
#include <QWidget>

#include "common/performance_presets.h"
#include "qt_common/config/setting_help.h"

class QLabel;
class QVBoxLayout;

namespace ConfigurationShared {

/// What a row shows now: its list or field's text, a slider's value label, or on/off for a switch.
/// Falls back to the setting's value when the row has none of those.
[[nodiscard]] QString DisplayedValue(QWidget* row, const Settings::BasicSetting& setting,
                                     const ComboboxTranslationMap& enums);

/// The first tab of a game's configuration: the preset its settings match, and each setting the
/// game changes from the global one, with its value, the global value, a link to it and a button
/// that goes back to the global value. Changes not applied yet count: it reads the pages' rows.
class PerGameSummary : public QWidget {
    Q_OBJECT

public:
    PerGameSummary(const TranslationMap& names, const ComboboxTranslationMap& enums,
                   std::optional<Settings::HostProfile> host, QWidget* parent = nullptr);
    ~PerGameSummary() override;

    /// The pages whose settings it sums up.
    void SetPages(std::vector<QWidget*> pages);

    /// Reads the pages again.
    void Refresh();

    /// The settings it lists now, by id, in page order.
    [[nodiscard]] const std::vector<u32>& Listed() const {
        return listed;
    }

signals:
    /// The link of a setting was followed: show `row`.
    void GoToSetting(QWidget* row);

protected:
    void changeEvent(QEvent* event) override;

private:
    const TranslationMap& names;
    const ComboboxTranslationMap& enums;
    std::optional<Settings::HostProfile> host;
    std::vector<QPointer<QWidget>> pages;
    std::vector<u32> listed;

    QLabel* title = nullptr;
    QLabel* preset = nullptr;
    QLabel* empty = nullptr;
    QWidget* list = nullptr;
    QVBoxLayout* list_layout = nullptr;
};

} // namespace ConfigurationShared

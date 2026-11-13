// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>

#include <QFrame>
#include <QPointer>

#include "common/performance_presets.h"
#include "qt_common/config/setting_help.h"

class QLabel;
class QScrollArea;

namespace ConfigurationShared {

/// The property a settings widget carries with its setting's id, for the help panel.
inline constexpr char SettingIdProperty[] = "lichiiSettingId";

/// A box under the settings pages that explains the option under the pointer or with the
/// keyboard focus: what it does, what it costs, when it applies and the recommended value. Other
/// controls show their tooltip there.
class SettingHelpPanel : public QFrame {
    Q_OBJECT

public:
    SettingHelpPanel(const TranslationMap& names, const SettingHelpMap& help,
                     const ComboboxTranslationMap& enums,
                     std::optional<Settings::HostProfile> host, QWidget* parent = nullptr);
    ~SettingHelpPanel() override;

    /// Follows the pointer and the focus among the controls inside `root`.
    void Watch(QWidget* root);

    /// Explains `widget`, or the nearest of its parents inside the watched page that is a setting
    /// or has a tooltip. Returns whether anything was shown.
    bool Explain(QWidget* widget);

    /// The panel's text, as rich text.
    QString Text() const;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    void ShowPlaceholder();

    const TranslationMap& names;
    const SettingHelpMap& help;
    const ComboboxTranslationMap& enums;
    std::optional<Settings::HostProfile> host;
    QPointer<QWidget> root;
    QPointer<QWidget> shown;
    QScrollArea* scroll = nullptr;
    QLabel* label = nullptr;
};

} // namespace ConfigurationShared

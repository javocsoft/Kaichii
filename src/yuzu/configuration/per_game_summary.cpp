// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/configuration/per_game_summary.h"

#include <algorithm>

#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

#include "common/settings.h"
#include "yuzu/configuration/settings_organizer.h"

namespace ConfigurationShared {

namespace {

QString Translate(const char* text) {
    return QCoreApplication::translate("ConfigurationShared", text);
}

} // Anonymous namespace

QString DisplayedValue(QWidget* row, const Settings::BasicSetting& setting,
                       const ComboboxTranslationMap& enums) {
    if (row != nullptr) {
        if (const auto* box = row->findChild<QComboBox*>()) {
            return box->currentText();
        }
        // Spin boxes and date edits; their text includes the suffix (%).
        if (const auto* spin = row->findChild<QAbstractSpinBox*>()) {
            return spin->text();
        }
        if (const auto* edit = row->findChild<QLineEdit*>()) {
            return edit->text();
        }
        if (row->findChild<QSlider*>() != nullptr) {
            // A slider shows its value in the row's last label (the first is its name).
            if (const auto labels = row->findChildren<QLabel*>(); !labels.isEmpty()) {
                return labels.back()->text();
            }
        }
        if (const auto* check = row->findChild<QCheckBox*>()) {
            return FormatSettingValue(setting, check->isChecked() ? "true" : "false", enums);
        }
    }
    return FormatSettingValue(setting, setting.ToString(), enums);
}

PerGameSummary::PerGameSummary(const TranslationMap& names_, const ComboboxTranslationMap& enums_,
                               std::optional<Settings::HostProfile> host_, QWidget* parent)
    : QWidget(parent), names{names_}, enums{enums_}, host{std::move(host_)} {
    auto* const layout = new QVBoxLayout(this);
    title = new QLabel(this);
    QFont bold = title->font();
    bold.setBold(true);
    title->setFont(bold);
    preset = new QLabel(this);
    preset->setWordWrap(true);
    list = new QWidget(this);
    list_layout = new QVBoxLayout(list);
    list_layout->setContentsMargins(0, 0, 0, 0);
    empty = new QLabel(this);
    empty->setWordWrap(true);
    layout->addWidget(title);
    layout->addWidget(preset);
    layout->addWidget(list);
    layout->addWidget(empty);
    layout->addStretch(1);
    Refresh();
}

PerGameSummary::~PerGameSummary() = default;

void PerGameSummary::SetPages(std::vector<QWidget*> pages_) {
    pages.assign(pages_.begin(), pages_.end());
    Refresh();
}

void PerGameSummary::Refresh() {
    const auto& tr = Translate;
    title->setText(tr("What this game changes from the global settings"));
    empty->setText(tr("Nothing: this game uses the global settings in every tab."));
    if (host) {
        const auto match = Settings::MatchingPerformancePreset(Settings::values, *host);
        preset->setText(match
                            ? tr("Its saved settings match the %1 preset.").arg(PresetName(*match))
                            : tr("Its saved settings match no preset."));
    }
    preset->setVisible(host.has_value());

    // Rebuilt after the click that asked for it has returned: a button can't delete itself.
    while (QLayoutItem* const item = list_layout->takeAt(0)) {
        if (QWidget* const old = item->widget()) {
            old->hide();
            old->deleteLater();
        }
        delete item;
    }
    listed.clear();

    // The settings whose "use the global value" button is on, in the order of the pages.
    struct Change {
        u32 id;
        QWidget* page;
        std::vector<QPointer<QPushButton>> buttons;
    };
    std::vector<Change> changes;
    for (const QPointer<QWidget>& page : pages) {
        if (!page) {
            continue;
        }
        for (QPushButton* const button : page->findChildren<QPushButton*>()) {
            // Shown and enabled: some pages keep it enabled and only hide it (VSync's).
            const QVariant id = button->property(RestoresSettingProperty);
            if (!id.isValid() || !button->isEnabled() ||
                !button->isVisibleTo(button->parentWidget())) {
                continue;
            }
            const auto it = std::ranges::find(changes, id.toUInt(), &Change::id);
            if (it != changes.end()) {
                it->buttons.emplace_back(button);
            } else {
                changes.push_back({id.toUInt(), page, {button}});
            }
        }
    }

    for (const Change& change : changes) {
        const Settings::BasicSetting* const setting = FindSetting(change.id);
        const auto name_it = names.find(change.id);
        if (setting == nullptr || name_it == names.end() || name_it->second.first.isEmpty()) {
            continue;
        }
        QString name = name_it->second.first.trimmed();
        if (name.endsWith(QLatin1Char(':'))) {
            name.chop(1);
        }
        QWidget* const row = FindSettingRow(change.page, change.id);

        auto* const line = new QWidget(list);
        auto* const line_layout = new QHBoxLayout(line);
        line_layout->setContentsMargins(0, 0, 0, 0);
        auto* const text = new QLabel(line);
        text->setTextFormat(Qt::RichText);
        text->setWordWrap(true);
        text->setText(
            tr("%1: <b>%2</b> &nbsp;<i>(global: %3)</i>")
                .arg(QStringLiteral("<a href=\"setting\">%1</a>").arg(name.toHtmlEscaped()),
                     DisplayedValue(row, *setting, enums).toHtmlEscaped(),
                     FormatSettingValue(*setting, setting->ToStringGlobal(), enums)
                         .toHtmlEscaped()));
        connect(text, &QLabel::linkActivated, this, [this, target = QPointer<QWidget>(row)] {
            if (target) {
                emit GoToSetting(target);
            }
        });
        auto* const use_global = new QPushButton(tr("Use global"), line);
        connect(use_global, &QPushButton::clicked, this, [this, buttons = change.buttons] {
            for (const QPointer<QPushButton>& button : buttons) {
                if (button && button->isEnabled() && button->isVisibleTo(button->parentWidget())) {
                    button->click();
                }
            }
            QMetaObject::invokeMethod(this, &PerGameSummary::Refresh, Qt::QueuedConnection);
        });
        line_layout->addWidget(text, 1);
        line_layout->addWidget(use_global);
        list_layout->addWidget(line);
        listed.push_back(change.id);
    }
    empty->setVisible(listed.empty());
}

void PerGameSummary::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        Refresh();
    }
    QWidget::changeEvent(event);
}

} // namespace ConfigurationShared

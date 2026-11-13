// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/configuration/setting_help_panel.h"

#include <QAbstractButton>
#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QGroupBox>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>

#include "common/settings_common.h"

namespace ConfigurationShared {

namespace {

/// Lines of text the panel shows before it scrolls.
constexpr int VisibleLines = 8;

QString ControlName(const QWidget& widget) {
    if (const auto* button = qobject_cast<const QAbstractButton*>(&widget)) {
        return button->text();
    }
    if (const auto* group = qobject_cast<const QGroupBox*>(&widget)) {
        return group->title();
    }
    if (const auto* text = qobject_cast<const QLabel*>(&widget)) {
        return text->text();
    }
    return widget.accessibleName();
}

} // Anonymous namespace

SettingHelpPanel::SettingHelpPanel(const TranslationMap& names_, const SettingHelpMap& help_,
                                   const ComboboxTranslationMap& enums_,
                                   std::optional<Settings::HostProfile> host_, QWidget* parent)
    : QFrame(parent), names{names_}, help{help_}, enums{enums_}, host{std::move(host_)} {
    setObjectName(QStringLiteral("SettingHelpPanel"));
    setFrameShape(QFrame::StyledPanel);
    auto* const layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    label = new QLabel(scroll);
    label->setTextFormat(Qt::RichText);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    label->setMargin(6);
    scroll->setWidget(label);
    layout->addWidget(scroll);
    // A fixed height, so the pages above don't move as the text changes.
    setFixedHeight(fontMetrics().lineSpacing() * VisibleLines +
                   2 * (label->margin() + frameWidth()));
    ShowPlaceholder();
}

SettingHelpPanel::~SettingHelpPanel() {
    if (qApp != nullptr) {
        qApp->removeEventFilter(this);
    }
}

void SettingHelpPanel::Watch(QWidget* root_) {
    root = root_;
    qApp->installEventFilter(this);
}

bool SettingHelpPanel::Explain(QWidget* widget) {
    for (QWidget* current = widget; current != nullptr; current = current->parentWidget()) {
        const QVariant id = current->property(SettingIdProperty);
        if (id.isValid()) {
            if (const Settings::BasicSetting* setting = FindSetting(id.toUInt())) {
                const SettingHelpContext context{
                    .names = names, .help = help, .enums = enums, .host = host ? &*host : nullptr};
                label->setText(SettingHelpHtml(*setting, context));
                scroll->verticalScrollBar()->setValue(0);
                shown = current;
                return true;
            }
        }
        if (!current->toolTip().isEmpty()) {
            label->setText(ControlHelpHtml(ControlName(*current), current->toolTip()));
            scroll->verticalScrollBar()->setValue(0);
            shown = current;
            return true;
        }
        if (current == root || current->isWindow()) {
            break;
        }
    }
    return false;
}

QString SettingHelpPanel::Text() const {
    return label->text();
}

bool SettingHelpPanel::eventFilter(QObject* watched, QEvent* event) {
    const auto type = event->type();
    if ((type == QEvent::Enter || type == QEvent::FocusIn) && root && watched->isWidgetType()) {
        auto* const widget = static_cast<QWidget*>(watched);
        if (widget == root || root->isAncestorOf(widget)) {
            Explain(widget);
        }
    }
    return QFrame::eventFilter(watched, event);
}

void SettingHelpPanel::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        if (!shown || !Explain(shown)) {
            ShowPlaceholder();
        }
    }
    QFrame::changeEvent(event);
}

void SettingHelpPanel::ShowPlaceholder() {
    shown = nullptr;
    label->setText(QStringLiteral("<p><i>") +
                   QCoreApplication::translate("ConfigurationShared",
                                               "Point at an option, or move to it with the "
                                               "keyboard, to read here what it does.")
                       .toHtmlEscaped() +
                   QStringLiteral("</i></p>"));
}

} // namespace ConfigurationShared

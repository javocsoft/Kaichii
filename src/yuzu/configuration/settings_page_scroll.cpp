// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/configuration/settings_page_scroll.h"

#include <QAbstractSlider>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QScrollBar>
#include <QStyle>

namespace ConfigurationShared {

namespace {

/// Hands a wheel event over an unfocused value widget to its parents (the page's scroll area).
/// An event filter that ignores the event and returns true lets QApplication pass it on.
class WheelGuard final : public QObject {
public:
    using QObject::QObject;

    bool eventFilter(QObject* watched, QEvent* event) override {
        if (event->type() == QEvent::Wheel && watched->isWidgetType() &&
            !static_cast<QWidget*>(watched)->hasFocus()) {
            event->ignore();
            return true;
        }
        return QObject::eventFilter(watched, event);
    }
};

bool ChangesOnWheel(const QWidget* widget) {
    return qobject_cast<const QComboBox*>(widget) != nullptr ||
           qobject_cast<const QAbstractSpinBox*>(widget) != nullptr ||
           (qobject_cast<const QAbstractSlider*>(widget) != nullptr &&
            qobject_cast<const QScrollBar*>(widget) == nullptr);
}

} // Anonymous namespace

SettingsPageScroll::SettingsPageScroll(QWidget* page, QWidget* parent) : QScrollArea(parent) {
    setObjectName(QStringLiteral("settingsPageScroll"));
    setFrameShape(QFrame::NoFrame);
    // The dark themes draw a border around every scroll area.
    if (!qApp->styleSheet().isEmpty()) {
        setStyleSheet(QStringLiteral(
            "QScrollArea#settingsPageScroll { border: none; background: transparent; }"));
    }
    setWidgetResizable(true);
    setAccessibleName(page->accessibleName());
    auto* const guard = new WheelGuard(this);
    for (QWidget* const widget : page->findChildren<QWidget*>()) {
        if (!ChangesOnWheel(widget)) {
            continue;
        }
        // Wheel focus would focus the widget under the cursor and let the wheel change it.
        if ((widget->focusPolicy() & Qt::WheelFocus) == Qt::WheelFocus) {
            widget->setFocusPolicy(Qt::StrongFocus);
        }
        widget->installEventFilter(guard);
    }
    setWidget(page);
}

QSize SettingsPageScroll::sizeHint() const {
    const QWidget* const page = widget();
    if (page == nullptr) {
        return QScrollArea::sizeHint();
    }
    const int bar = style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, this);
    const QMargins frame = contentsMargins();
    return page->sizeHint().expandedTo(page->minimumSizeHint()) +
           QSize{bar + frame.left() + frame.right(), frame.top() + frame.bottom()};
}

QSize FitSizeToScreen(QSize wanted, QSize available, double fraction) {
    const QSize limit{static_cast<int>(available.width() * fraction),
                      static_cast<int>(available.height() * fraction)};
    return wanted.boundedTo(limit);
}

void FitDialogToScreen(QDialog& dialog, double fraction) {
    const QWidget* const anchor = dialog.parentWidget() ? dialog.parentWidget() : &dialog;
    QScreen* screen = anchor->screen();
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }
    if (screen == nullptr) {
        dialog.adjustSize();
        return;
    }
    dialog.resize(FitSizeToScreen(dialog.sizeHint(), screen->availableGeometry().size(), fraction));
}

} // namespace ConfigurationShared

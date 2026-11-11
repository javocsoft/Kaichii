// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/configuration/settings_page_scroll.h"

#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QScreen>
#include <QSpinBox>
#include <QTest>
#include <QVBoxLayout>
#include <QWheelEvent>

namespace {

using ConfigurationShared::SettingsPageScroll;

/// A page much taller than any test window, with a combo box and a spin box at its top.
struct Page {
    Page() : widget{new QWidget} {
        widget->setAccessibleName(QStringLiteral("Graphics"));
        auto* const layout = new QVBoxLayout(widget);
        combo = new QComboBox;
        combo->addItems({QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")});
        layout->addWidget(combo);
        spin = new QSpinBox;
        layout->addWidget(spin);
        auto* const filler = new QWidget;
        filler->setMinimumSize(200, 2000);
        layout->addWidget(filler);
    }
    QWidget* widget;
    QComboBox* combo;
    QSpinBox* spin;
};

/// Sends a wheel notch down to `target`; returns whether it took the event. For a wheel event from
/// the window system, QApplication hands an event the widget left unaccepted to its parents, up to
/// the page's scroll area.
bool WheelDown(QWidget& target) {
    const QPointF centre = QRectF(target.rect()).center();
    QWheelEvent event{centre, target.mapToGlobal(centre), QPoint{}, QPoint{0, -120},
                      Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false};
    QApplication::sendEvent(&target, &event);
    return event.isAccepted();
}

} // Anonymous namespace

class SettingsPageScrollTest : public QObject {
    Q_OBJECT

private slots:
    void KeepsTheTitleAndAsksForTheWholePage() {
        Page page;
        SettingsPageScroll scroll{page.widget};
        QCOMPARE(scroll.widget(), page.widget);
        QCOMPARE(scroll.accessibleName(), QStringLiteral("Graphics"));
        QVERIFY(scroll.sizeHint().height() >= page.widget->sizeHint().height());
        QVERIFY(scroll.sizeHint().width() > page.widget->sizeHint().width());
        // The scroll area can shrink far below the page.
        QVERIFY(scroll.minimumSizeHint().height() < 500);
    }

    void WheelPassesOverUnfocusedControls() {
        Page page;
        SettingsPageScroll scroll{page.widget};
        scroll.resize(300, 200);
        scroll.show();
        QVERIFY(QTest::qWaitForWindowExposed(&scroll));
        // Showing the window may have focused the first control.
        if (QWidget* const focused = QApplication::focusWidget()) {
            focused->clearFocus();
        }
        QVERIFY(!page.combo->hasFocus());
        QCOMPARE(page.combo->focusPolicy(), Qt::StrongFocus);
        QCOMPARE(page.spin->focusPolicy(), Qt::StrongFocus);

        QVERIFY(!WheelDown(*page.combo));
        QCOMPARE(page.combo->currentIndex(), 0);
        QVERIFY(!WheelDown(*page.spin));
        QCOMPARE(page.spin->value(), 0);

        // Once clicked or tabbed into, a control takes the wheel.
        scroll.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&scroll));
        page.combo->setFocus();
        QVERIFY(page.combo->hasFocus());
        QVERIFY(WheelDown(*page.combo));
        QCOMPARE(page.combo->currentIndex(), 1);
    }

    void FitsSizesToTheScreen() {
        using ConfigurationShared::FitSizeToScreen;
        // Configure at 100 % on a Steam Deck (1280x800), and at 200 % on a 1080p screen.
        QCOMPARE(FitSizeToScreen({1223, 720}, {1280, 800}), QSize(1152, 720));
        QCOMPARE(FitSizeToScreen({2446, 1350}, {1920, 1040}), QSize(1728, 936));
        QCOMPARE(FitSizeToScreen({600, 400}, {1920, 1040}), QSize(600, 400));
    }

    void FitsADialogOfPagesToItsScreen() {
        QDialog dialog;
        auto* const layout = new QVBoxLayout(&dialog);
        Page page;
        layout->addWidget(new SettingsPageScroll(page.widget, &dialog));
        ConfigurationShared::FitDialogToScreen(dialog);
        const QSize available = dialog.screen()->availableGeometry().size();
        QVERIFY(dialog.height() <= available.height() * 9 / 10);
        QVERIFY(dialog.width() >= page.widget->sizeHint().width());
        QVERIFY(dialog.minimumSizeHint().height() < available.height() / 2);
    }
};

QTEST_MAIN(SettingsPageScrollTest)

#include "settings_page_scroll.moc"

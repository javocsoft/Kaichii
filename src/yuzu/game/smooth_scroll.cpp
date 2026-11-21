// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QAbstractScrollArea>
#include <QEasingCurve>
#include <QScrollBar>
#include <QVariantAnimation>
#include <QWheelEvent>

#include "yuzu/game/smooth_scroll.h"

SmoothScroll::SmoothScroll(std::function<QAbstractScrollArea*()> view_, QObject* parent)
    : QObject{parent}, view{std::move(view_)} {
    vertical = new QVariantAnimation(this);
    horizontal = new QVariantAnimation(this);
    for (QVariantAnimation* const animation : {vertical, horizontal}) {
        animation->setEasingCurve(QEasingCurve::OutCubic);
        animation->setDuration(DurationMs);
        // The bar is looked up on every step: the view can change between turns.
        connect(animation, &QVariantAnimation::valueChanged, this,
                [this, animation](const QVariant& value) {
                    if (QAbstractScrollArea* const area = view()) {
                        QScrollBar* const bar = animation == vertical
                                                    ? area->verticalScrollBar()
                                                    : area->horizontalScrollBar();
                        bar->setValue(value.toInt());
                    }
                });
    }
}

void SmoothScroll::Wheel(const QWheelEvent& event) {
    QAbstractScrollArea* const area = view();
    if (area == nullptr) {
        return;
    }
    int delta_x = event.angleDelta().x();
    int delta_y = event.angleDelta().y();
    if ((event.modifiers() & Qt::ShiftModifier) && delta_y != 0 && delta_x == 0) {
        delta_x = delta_y;
        delta_y = 0;
    }
    if (delta_y != 0) {
        Scroll(vertical, vertical_target, area->verticalScrollBar(), delta_y);
    }
    if (delta_x != 0) {
        Scroll(horizontal, horizontal_target, area->horizontalScrollBar(), delta_x);
    }
}

void SmoothScroll::Scroll(QVariantAnimation* animation, int& target, QScrollBar* bar, int delta) {
    // Turns during a scroll add up; a new scroll starts from where the bar is.
    if (animation->state() == QAbstractAnimation::Stopped) {
        target = bar->value();
    }
    target = qBound(bar->minimum(), target - delta, bar->maximum());
    animation->stop();
    animation->setStartValue(bar->value());
    animation->setEndValue(target);
    animation->start();
}

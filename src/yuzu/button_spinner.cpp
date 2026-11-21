// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/button_spinner.h"

#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QTimer>

ButtonSpinner::ButtonSpinner(QPushButton* button_) : QObject(button_), button{button_} {
    timer = new QTimer(this);
    timer->setInterval(80);
    connect(timer, &QTimer::timeout, this, &ButtonSpinner::Tick);
}

void ButtonSpinner::Start(std::function<QString(qint64)> text_) {
    if (!timer->isActive()) {
        restore_text = button->text();
    }
    text = std::move(text_);
    frame = 0;
    clock.start();
    Tick();
    timer->start();
}

void ButtonSpinner::Stop() {
    if (!timer->isActive()) {
        return;
    }
    timer->stop();
    button->setIcon(QIcon{});
    button->setText(restore_text);
    text = {};
}

bool ButtonSpinner::Running() const {
    return timer->isActive();
}

void ButtonSpinner::Tick() {
    // A 270° arc that turns a twelfth of a circle every tick, in the highlight colour.
    const qreal ratio = button->devicePixelRatioF();
    const int side = button->fontMetrics().height();
    QPixmap image(QSize(side, side) * ratio);
    image.setDevicePixelRatio(ratio);
    image.fill(Qt::transparent);
    {
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        QPen pen(button->palette().color(QPalette::Highlight), qMax(2.0, side / 7.0));
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        const qreal inset = pen.widthF();
        painter.drawArc(QRectF(inset, inset, side - 2 * inset, side - 2 * inset),
                        -frame * 30 * 16, 270 * 16);
    }
    frame = (frame + 1) % 12;
    QIcon icon;
    icon.addPixmap(image, QIcon::Normal);
    icon.addPixmap(image, QIcon::Disabled); // Qt would grey it out on a disabled button
    button->setIconSize(QSize(side, side));
    button->setIcon(icon);
    if (text) {
        button->setText(text(clock.elapsed() / 1000));
    }
}

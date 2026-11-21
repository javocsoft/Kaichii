// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>

#include <QBrush>
#include <QColor>
#include <QPainter>

#include "yuzu/game/cover_art.h"

namespace Yuzu {

namespace {

QImage Rounded(const QImage& filled, qreal radius) {
    QImage rounded(filled.size(), QImage::Format_ARGB32_Premultiplied);
    rounded.fill(Qt::transparent);
    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(filled));
    painter.drawRoundedRect(QRectF(QPointF(0, 0), QSizeF(filled.size())), radius, radius);
    return rounded;
}

/// The picture scaled to cover `size`, its middle kept.
QImage Filled(const QImage& picture, QSize size) {
    const QImage scaled = picture.convertToFormat(QImage::Format_ARGB32_Premultiplied)
                              .scaled(size, Qt::KeepAspectRatioByExpanding,
                                      Qt::SmoothTransformation);
    return scaled.copy((scaled.width() - size.width()) / 2,
                       (scaled.height() - size.height()) / 2, size.width(), size.height());
}

} // Anonymous namespace

QImage FitCover(const QImage& picture, QSize size, qreal radius) {
    return Rounded(Filled(picture, size), radius);
}

QImage IconBox(const QImage& icon, QSize size, qreal radius) {
    // A cheap blur: the icon shrunk to a few pixels, then stretched back smoothly.
    const QImage tiny = icon.convertToFormat(QImage::Format_ARGB32_Premultiplied)
                            .scaled(6, 6, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QImage box(size, QImage::Format_ARGB32_Premultiplied);
    box.fill(Qt::black);
    QPainter painter(&box);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(QRect(QPoint(0, 0), size),
                      Filled(tiny, QSize(6, std::max(1, 6 * size.height() / size.width()))));
    painter.fillRect(box.rect(), QColor(0, 0, 0, 90));

    const int side = size.width() * 4 / 5;
    const QImage front = FitCover(icon, QSize(side, side), radius);
    painter.drawImage((size.width() - side) / 2, (size.height() - side) / 2, front);
    painter.end();
    return Rounded(box, radius);
}

QImage CoverShadow(const QImage& cover, int alpha) {
    QImage shadow = cover.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&shadow);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(shadow.rect(), QColor(0, 0, 0, alpha));
    return shadow;
}

} // namespace Yuzu

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QBrush>
#include <QColor>
#include <QPainter>

#include "yuzu/game/cover_art.h"

namespace Yuzu {

QImage FitCover(const QImage& picture, int side, qreal radius) {
    QImage fitted = picture.convertToFormat(QImage::Format_ARGB32_Premultiplied)
                        .scaled(side, side, Qt::KeepAspectRatioByExpanding,
                                Qt::SmoothTransformation);
    fitted = fitted.copy((fitted.width() - side) / 2, (fitted.height() - side) / 2, side, side);

    QImage cover(side, side, QImage::Format_ARGB32_Premultiplied);
    cover.fill(Qt::transparent);
    QPainter painter(&cover);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(fitted));
    painter.drawRoundedRect(QRectF(0, 0, side, side), radius, radius);
    return cover;
}

QImage CoverShadow(const QImage& cover, int alpha) {
    QImage shadow = cover.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&shadow);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(shadow.rect(), QColor(0, 0, 0, alpha));
    return shadow;
}

} // namespace Yuzu

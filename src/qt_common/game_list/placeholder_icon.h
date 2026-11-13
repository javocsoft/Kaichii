// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>

#include <QColor>
#include <QFont>
#include <QHash>
#include <QImage>
#include <QPainter>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

namespace QtCommon {

/// Up to two initials of a game's name, for the icon a game doesn't have: "Donkey Kong Bananza"
/// gives "DK".
[[nodiscard]] inline QString PlaceholderInitials(const QString& name) {
    static const QRegularExpression separators(QStringLiteral("[^\\p{L}\\p{N}]+"));
    QString initials;
    for (const QString& word : name.split(separators, Qt::SkipEmptyParts)) {
        initials += word.front().toUpper();
        if (initials.size() == 2) {
            break;
        }
    }
    return initials;
}

/// A rounded square with the game's initials, in a color its name picks so each game keeps the
/// same one. Drawn on a QImage: game list items are made on a worker thread.
[[nodiscard]] inline QImage PlaceholderIcon(const QString& name, int size) {
    QImage icon(std::max(size, 1), std::max(size, 1), QImage::Format_ARGB32_Premultiplied);
    icon.fill(Qt::transparent);
    QPainter painter(&icon);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor::fromHsl(static_cast<int>(qHash(name, 0) % 360), 110, 100));
    const QRectF square(0, 0, icon.width(), icon.height());
    painter.drawRoundedRect(square, icon.width() * 0.15, icon.height() * 0.15);
    QFont font = painter.font();
    font.setPixelSize(std::max(1, icon.height() * 2 / 5));
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(square, Qt::AlignCenter, PlaceholderInitials(name));
    return icon;
}

} // namespace QtCommon

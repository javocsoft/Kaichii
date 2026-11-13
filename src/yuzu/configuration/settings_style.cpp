// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/configuration/settings_style.h"

#include <array>
#include <cmath>
#include <utility>

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QWidget>

#include "qt_common/config/uisettings.h"

namespace ConfigurationShared {

namespace {

/// Draws `glyph` in a 24 by 24 box, as line art.
void DrawGlyph(QPainter& p, CategoryGlyph glyph) {
    switch (glyph) {
    case CategoryGlyph::Performance: {
        // A gauge: an arc open at the bottom, and its needle.
        p.drawArc(QRectF(4, 6, 16, 16), -30 * 16, 240 * 16);
        p.drawLine(QPointF(12, 14), QPointF(16.5, 9.5));
        p.drawPoint(QPointF(12, 14));
        break;
    }
    case CategoryGlyph::Image: {
        p.drawRoundedRect(QRectF(3, 5, 18, 14), 2, 2);
        QPainterPath hills;
        hills.moveTo(3.5, 17);
        hills.lineTo(9, 11);
        hills.lineTo(13, 15);
        hills.lineTo(16, 12);
        hills.lineTo(20.5, 16.5);
        p.drawPath(hills);
        p.drawEllipse(QPointF(15.5, 9), 1.4, 1.4);
        break;
    }
    case CategoryGlyph::Smoothness: {
        QPainterPath wave;
        wave.moveTo(3, 12);
        wave.cubicTo(6, 5, 9, 5, 12, 12);
        wave.cubicTo(15, 19, 18, 19, 21, 12);
        p.drawPath(wave);
        break;
    }
    case CategoryGlyph::Audio: {
        QPainterPath speaker;
        speaker.moveTo(4, 9.5);
        speaker.lineTo(8, 9.5);
        speaker.lineTo(12.5, 5.5);
        speaker.lineTo(12.5, 18.5);
        speaker.lineTo(8, 14.5);
        speaker.lineTo(4, 14.5);
        speaker.closeSubpath();
        p.drawPath(speaker);
        p.drawArc(QRectF(11, 8.5, 7, 7), -45 * 16, 90 * 16);
        p.drawArc(QRectF(10, 5, 11, 14), -50 * 16, 100 * 16);
        break;
    }
    case CategoryGlyph::Controls: {
        p.drawRoundedRect(QRectF(2.5, 7, 19, 11), 5, 5);
        p.drawLine(QPointF(6, 12.5), QPointF(10, 12.5));
        p.drawLine(QPointF(8, 10.5), QPointF(8, 14.5));
        p.drawEllipse(QPointF(15.5, 11.5), 0.6, 0.6);
        p.drawEllipse(QPointF(17.5, 13.5), 0.6, 0.6);
        break;
    }
    case CategoryGlyph::System: {
        p.drawRoundedRect(QRectF(7, 7, 10, 10), 1.5, 1.5);
        p.drawRect(QRectF(10, 10, 4, 4));
        for (const double at : {10.0, 14.0}) {
            p.drawLine(QPointF(at, 4), QPointF(at, 7));
            p.drawLine(QPointF(at, 17), QPointF(at, 20));
            p.drawLine(QPointF(4, at), QPointF(7, at));
            p.drawLine(QPointF(17, at), QPointF(20, at));
        }
        break;
    }
    case CategoryGlyph::Interface: {
        p.drawRoundedRect(QRectF(3, 5, 18, 14), 2, 2);
        p.drawLine(QPointF(3, 9), QPointF(21, 9));
        p.drawPoint(QPointF(5.5, 7));
        p.drawPoint(QPointF(7.5, 7));
        break;
    }
    case CategoryGlyph::Advanced: {
        // Three sliders, each line broken by its knob.
        constexpr double Knob = 2.0;
        constexpr std::array<std::pair<double, double>, 3> sliders{
            {{7.0, 9.0}, {12.0, 15.0}, {17.0, 7.0}}};
        for (const auto& [y, x] : sliders) {
            p.drawLine(QPointF(4, y), QPointF(x - Knob, y));
            p.drawLine(QPointF(x + Knob, y), QPointF(20, y));
            p.drawEllipse(QPointF(x, y), Knob, Knob);
        }
        break;
    }
    }
}

QPixmap GlyphPixmap(CategoryGlyph glyph, const QColor& color, int size, qreal ratio) {
    const int pixels = static_cast<int>(std::lround(size * ratio));
    QImage image(pixels, pixels, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(pixels / 24.0, pixels / 24.0);
    painter.setPen(QPen(color, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    DrawGlyph(painter, glyph);
    painter.end();
    QPixmap pixmap = QPixmap::fromImage(image);
    pixmap.setDevicePixelRatio(ratio);
    return pixmap;
}

} // Anonymous namespace

QIcon CategoryIcon(CategoryGlyph glyph, const QColor& color, const QColor& selected, int size) {
    QIcon icon;
    for (const qreal ratio : {1.0, 2.0}) {
        icon.addPixmap(GlyphPixmap(glyph, color, size, ratio), QIcon::Normal);
        icon.addPixmap(GlyphPixmap(glyph, selected, size, ratio), QIcon::Selected);
    }
    return icon;
}

bool UsesSystemPalette() {
    const std::string& theme = UISettings::values.theme;
    return theme.empty() || theme == "default" || theme == "colorful";
}

QString SettingsDialogStyleSheet() {
    return QStringLiteral(
        // Sections as cards.
        "QGroupBox {"
        "  border: 1px solid palette(mid);"
        "  border-radius: 8px;"
        "  margin-top: 1.3em;"
        "  padding: 0.25em 0.4em 0.2em 0.4em;"
        "  background-color: palette(base);"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  left: 0.7em;"
        "  padding: 0 0.3em;"
        "}"
        // The categories as a navigation list.
        "QListWidget#selectorList {"
        "  border: none;"
        "  background: transparent;"
        "  outline: 0;"
        "}"
        "QListWidget#selectorList::item {"
        "  padding: 0.35em 0.3em;"
        "  border-radius: 6px;"
        "}"
        "QListWidget#selectorList::item:selected {"
        "  background: palette(highlight);"
        "  color: palette(highlighted-text);"
        "}"
        "QListWidget#selectorList::item:hover:!selected {"
        "  background: palette(midlight);"
        "}"
        // The help panel set apart from the pages.
        "QFrame#SettingHelpPanel {"
        "  border: 1px solid palette(mid);"
        "  border-radius: 8px;"
        "  background-color: palette(alternate-base);"
        "}"
        "QFrame#SettingHelpPanel QScrollArea,"
        "QFrame#SettingHelpPanel QWidget#qt_scrollarea_viewport,"
        "QFrame#SettingHelpPanel QLabel {"
        "  background: transparent;"
        "}");
}

void StyleSettingsDialog(QWidget& dialog) {
    if (UsesSystemPalette()) {
        dialog.setStyleSheet(SettingsDialogStyleSheet());
    }
}

} // namespace ConfigurationShared

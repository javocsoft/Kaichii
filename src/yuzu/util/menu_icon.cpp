// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QBuffer>
#include <QFile>
#include <QImageReader>
#include <QPixmap>

#include "qt_common/config/uisettings.h"
#include "yuzu/util/menu_icon.h"

namespace {

/// The colour the icons are drawn in, replaced by the theme's.
constexpr char SourceColor[] = "#b4b6bc";
constexpr char LightThemeColor[] = "#55585f";
constexpr char DarkThemeColor[] = "#b4b6bc";
constexpr char HighlightColor[] = "#ffffff";

/// Menus draw 16 px icons, scaled with the screen; these cover 100% to 300%.
constexpr int Sizes[] = {16, 20, 24, 32, 48};

QPixmap Render(QByteArray svg, int size) {
    QBuffer buffer{&svg};
    buffer.open(QIODevice::ReadOnly);
    // Qt's SVG image plugin, which ships with every build: the frontend doesn't link Qt Svg.
    QImageReader reader{&buffer, "svg"};
    reader.setScaledSize(QSize{size, size});
    return QPixmap::fromImage(reader.read());
}

} // Anonymous namespace

QIcon MenuIcon(const char* name) {
    QFile file{QStringLiteral(":/menu_icons/%1.svg").arg(QLatin1String(name))};
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QByteArray svg = file.readAll();
    const bool dark = UISettings::IsDarkTheme();
    const QByteArray normal =
        QByteArray{svg}.replace(SourceColor, dark ? DarkThemeColor : LightThemeColor);
    const QByteArray highlighted = QByteArray{svg}.replace(SourceColor, HighlightColor);

    QIcon icon;
    for (const int size : Sizes) {
        const QPixmap pixmap = Render(normal, size);
        if (pixmap.isNull()) {
            return {};
        }
        icon.addPixmap(pixmap, QIcon::Normal);
        // A dark theme highlights a row in its accent with white text; light themes keep the
        // normal tone, as their highlight is light.
        if (dark) {
            icon.addPixmap(Render(highlighted, size), QIcon::Active);
        }
    }
    return icon;
}

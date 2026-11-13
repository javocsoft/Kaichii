// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "qt_common/util/guide.h"

#include <QLocale>
#include <QRegularExpression>

#include "common/scm_rev.h"
#include "qt_common/config/uisettings.h"

namespace QtCommon::Guide {

QUrl PageUrl(const QString& repository, const QString& language, const QString& page) {
    static const QRegularExpression repository_pattern(
        QStringLiteral("^([A-Za-z0-9-]+)/([A-Za-z0-9._-]+)$"));
    static const QRegularExpression page_pattern(QStringLiteral("^([A-Za-z0-9-]+/)*$"));
    const auto match = repository_pattern.match(repository);
    if (!match.hasMatch() || !page_pattern.match(page).hasMatch()) {
        return {};
    }
    // GitHub Pages serves a project at <owner>.github.io/<name>/; the site keeps English at its
    // root and Spanish under es/.
    const bool spanish = language == QStringLiteral("es") ||
                         language.startsWith(QStringLiteral("es_")) ||
                         language.startsWith(QStringLiteral("es-"));
    return QUrl(QStringLiteral("https://%1.github.io/%2/%3%4")
                    .arg(match.captured(1).toLower(), match.captured(2),
                         spanish ? QStringLiteral("es/") : QString{}, page));
}

QUrl Page(const QString& page) {
    const auto& chosen = UISettings::values.language.GetValue();
    const QString language =
        chosen.empty() ? QLocale::system().name() : QString::fromStdString(chosen);
    return PageUrl(QString::fromUtf8(Common::g_build_auto_update_repo), language, page);
}

} // namespace QtCommon::Guide

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "qt_common/util/translations.h"

#include <QLocale>
#include <QTranslator>

namespace QtCommon::Translations {

bool Load(QTranslator& interface, QTranslator& qt, const QString& language,
          const QString& directory) {
    // A failed load leaves the translator empty, so a language Qt has no catalog for keeps
    // Qt's strings in English rather than those of the language chosen before.
    if (language.isEmpty()) {
        (void)qt.load(QLocale(), QStringLiteral("qtbase"), QStringLiteral("_"), directory);
        return interface.load(QLocale(), {}, {}, directory);
    }
    (void)qt.load(QStringLiteral("qtbase_") + language, directory);
    return interface.load(language, directory);
}

} // namespace QtCommon::Translations

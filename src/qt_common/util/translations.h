// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

class QTranslator;

namespace QtCommon::Translations {

/// Loads the catalogs for `language` (the system's languages when it is empty) from `directory`:
/// the interface's (`<language>.qm`) into `interface`, and Qt's own (`qtbase_<language>.qm`: its
/// standard buttons, wizard buttons and dialogs) into `qt`. A translator whose catalog is missing
/// is left empty. Returns whether the interface's catalog loaded.
bool Load(QTranslator& interface, QTranslator& qt, const QString& language,
          const QString& directory);

} // namespace QtCommon::Translations

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QUrl>

namespace QtCommon::Guide {

/// A page of the player's guide that `repository` ("owner/name") publishes on its GitHub Pages
/// site: in Spanish when `language` (an interface language such as "es" or "es_ES") is Spanish,
/// in English otherwise. `page` is the page's folder in the site ("Compatibility/"), empty for
/// the start page. An invalid repository gives an empty URL.
[[nodiscard]] QUrl PageUrl(const QString& repository, const QString& language,
                           const QString& page = {});

/// PageUrl for this build's repository and the interface language (the one chosen in Kaichii, or
/// the system's).
[[nodiscard]] QUrl Page(const QString& page = {});

} // namespace QtCommon::Guide

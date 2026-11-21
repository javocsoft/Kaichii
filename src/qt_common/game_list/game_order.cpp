// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>

#include <QFileInfo>
#include <QStandardItem>
#include <QStandardItemModel>

#include "qt_common/config/uisettings.h"
#include "qt_common/game_list/game_list_p.h"
#include "qt_common/game_list/game_order.h"

namespace QtCommon {

namespace {

/// The name without accents and in lower case, so "Ōkami" sorts with "okami".
QString Folded(const QString& name) {
    const QString decomposed = name.normalized(QString::NormalizationForm_D);
    QString folded;
    folded.reserve(decomposed.size());
    for (const QChar c : decomposed) {
        if (c.category() != QChar::Mark_NonSpacing) {
            folded += c.toCaseFolded();
        }
    }
    return folded;
}

/// Compares names part by part, runs of digits by their value: "Mario 2" before "Mario 10". Done
/// here rather than with QCollator, whose numeric mode depends on how Qt was built and the locale.
int NaturalCompare(const QString& a, const QString& b) {
    const QString x = Folded(a);
    const QString y = Folded(b);
    qsizetype i = 0;
    qsizetype j = 0;
    while (i < x.size() && j < y.size()) {
        if (x[i].isDigit() && y[j].isDigit()) {
            const qsizetype start_i = i;
            const qsizetype start_j = j;
            while (i < x.size() && x[i].isDigit()) {
                ++i;
            }
            while (j < y.size() && y[j].isDigit()) {
                ++j;
            }
            // Leading zeros aside, a longer number is larger; equal lengths compare as text.
            QStringView number_x = QStringView{x}.sliced(start_i, i - start_i);
            QStringView number_y = QStringView{y}.sliced(start_j, j - start_j);
            while (number_x.size() > 1 && number_x.front() == u'0') {
                number_x = number_x.sliced(1);
            }
            while (number_y.size() > 1 && number_y.front() == u'0') {
                number_y = number_y.sliced(1);
            }
            if (number_x.size() != number_y.size()) {
                return number_x.size() < number_y.size() ? -1 : 1;
            }
            if (const int by_digits = number_x.compare(number_y); by_digits != 0) {
                return by_digits;
            }
            continue;
        }
        if (x[i] != y[j]) {
            return x[i] < y[j] ? -1 : 1;
        }
        ++i;
        ++j;
    }
    if (i < x.size()) {
        return 1;
    }
    return j < y.size() ? -1 : 0;
}

/// A game's name for sorting: its title, or its file name when it has none.
QString SortName(const QString& title, const QString& path) {
    return title.isEmpty() ? QFileInfo(path).completeBaseName() : title;
}

} // Anonymous namespace

bool GameSortsBefore(const QString& title_a, const QString& path_a, const QString& title_b,
                     const QString& path_b) {
    const int by_name = NaturalCompare(SortName(title_a, path_a), SortName(title_b, path_b));
    if (by_name != 0) {
        return by_name < 0;
    }
    return path_a < path_b;
}

GameSort CurrentGameSort() {
    return {UISettings::values.game_list_sort.GetValue(),
            UISettings::values.game_list_sort_descending.GetValue()};
}

bool GameSortsBefore(const QStandardItem& a, const QStandardItem& b, GameSort sort) {
    const auto by_name = [&a, &b] {
        return GameSortsBefore(a.data(GameListItemPath::TitleRole).toString(),
                               a.data(GameListItemPath::FullPathRole).toString(),
                               b.data(GameListItemPath::TitleRole).toString(),
                               b.data(GameListItemPath::FullPathRole).toString());
    };
    // Compares the key, a before b in ascending order: -1, 0 or 1.
    int by_key = 0;
    switch (sort.key) {
    case Settings::GameListSort::Name:
        if (by_name()) {
            by_key = -1;
        } else if (GameSortsBefore(b.data(GameListItemPath::TitleRole).toString(),
                                   b.data(GameListItemPath::FullPathRole).toString(),
                                   a.data(GameListItemPath::TitleRole).toString(),
                                   a.data(GameListItemPath::FullPathRole).toString())) {
            by_key = 1;
        }
        break;
    case Settings::GameListSort::DateAdded: {
        // A game whose date isn't known counts as the oldest.
        const QDateTime added_a = a.data(GameListItemPath::AddedRole).toDateTime();
        const QDateTime added_b = b.data(GameListItemPath::AddedRole).toDateTime();
        const qint64 time_a = added_a.isValid() ? added_a.toMSecsSinceEpoch() : 0;
        const qint64 time_b = added_b.isValid() ? added_b.toMSecsSinceEpoch() : 0;
        by_key = time_a < time_b ? -1 : (time_a > time_b ? 1 : 0);
        break;
    }
    case Settings::GameListSort::PlayTime: {
        const qulonglong played_a = a.data(GameListItemPath::PlayTimeRole).toULongLong();
        const qulonglong played_b = b.data(GameListItemPath::PlayTimeRole).toULongLong();
        by_key = played_a < played_b ? -1 : (played_a > played_b ? 1 : 0);
        break;
    }
    }
    if (by_key != 0) {
        return sort.descending ? by_key > 0 : by_key < 0;
    }
    return by_name();
}

int SortedGameRow(const QStandardItem& parent, const QStandardItem& game, GameSort sort) {
    // The first row that sorts after the new game, so equal games keep their arrival order.
    int low = 0;
    int high = parent.rowCount();
    while (low < high) {
        const int middle = (low + high) / 2;
        const QStandardItem* const row = parent.child(middle, 0);
        if (row != nullptr && !GameSortsBefore(game, *row, sort)) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    return low;
}

void SortGames(QStandardItemModel& model, GameSort sort) {
    QList<QList<QStandardItem*>> rows;
    rows.reserve(model.rowCount());
    while (model.rowCount() > 0) {
        rows.append(model.takeRow(0));
    }
    std::stable_sort(rows.begin(), rows.end(),
                     [sort](const QList<QStandardItem*>& a, const QList<QStandardItem*>& b) {
                         return !a.isEmpty() && !b.isEmpty() &&
                                GameSortsBefore(*a.front(), *b.front(), sort);
                     });
    for (const QList<QStandardItem*>& row : rows) {
        model.appendRow(row);
    }
}

void SortForView(QStandardItemModel& model, bool tree_view, int column, Qt::SortOrder order) {
    if (tree_view) {
        model.sort(column, order);
    }
}

QDateTime GameFileAdded(const QString& path) {
    const QFileInfo file(path);
    if (!file.exists()) {
        return {};
    }
    const QDateTime born = file.birthTime();
    return born.isValid() ? born : file.lastModified();
}

} // namespace QtCommon

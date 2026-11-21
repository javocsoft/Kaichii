// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QFileInfo>
#include <QStandardItem>
#include <QStandardItemModel>

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

int SortedGameRow(const QStandardItem& parent, const QString& title, const QString& path) {
    // The first row that sorts after the new game.
    int low = 0;
    int high = parent.rowCount();
    while (low < high) {
        const int middle = (low + high) / 2;
        const QStandardItem* const row = parent.child(middle, 0);
        const bool row_first =
            row != nullptr &&
            GameSortsBefore(row->data(GameListItemPath::TitleRole).toString(),
                            row->data(GameListItemPath::FullPathRole).toString(), title, path);
        if (row_first) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    return low;
}

void SortForView(QStandardItemModel& model, bool tree_view, int column, Qt::SortOrder order) {
    if (tree_view) {
        model.sort(column, order);
    }
}

} // namespace QtCommon

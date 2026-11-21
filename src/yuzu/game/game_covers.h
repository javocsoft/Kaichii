// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QHash>
#include <QListView>
#include <QPixmap>
#include <QString>
#include <QStyledItemDelegate>

class GameListModel;

/// Draws a game as its cover alone: the game's own icon, large, with rounded corners and a shadow.
class GameCoverDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit GameCoverDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    bool helpEvent(QHelpEvent* event, QAbstractItemView* view, const QStyleOptionViewItem& option,
                   const QModelIndex& index) override;

    /// Sets the cover's side and the cell it sits in. Changing the side drops the cached covers.
    void SetSizes(int cover, const QSize& cell);
    /// Where the cover is drawn inside a cell, for hit tests.
    [[nodiscard]] QRect CoverRect(const QRect& cell) const;

private:
    struct Art {
        QPixmap cover;
        /// The cover's shape in black: its shadow follows transparent pictures instead of a square.
        QPixmap shadow;
    };

    /// The cover at the current side, decoded once and kept while it may be on screen.
    const Art& Cover(const QModelIndex& index, qreal device_pixel_ratio) const;
    [[nodiscard]] qreal Radius() const;

    int m_cover = 200;
    QSize m_cell{220, 220};
    mutable QHash<QString, Art> m_cache;
};

/// The game list as a shelf of large covers, without names: "View > Game List Mode > Cover View".
class GameCovers : public QListView {
    Q_OBJECT

public:
    explicit GameCovers(QWidget* parent = nullptr);

    void SetModel(GameListModel* model);
    void ApplyFilter(const QString& edit_filter_text, GameListModel* model);
    /// Fits whole columns of covers to the view's width.
    void UpdateCoverSize();

    QModelIndex indexAt(const QPoint& point) const override;

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    GameCoverDelegate* m_delegate = nullptr;
};

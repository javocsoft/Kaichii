// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QHash>
#include <QAbstractItemView>
#include <QPersistentModelIndex>
#include <QList>
#include <QPixmap>
#include <QString>
#include <QStyledItemDelegate>

#include "yuzu/game/cover_layout.h"

class QStandardItemModel;

/// What a shelf shows for each game.
enum class CoverShape {
    /// The game's own icon, square: the cover view.
    Square,
    /// The box art the player chose, as a case's front, or the icon on a stand-in case: the full
    /// cover view.
    Box,
};

/// Draws a game as its cover alone, large, with rounded corners and a shadow.
class GameCoverDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit GameCoverDelegate(CoverShape shape = CoverShape::Square, QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    bool helpEvent(QHelpEvent* event, QAbstractItemView* view, const QStyleOptionViewItem& option,
                   const QModelIndex& index) override;

    /// Sets the cover's size and the cell it sits in. Changing the size drops the cached covers.
    void SetSizes(const QSize& cover, const QSize& cell);
    /// Drops the cached covers, so changed icons and box art are read again.
    void ClearCache();
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

    CoverShape m_shape;
    QSize m_cover{200, 200};
    QSize m_cell{220, 220};
    mutable QHash<QString, Art> m_cache;
};

/// The game list as a shelf of large covers, without names: "View > Game List Mode > Cover View",
/// or "Full Cover View" with the games' cases. Games keep the model's order (by name), optionally
/// in two sections, Switch 2 and Switch, each under a heading.
class GameCovers : public QAbstractItemView {
    Q_OBJECT

public:
    explicit GameCovers(CoverShape shape, QWidget* parent = nullptr);

    void SetModel(QStandardItemModel* model);
    void ApplyFilter(const QString& edit_filter_text, QStandardItemModel* model);
    /// Fits whole columns of covers to the view's width.
    void UpdateCoverSize();
    /// Shows Switch 2 and Switch games apart, each section under a heading.
    void SetSections(bool sections);
    void SetRowHidden(int row, bool hidden);

    QModelIndex indexAt(const QPoint& point) const override;
    QRect visualRect(const QModelIndex& index) const override;
    void scrollTo(const QModelIndex& index, ScrollHint hint = EnsureVisible) override;

protected:
    QModelIndex moveCursor(CursorAction action, Qt::KeyboardModifiers modifiers) override;
    int horizontalOffset() const override;
    int verticalOffset() const override;
    bool isIndexHidden(const QModelIndex& index) const override;
    void setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags flags) override;
    QRegion visualRegionForSelection(const QItemSelection& selection) const override;

    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void updateGeometries() override;

    void rowsInserted(const QModelIndex& parent, int start, int end) override;
    void rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) override;
    void dataChanged(const QModelIndex& top_left, const QModelIndex& bottom_right,
                     const QList<int>& roles = QList<int>()) override;
    void reset() override;

private:
    /// Marks the shelf for laying out again before it is next used.
    void Invalidate();
    /// Lays the shelf out if the model, the filter or the size changed since.
    void EnsureLayout() const;
    /// A game's cell in the shelf's coordinates; empty when it isn't shown.
    QRect CellRect(int row) const;
    /// Where a game is in reading order; -1 when it isn't shown.
    int SpotOf(const QModelIndex& index) const;
    QModelIndex IndexOfSpot(int spot) const;
    /// The nearest game in the next row up (-1) or down (+1) from a spot, or -1.
    int SpotInRow(int spot, int direction) const;

    CoverShape m_shape;
    GameCoverDelegate* m_delegate = nullptr;
    bool m_sections = false;
    int m_columns = 1;
    QSize m_cell{220, 220};
    /// Games the filter hides. A list: persistent indexes move as rows are inserted, so they
    /// can't be hashed.
    QList<QPersistentModelIndex> m_hidden;
    QPersistentModelIndex m_hover;

    mutable bool m_dirty = true;
    mutable Yuzu::Shelf m_shelf;
    /// A shown game's place in m_shelf.spots, by model row.
    mutable QHash<int, int> m_spot_of_row;
};

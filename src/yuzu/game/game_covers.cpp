// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <QHelpEvent>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QToolTip>

#include "qt_common/game_list/box_art.h"
#include "qt_common/game_list/game_list_p.h"
#include "qt_common/game_list/model.h"
#include "yuzu/game/common.h"
#include "yuzu/game/cover_art.h"
#include "yuzu/game/cover_layout.h"
#include "yuzu/game/game_covers.h"

namespace {

constexpr int Gap = Yuzu::CoverGap;
/// Covers kept decoded; the shelf shows far fewer at once.
constexpr qsizetype CacheLimit = 300;
/// How dark each of the shadow's layers is, out of 255.
constexpr int ShadowAlpha = 22;

QString Title(const QModelIndex& index) {
    QString title = index.data(GameListItemPath::TitleRole).toString();
    if (title.isEmpty()) {
        title = index.data(Qt::DisplayRole).toString().split(QLatin1Char('\n')).first();
    }
    return title;
}

bool IsGame(const QModelIndex& index) {
    return index.data(GameListItem::TypeRole).toInt() == static_cast<int>(GameListItemType::Game);
}

} // Anonymous namespace

GameCoverDelegate::GameCoverDelegate(CoverShape shape, QObject* parent)
    : QStyledItemDelegate{parent}, m_shape{shape} {
    setObjectName(QStringLiteral("GameCover"));
}

void GameCoverDelegate::SetSizes(const QSize& cover, const QSize& cell) {
    if (cover != m_cover) {
        m_cache.clear();
    }
    m_cover = cover;
    m_cell = cell;
}

void GameCoverDelegate::ClearCache() {
    m_cache.clear();
}

QRect GameCoverDelegate::CoverRect(const QRect& cell) const {
    return {QPoint{cell.left() + (cell.width() - m_cover.width()) / 2, cell.top() + Gap / 2},
            m_cover};
}

QSize GameCoverDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const {
    return m_cell;
}

qreal GameCoverDelegate::Radius() const {
    // A case's corners are tighter than an icon's.
    return m_shape == CoverShape::Box ? std::max(4.0, m_cover.width() / 30.0)
                                      : std::max(6.0, m_cover.width() / 24.0);
}

const GameCoverDelegate::Art& GameCoverDelegate::Cover(const QModelIndex& index,
                                                       qreal device_pixel_ratio) const {
    const QString key = index.data(GameListItemPath::FullPathRole).toString();
    if (const auto it = m_cache.constFind(key); it != m_cache.cend()) {
        return it.value();
    }
    const QSize size = (QSizeF(m_cover) * device_pixel_ratio).toSize();
    const qreal radius = Radius() * device_pixel_ratio;
    const QImage box_art =
        m_shape == CoverShape::Box
            ? QtCommon::BoxArt::Load(index.data(GameListItemPath::ProgramIdRole).toULongLong())
            : QImage{};
    QImage cover;
    if (!box_art.isNull()) {
        cover = Yuzu::FitCover(box_art, size, radius);
    } else {
        QImage icon = QImage::fromData(index.data(GameListItemPath::CoverRole).toByteArray());
        if (icon.isNull()) {
            // No icon in the game: the list's own picture (its initials) at the cover's size.
            icon = QtCommon::PlaceholderIcon(Title(index), size.width());
        }
        cover = m_shape == CoverShape::Box ? Yuzu::IconBox(icon, size, radius)
                                           : Yuzu::FitCover(icon, size, radius);
    }
    Art art{QPixmap::fromImage(cover),
            QPixmap::fromImage(Yuzu::CoverShadow(cover, ShadowAlpha))};
    art.cover.setDevicePixelRatio(device_pixel_ratio);
    art.shadow.setDevicePixelRatio(device_pixel_ratio);
    if (m_cache.size() >= CacheLimit) {
        m_cache.clear();
    }
    return m_cache.insert(key, std::move(art)).value();
}

void GameCoverDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const {
    if (!index.isValid()) {
        return;
    }
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    const QRect cover = CoverRect(option.rect);
    const qreal radius = Radius();
    const QPalette& palette = option.palette;

    QPainterPath shape;
    shape.addRoundedRect(QRectF{cover}, radius, radius);
    if (IsGame(index)) {
        // A soft shadow under the picture's own shape (the shelf look), then the picture, whose
        // transparent parts show the shelf behind it.
        const Art& art = Cover(index, painter->device()->devicePixelRatioF());
        for (int step = 1; step <= 4; ++step) {
            painter->drawPixmap(cover.translated(0, step), art.shadow);
        }
        painter->drawPixmap(cover, art.cover);
    } else {
        // Not a game (the "add a folder" entry): its icon and name on a plain card.
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, ShadowAlpha));
        for (int step = 1; step <= 4; ++step) {
            painter->drawPath(shape.translated(0, step));
        }
        painter->setBrush(palette.window().color().lighter(110));
        painter->setPen(QPen(palette.mid().color(), 1));
        painter->drawPath(shape);
        const QPixmap icon = index.data(Qt::DecorationRole).value<QPixmap>();
        const int icon_side = m_cover.width() / 3;
        const QRect icon_rect{cover.left() + (m_cover.width() - icon_side) / 2,
                              cover.top() + m_cover.height() / 4, icon_side, icon_side};
        if (!icon.isNull()) {
            painter->drawPixmap(icon_rect, icon);
        }
        painter->setPen(palette.text().color());
        painter->drawText(QRect{cover.left() + 8, icon_rect.bottom() + 8, m_cover.width() - 16,
                                cover.bottom() - icon_rect.bottom() - 16},
                          Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, Title(index));
    }

    // Hover and selection as an outline in the theme's highlight.
    if (option.state & (QStyle::State_Selected | QStyle::State_MouseOver)) {
        const bool selected = option.state & QStyle::State_Selected;
        QColor outline = palette.highlight().color();
        if (!selected) {
            outline.setAlpha(170);
        }
        const qreal width = selected ? 3.0 : 2.0;
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(outline, width));
        painter->drawRoundedRect(QRectF{cover}.adjusted(-width / 2, -width / 2, width / 2,
                                                        width / 2),
                                 radius + width / 2, radius + width / 2);
    }
    painter->restore();
}

bool GameCoverDelegate::helpEvent(QHelpEvent* event, QAbstractItemView* view,
                                  const QStyleOptionViewItem& option, const QModelIndex& index) {
    if (event->type() != QEvent::ToolTip || !index.isValid()) {
        return QStyledItemDelegate::helpEvent(event, view, option, index);
    }
    // Without names on the shelf, the tooltip leads with the game's name.
    QString text = Title(index);
    if (const QString details = index.data(Qt::ToolTipRole).toString(); !details.isEmpty()) {
        text += QLatin1Char('\n') + details;
    }
    QToolTip::showText(event->globalPos(), text, view);
    return true;
}

GameCovers::GameCovers(CoverShape shape, QWidget* parent) : QListView{parent}, m_shape{shape} {
    m_delegate = new GameCoverDelegate(shape, this);
    setItemDelegate(m_delegate);

    setViewMode(QListView::ListMode);
    setFlow(QListView::LeftToRight);
    setWrapping(true);
    setResizeMode(QListView::Adjust);
    setUniformItemSizes(true);
    setSpacing(0);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setMouseTracking(true);
    setFrameShape(QFrame::NoFrame);
}

void GameCovers::SetModel(GameListModel* model) {
    QListView::setModel(model);
    // A repopulate after a game's icon or box art changed must draw the new picture.
    connect(model, &QAbstractItemModel::rowsAboutToBeRemoved, m_delegate,
            &GameCoverDelegate::ClearCache);
    connect(model, &QAbstractItemModel::modelAboutToBeReset, m_delegate,
            &GameCoverDelegate::ClearCache);
    UpdateCoverSize();
}

void GameCovers::ApplyFilter(const QString& edit_filter_text, GameListModel* model) {
    for (int i = 0; i < model->rowCount(); ++i) {
        if (const QStandardItem* item = model->item(i, 0)) {
            setRowHidden(i, !Yuzu::FilterMatches(edit_filter_text, item));
        }
    }
}

void GameCovers::UpdateCoverSize() {
    QSize cover;
    QSize cell;
    if (m_shape == CoverShape::Box) {
        const auto layout = Yuzu::ComputeBoxLayout(viewport()->width());
        cover = {layout.width, layout.height};
        cell = {layout.cell_width, layout.height + Gap};
    } else {
        const auto layout = Yuzu::ComputeCoverLayout(viewport()->width());
        cover = {layout.cover, layout.cover};
        cell = {layout.cell_width, layout.cover + Gap};
    }
    if (gridSize() != cell) {
        m_delegate->SetSizes(cover, cell);
        setGridSize(cell);
        viewport()->update();
    }
}

QModelIndex GameCovers::indexAt(const QPoint& point) const {
    const QModelIndex index = QListView::indexAt(point);
    if (!index.isValid() || !m_delegate->CoverRect(visualRect(index)).contains(point)) {
        return {};
    }
    return index;
}

void GameCovers::resizeEvent(QResizeEvent* event) {
    QListView::resizeEvent(event);
    UpdateCoverSize();
}

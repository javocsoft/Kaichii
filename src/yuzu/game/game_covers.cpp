// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <cstdlib>
#include <vector>
#include <QHelpEvent>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSet>
#include <QStandardItemModel>
#include <QToolTip>

#include "qt_common/game_list/game_pictures.h"
#include "qt_common/game_list/game_list_p.h"
#include "qt_common/game_list/game_order.h"
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

GameCovers::GameCovers(CoverShape shape, QWidget* parent)
    : QAbstractItemView{parent}, m_shape{shape} {
    m_delegate = new GameCoverDelegate(shape, this);
    setItemDelegate(m_delegate);

    setSelectionMode(QAbstractItemView::SingleSelection);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setMouseTracking(true);
    setFrameShape(QFrame::NoFrame);
}

void GameCovers::SetModel(QStandardItemModel* model) {
    setModel(model);
    // A repopulate after a game's icon or box art changed must draw the new picture.
    connect(model, &QAbstractItemModel::rowsAboutToBeRemoved, m_delegate,
            &GameCoverDelegate::ClearCache);
    connect(model, &QAbstractItemModel::modelAboutToBeReset, m_delegate,
            &GameCoverDelegate::ClearCache);
    // Rows moved or removed change the order, which rowsInserted alone doesn't cover.
    connect(model, &QAbstractItemModel::rowsRemoved, this, &GameCovers::Invalidate);
    connect(model, &QAbstractItemModel::rowsMoved, this, &GameCovers::Invalidate);
    connect(model, &QAbstractItemModel::layoutChanged, this, &GameCovers::Invalidate);
    UpdateCoverSize();
}

void GameCovers::ApplyFilter(const QString& edit_filter_text, QStandardItemModel* model) {
    for (int i = 0; i < model->rowCount(); ++i) {
        if (const QStandardItem* item = model->item(i, 0)) {
            SetRowHidden(i, !Yuzu::FilterMatches(edit_filter_text, item));
        }
    }
}

void GameCovers::SetRowHidden(int row, bool hidden) {
    if (model() == nullptr) {
        return;
    }
    const QPersistentModelIndex index = model()->index(row, 0, rootIndex());
    if (hidden == m_hidden.contains(index)) {
        return;
    }
    if (hidden) {
        m_hidden.append(index);
    } else {
        m_hidden.removeAll(index);
    }
    Invalidate();
}

void GameCovers::SetSections(bool sections) {
    if (sections != m_sections) {
        m_sections = sections;
        Invalidate();
    }
}

void GameCovers::UpdateCoverSize() {
    QSize cover;
    QSize cell;
    int columns = 1;
    if (m_shape == CoverShape::Box) {
        const auto layout = Yuzu::ComputeBoxLayout(viewport()->width());
        cover = {layout.width, layout.height};
        cell = {layout.cell_width, layout.height + Gap};
        columns = layout.columns;
    } else {
        const auto layout = Yuzu::ComputeCoverLayout(viewport()->width());
        cover = {layout.cover, layout.cover};
        cell = {layout.cell_width, layout.cover + Gap};
        columns = layout.columns;
    }
    if (cell != m_cell || columns != m_columns) {
        m_delegate->SetSizes(cover, cell);
        m_cell = cell;
        m_columns = columns;
        Invalidate();
    }
}

void GameCovers::Invalidate() {
    m_dirty = true;
    updateGeometries();
    viewport()->update();
}

void GameCovers::EnsureLayout() const {
    if (!m_dirty) {
        return;
    }
    m_dirty = false;
    std::vector<Yuzu::ShelfEntry> entries;
    if (model() != nullptr) {
        QSet<int> hidden_rows;
        for (const QPersistentModelIndex& index : m_hidden) {
            if (index.isValid()) {
                hidden_rows.insert(index.row());
            }
        }
        const int rows = model()->rowCount(rootIndex());
        entries.reserve(rows);
        for (int row = 0; row < rows; ++row) {
            const QModelIndex index = model()->index(row, 0, rootIndex());
            if (hidden_rows.contains(row)) {
                continue;
            }
            const u64 program_id = index.data(GameListItemPath::ProgramIdRole).toULongLong();
            entries.push_back({row, QtCommon::IsSwitch2Program(program_id)});
        }
    }
    m_shelf = Yuzu::ComputeShelf(entries, m_columns, m_cell.width(), m_cell.height(), m_sections);
    m_spot_of_row.clear();
    for (int spot = 0; spot < static_cast<int>(m_shelf.spots.size()); ++spot) {
        m_spot_of_row.insert(m_shelf.spots[spot].row, spot);
    }
}

QRect GameCovers::CellRect(int row) const {
    EnsureLayout();
    const auto it = m_spot_of_row.constFind(row);
    if (it == m_spot_of_row.cend()) {
        return {};
    }
    const Yuzu::ShelfSpot& spot = m_shelf.spots[*it];
    return {QPoint{spot.x, spot.y}, m_cell};
}

int GameCovers::SpotOf(const QModelIndex& index) const {
    if (!index.isValid() || index.parent() != rootIndex()) {
        return -1;
    }
    EnsureLayout();
    return m_spot_of_row.value(index.row(), -1);
}

QModelIndex GameCovers::IndexOfSpot(int spot) const {
    EnsureLayout();
    if (spot < 0 || spot >= static_cast<int>(m_shelf.spots.size()) || model() == nullptr) {
        return {};
    }
    return model()->index(m_shelf.spots[spot].row, 0, rootIndex());
}

int GameCovers::SpotInRow(int spot, int direction) const {
    EnsureLayout();
    const auto& spots = m_shelf.spots;
    if (spot < 0 || spot >= static_cast<int>(spots.size())) {
        return -1;
    }
    const Yuzu::ShelfSpot& from = spots[spot];
    // The nearest row in that direction, then the game in it closest to the same column.
    int row_y = -1;
    for (const Yuzu::ShelfSpot& other : spots) {
        const bool beyond = direction < 0 ? other.y < from.y : other.y > from.y;
        if (beyond && (row_y < 0 || (direction < 0 ? other.y > row_y : other.y < row_y))) {
            row_y = other.y;
        }
    }
    if (row_y < 0) {
        return -1;
    }
    int best = -1;
    for (int i = 0; i < static_cast<int>(spots.size()); ++i) {
        if (spots[i].y == row_y &&
            (best < 0 || std::abs(spots[i].x - from.x) < std::abs(spots[best].x - from.x))) {
            best = i;
        }
    }
    return best;
}

QRect GameCovers::visualRect(const QModelIndex& index) const {
    if (!index.isValid() || index.parent() != rootIndex()) {
        return {};
    }
    const QRect cell = CellRect(index.row());
    return cell.isNull() ? QRect{} : cell.translated(0, -verticalOffset());
}

QModelIndex GameCovers::indexAt(const QPoint& point) const {
    EnsureLayout();
    const QPoint at = point + QPoint(0, verticalOffset());
    for (const Yuzu::ShelfSpot& spot : m_shelf.spots) {
        const QRect cell{QPoint{spot.x, spot.y}, m_cell};
        if (cell.contains(at)) {
            // Only the cover itself, not the gap around it.
            if (!m_delegate->CoverRect(cell).contains(at)) {
                return {};
            }
            return model()->index(spot.row, 0, rootIndex());
        }
    }
    return {};
}

void GameCovers::scrollTo(const QModelIndex& index, ScrollHint hint) {
    const int spot = SpotOf(index);
    if (spot < 0) {
        return;
    }
    QRect cell = CellRect(index.row());
    // The first games of a section bring their heading into view too.
    for (const Yuzu::ShelfHeading& heading : m_shelf.headings) {
        if (heading.y + Yuzu::ShelfHeadingHeight == cell.top()) {
            cell.setTop(heading.y);
        }
    }
    QScrollBar* const bar = verticalScrollBar();
    const int height = viewport()->height();
    switch (hint) {
    case PositionAtTop:
        bar->setValue(cell.top());
        break;
    case PositionAtBottom:
        bar->setValue(cell.bottom() + 1 - height);
        break;
    case PositionAtCenter:
        bar->setValue(cell.center().y() - height / 2);
        break;
    case EnsureVisible:
        if (cell.top() < bar->value()) {
            bar->setValue(cell.top());
        } else if (cell.bottom() + 1 > bar->value() + height) {
            bar->setValue(cell.bottom() + 1 - height);
        }
        break;
    }
}

QModelIndex GameCovers::moveCursor(CursorAction action, Qt::KeyboardModifiers) {
    EnsureLayout();
    const int count = static_cast<int>(m_shelf.spots.size());
    if (count == 0) {
        return {};
    }
    const int current = SpotOf(currentIndex());
    if (current < 0) {
        return IndexOfSpot(0);
    }
    int next = current;
    switch (action) {
    case MoveLeft:
    case MovePrevious:
        next = std::max(0, current - 1);
        break;
    case MoveRight:
    case MoveNext:
        next = std::min(count - 1, current + 1);
        break;
    case MoveUp:
    case MoveDown: {
        const int found = SpotInRow(current, action == MoveUp ? -1 : 1);
        next = found < 0 ? current : found;
        break;
    }
    case MovePageUp:
    case MovePageDown: {
        const int rows = std::max(1, viewport()->height() / std::max(1, m_cell.height()));
        for (int i = 0; i < rows; ++i) {
            const int found = SpotInRow(next, action == MovePageUp ? -1 : 1);
            if (found < 0) {
                break;
            }
            next = found;
        }
        break;
    }
    case MoveHome:
        next = 0;
        break;
    case MoveEnd:
        next = count - 1;
        break;
    }
    return IndexOfSpot(next);
}

int GameCovers::horizontalOffset() const {
    return 0;
}

int GameCovers::verticalOffset() const {
    return verticalScrollBar()->value();
}

bool GameCovers::isIndexHidden(const QModelIndex& index) const {
    return SpotOf(index) < 0;
}

void GameCovers::setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags flags) {
    EnsureLayout();
    const QRect area = rect.normalized().translated(0, verticalOffset());
    QItemSelection selection;
    for (const Yuzu::ShelfSpot& spot : m_shelf.spots) {
        const QRect cell{QPoint{spot.x, spot.y}, m_cell};
        if (m_delegate->CoverRect(cell).intersects(area)) {
            const QModelIndex index = model()->index(spot.row, 0, rootIndex());
            selection.select(index, index);
        }
    }
    selectionModel()->select(selection, flags);
}

QRegion GameCovers::visualRegionForSelection(const QItemSelection& selection) const {
    QRegion region;
    for (const QModelIndex& index : selection.indexes()) {
        region += visualRect(index);
    }
    return region;
}

void GameCovers::paintEvent(QPaintEvent* event) {
    EnsureLayout();
    QPainter painter(viewport());
    const int offset = verticalOffset();
    const QRect exposed = event->rect().translated(0, offset);

    // The sections' headings: the platform and how many games, over a thin rule.
    QFont heading_font = font();
    heading_font.setBold(true);
    heading_font.setPointSizeF(heading_font.pointSizeF() * 1.25);
    for (const Yuzu::ShelfHeading& heading : m_shelf.headings) {
        const QRect area{0, heading.y, viewport()->width(), Yuzu::ShelfHeadingHeight};
        if (!area.intersects(exposed)) {
            continue;
        }
        // Lined up with the first column's covers.
        const int inset = m_delegate->CoverRect(QRect{QPoint{0, 0}, m_cell}).left();
        const QRect shown = area.translated(0, -offset).adjusted(inset, 0, -inset, -6);
        const QString name =
            heading.switch2 ? tr("Nintendo Switch 2") : tr("Nintendo Switch");
        painter.setPen(palette().color(QPalette::Text));
        painter.setFont(heading_font);
        painter.drawText(shown, Qt::AlignLeft | Qt::AlignBottom, name);
        const int name_width = QFontMetrics(heading_font).horizontalAdvance(name);
        painter.setFont(font());
        painter.setPen(palette().color(QPalette::PlaceholderText));
        painter.drawText(shown.adjusted(name_width + 12, 0, 0, 0), Qt::AlignLeft | Qt::AlignBottom,
                         heading.count == 1 ? tr("1 game")
                                            : tr("%1 games").arg(heading.count));
        painter.setPen(QPen(palette().color(QPalette::Mid), 1));
        painter.drawLine(shown.left(), shown.bottom() + 4, shown.right(), shown.bottom() + 4);
    }

    QStyleOptionViewItem option;
    initViewItemOption(&option);
    const QModelIndex current = currentIndex();
    for (const Yuzu::ShelfSpot& spot : m_shelf.spots) {
        const QRect cell{QPoint{spot.x, spot.y}, m_cell};
        if (!cell.intersects(exposed)) {
            continue;
        }
        const QModelIndex index = model()->index(spot.row, 0, rootIndex());
        option.rect = cell.translated(0, -offset);
        option.state &= ~(QStyle::State_Selected | QStyle::State_MouseOver |
                          QStyle::State_HasFocus);
        if (selectionModel() != nullptr && selectionModel()->isSelected(index)) {
            option.state |= QStyle::State_Selected;
        }
        if (index == m_hover) {
            option.state |= QStyle::State_MouseOver;
        }
        if (index == current && hasFocus()) {
            option.state |= QStyle::State_HasFocus;
        }
        itemDelegate()->paint(&painter, option, index);
    }
}

void GameCovers::resizeEvent(QResizeEvent* event) {
    QAbstractItemView::resizeEvent(event);
    UpdateCoverSize();
    updateGeometries();
}

void GameCovers::mouseMoveEvent(QMouseEvent* event) {
    const QPersistentModelIndex hovered = indexAt(event->position().toPoint());
    if (hovered != m_hover) {
        viewport()->update(visualRect(m_hover));
        m_hover = hovered;
        viewport()->update(visualRect(m_hover));
    }
    QAbstractItemView::mouseMoveEvent(event);
}

void GameCovers::leaveEvent(QEvent* event) {
    if (m_hover.isValid()) {
        viewport()->update(visualRect(m_hover));
        m_hover = QPersistentModelIndex{};
    }
    QAbstractItemView::leaveEvent(event);
}

void GameCovers::updateGeometries() {
    EnsureLayout();
    QScrollBar* const bar = verticalScrollBar();
    bar->setRange(0, std::max(0, m_shelf.height - viewport()->height()));
    bar->setPageStep(viewport()->height());
    bar->setSingleStep(std::max(1, m_cell.height() / 4));
    QAbstractItemView::updateGeometries();
}

void GameCovers::rowsInserted(const QModelIndex& parent, int start, int end) {
    QAbstractItemView::rowsInserted(parent, start, end);
    Invalidate();
}

void GameCovers::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) {
    QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
    Invalidate();
}

void GameCovers::dataChanged(const QModelIndex& top_left, const QModelIndex& bottom_right,
                             const QList<int>& roles) {
    QAbstractItemView::dataChanged(top_left, bottom_right, roles);
    Invalidate();
}

void GameCovers::reset() {
    m_hidden.clear();
    m_hover = QPersistentModelIndex{};
    QAbstractItemView::reset();
    Invalidate();
}

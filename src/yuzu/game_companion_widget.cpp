// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/game_companion_widget.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QFontMetrics>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLinearGradient>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QStyledItemDelegate>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <utility>

namespace {

QString Id(const GameCompanion::Entry& entry) {
    return QString::fromUtf8(entry.id.data(), static_cast<qsizetype>(entry.id.size()));
}

constexpr int SummaryRole = Qt::UserRole + 1;
constexpr int KindRole = Qt::UserRole + 2;

class CompanionHero final : public QWidget {
public:
    explicit CompanionHero(GameCompanionAppearance appearance_, QWidget* parent)
        : QWidget(parent), appearance{std::move(appearance_)} {
        setFixedHeight(150);
        setObjectName(QStringLiteral("companionHero"));
    }

    void SetLanguage(bool spanish_) {
        spanish = spanish_;
        setAccessibleName(GameCompanion::Localize(appearance.title, spanish) +
                          QStringLiteral(" — ") +
                          GameCompanion::Localize(appearance.subtitle, spanish));
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QRectF frame = rect().adjusted(1, 1, -1, -1);
        QLinearGradient gradient(frame.topLeft(), frame.bottomRight());
        gradient.setColorAt(0, appearance.forest.lighter(125));
        gradient.setColorAt(1, appearance.forest.darker(125));
        painter.setPen(Qt::NoPen);
        painter.setBrush(gradient);
        painter.drawRoundedRect(frame, 18, 18);

        painter.save();
        painter.setClipRect(frame);
        painter.setOpacity(0.13);
        painter.setPen(QPen(appearance.gold, 1));
        for (int radius : {90, 126, 162}) {
            painter.drawEllipse(QPointF(width() - 20, 28), radius, radius);
        }
        if (appearance.motif == GameCompanionAppearance::Motif::SlateRings) {
            // An abstract rune dial: two rings, an inner disc and eight ticks.
            painter.setOpacity(0.42);
            const QPointF center(width() - 58, 54);
            painter.setPen(QPen(appearance.gold, 2));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(center, 30, 30);
            painter.drawEllipse(center, 19, 19);
            painter.setBrush(appearance.gold);
            painter.drawEllipse(center, 6, 6);
            for (int tick = 0; tick < 8; ++tick) {
                painter.save();
                painter.translate(center);
                painter.rotate(tick * 45.0);
                painter.drawLine(QPointF(0, -34), QPointF(0, -40));
                painter.restore();
            }
        }
        if (appearance.motif == GameCompanionAppearance::Motif::ZonaiSpiral) {
            // An abstract emblem: a ring around a spiral of shrinking arcs.
            painter.setOpacity(0.45);
            const QPointF center(width() - 58, 54);
            painter.setPen(QPen(appearance.gold, 2));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(center, 32, 32);
            for (int turn = 0; turn < 4; ++turn) {
                const qreal radius = 24 - turn * 6;
                painter.drawArc(QRectF(center.x() - radius, center.y() - radius, radius * 2,
                                       radius * 2),
                                (turn * 90) * 16, 270 * 16);
            }
            painter.setBrush(appearance.gold);
            painter.drawEllipse(center, 3, 3);
        }
        if (appearance.motif == GameCompanionAppearance::Motif::Glitch) {
            // Rows of offset blocks, like a corrupted scanline.
            painter.setOpacity(0.5);
            painter.setPen(Qt::NoPen);
            painter.setBrush(appearance.gold);
            constexpr int rows[][3] = {{0, 34, 8}, {12, 18, 6}, {4, 46, 8}, {20, 26, 6},
                                       {8, 40, 8}, {28, 14, 6}};
            for (int row = 0; row < 6; ++row) {
                const qreal y = 24 + row * 13;
                painter.drawRect(QRectF(width() - 104 + rows[row][0], y, rows[row][1],
                                        rows[row][2]));
            }
        }
        if (appearance.motif == GameCompanionAppearance::Motif::GoldenTriangles) {
            painter.setOpacity(0.3);
            painter.setPen(Qt::NoPen);
            painter.setBrush(appearance.gold);
            const qreal x = width() - 77;
            const qreal y = 24;
            const auto triangle = [&painter](qreal left, qreal top) {
                QPolygonF shape;
                shape << QPointF(left + 20, top) << QPointF(left, top + 35)
                      << QPointF(left + 40, top + 35);
                painter.drawPolygon(shape);
            };
            triangle(x + 20, y);
            triangle(x, y + 36);
            triangle(x + 40, y + 36);
        }
        painter.restore();

        painter.setPen(appearance.gold.lighter(150));
        QFont eyebrow = font();
        eyebrow.setPixelSize(10);
        eyebrow.setBold(true);
        eyebrow.setLetterSpacing(QFont::AbsoluteSpacing, 2);
        painter.setFont(eyebrow);
        painter.drawText(QRect(22, 18, width() - 44, 19), Qt::AlignLeft | Qt::AlignVCenter,
                         GameCompanion::Localize(appearance.eyebrow, spanish).toUpper());

        QFont title_font = font();
        title_font.setPixelSize(25);
        title_font.setBold(true);
        painter.setFont(title_font);
        painter.setPen(QColor(QStringLiteral("#fff8e8")));
        painter.drawText(QRect(22, 47, width() - 44, 43), Qt::AlignLeft | Qt::AlignVCenter,
                         GameCompanion::Localize(appearance.title, spanish));

        QFont subtitle_font = font();
        subtitle_font.setPixelSize(12);
        painter.setFont(subtitle_font);
        painter.setPen(QColor(QStringLiteral("#e9dfc5")));
        painter.drawText(QRect(22, 94, width() - 50, 42), Qt::AlignLeft | Qt::TextWordWrap,
                         GameCompanion::Localize(appearance.subtitle, spanish));
    }

private:
    GameCompanionAppearance appearance;
    bool spanish{};
};

class CompanionCardDelegate final : public QStyledItemDelegate {
public:
    CompanionCardDelegate(GameCompanionAppearance appearance_, QObject* parent)
        : QStyledItemDelegate(parent), appearance{std::move(appearance_)} {}

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex&) const override {
        return {1, 78};
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        const bool selected = option.state & QStyle::State_Selected;
        const QRect card = option.rect.adjusted(4, 3, -4, -3);
        painter->setPen(QPen(selected ? appearance.gold.darker(120) : QColor("#d7cdb8"), 1));
        painter->setBrush(selected ? QColor("#fff9e9") : QColor("#faf7ef"));
        painter->drawRoundedRect(card, 10, 10);
        painter->setPen(Qt::NoPen);
        painter->setBrush(selected ? appearance.gold : QColor("#a9a78d"));
        painter->drawRoundedRect(QRect(card.left() + 1, card.top() + 9, 3, card.height() - 18),
                                 1.5, 1.5);

        const int left = card.left() + 16;
        const int available = card.width() - 30;
        QFont kind_font = option.font;
        kind_font.setPixelSize(10);
        kind_font.setBold(true);
        painter->setFont(kind_font);
        painter->setPen(appearance.forest);
        painter->drawText(QRect(left, card.top() + 8, available, 14), Qt::AlignLeft,
                          index.data(KindRole).toString().toUpper());

        QFont title_font = option.font;
        title_font.setBold(true);
        painter->setFont(title_font);
        painter->setPen(appearance.ink);
        const QFontMetrics title_metrics(title_font);
        painter->drawText(QRect(left, card.top() + 25, available, 20), Qt::AlignLeft,
                          title_metrics.elidedText(index.data(Qt::DisplayRole).toString(),
                                                   Qt::ElideRight, available));

        QFont summary_font = option.font;
        summary_font.setPixelSize(11);
        painter->setFont(summary_font);
        painter->setPen(QColor("#686352"));
        const QFontMetrics summary_metrics(summary_font);
        painter->drawText(QRect(left, card.top() + 47, available, 17), Qt::AlignLeft,
                          summary_metrics.elidedText(index.data(SummaryRole).toString(),
                                                     Qt::ElideRight, available));
        painter->restore();
    }

private:
    GameCompanionAppearance appearance;
};

} // namespace

GameCompanionAppearance GameCompanionAppearance::Ocarina() {
    return {{"Ocarina of Time", "Ocarina of Time"},
            {"Hyrule field guide", "Guía de Hyrule"},
            {"Places, creatures and legends of Hyrule", "Lugares, criaturas y leyendas de Hyrule"},
            {"Search Hyrule...", "Buscar en Hyrule..."},
            QColor("#193b35"), QColor("#d7b66b"), QColor("#f1eddf"), QColor("#292b24"),
            Motif::GoldenTriangles};
}

GameCompanionAppearance GameCompanionAppearance::BreathOfTheWild() {
    return {{"Breath of the Wild", "Breath of the Wild"},
            {"Hyrule field guide", "Guía de Hyrule"},
            {"Regions, people and secrets of a wild Hyrule",
             "Regiones, gentes y secretos de un Hyrule salvaje"},
            {"Search Hyrule...", "Buscar en Hyrule..."},
            QColor("#1d3946"), QColor("#7fcfd6"), QColor("#eef1ec"), QColor("#22292c"),
            Motif::SlateRings};
}

GameCompanionAppearance GameCompanionAppearance::TearsOfTheKingdom() {
    return {{"Tears of the Kingdom", "Tears of the Kingdom"},
            {"Hyrule field guide", "Guía de Hyrule"},
            {"The skies, the surface and the Depths of Hyrule",
             "Los cielos, la superficie y el Subsuelo de Hyrule"},
            {"Search Hyrule...", "Buscar en Hyrule..."},
            QColor("#1f3a30"), QColor("#8fdcae"), QColor("#eef1e8"), QColor("#232a24"),
            Motif::ZonaiSpiral};
}

GameCompanionAppearance GameCompanionAppearance::AxiomVerge() {
    return {{"Axiom Verge", "Axiom Verge"},
            {"Sudra field guide", "Guía de Sudra"},
            {"Areas, weapons, bosses and the story of Sudra",
             "Zonas, armas, jefes y la historia de Sudra"},
            {"Search Sudra...", "Buscar en Sudra..."},
            QColor("#2a1b39"), QColor("#e2559c"), QColor("#efedf2"), QColor("#25212b"),
            Motif::Glitch};
}

GameCompanionAppearance GameCompanionAppearance::AxiomVerge2() {
    return {{"Axiom Verge 2", "Axiom Verge 2"},
            {"Kiengir field guide", "Guía de Kiengir"},
            {"Lands, Arms, the Breach and the story of Kiengir",
             "Tierras, Arms, la Brecha y la historia de Kiengir"},
            {"Search Kiengir...", "Buscar en Kiengir..."},
            QColor("#1d2d4a"), QColor("#d9b25f"), QColor("#eff0ec"), QColor("#22252c"),
            Motif::Glitch};
}

GameCompanionAppearance GameCompanionAppearance::For(const GameCompanion::Game& game) {
    switch (game.program_id) {
    case GameCompanion::BreathOfTheWildProgramId:
        return BreathOfTheWild();
    case GameCompanion::TearsOfTheKingdomProgramId:
        return TearsOfTheKingdom();
    case GameCompanion::AxiomVergeProgramId:
        return AxiomVerge();
    case GameCompanion::AxiomVerge2ProgramId:
        return AxiomVerge2();
    default:
        return Ocarina();
    }
}

GameCompanionWidget::GameCompanionWidget(const GameCompanion::Game& game_, bool spanish_,
                                         QWidget* parent)
    : GameCompanionWidget(game_, spanish_, parent, GameCompanionAppearance::For(game_)) {}

GameCompanionWidget::GameCompanionWidget(const GameCompanion::Game& game_, bool spanish_,
                                         QWidget* parent, GameCompanionAppearance appearance_)
    : QWidget(parent), game{game_}, spanish{spanish_}, appearance{std::move(appearance_)} {
    setObjectName(QStringLiteral("companionRoot"));
    setAttribute(Qt::WA_StyledBackground);
    setStyleSheet(QStringLiteral(R"(
        QWidget#companionRoot { background: %1; }
        QLabel#companionIntro { color: #656b5d; font-size: 11px; }
        QLabel#companionResultsHeading { color: %2; font-size: 12px; font-weight: bold; }
        QWidget#companionContext {
            background: #e7ddc7; border: 1px solid #c6b281; border-radius: 10px;
        }
        QLabel#companionContextLabel { color: %2; font-weight: bold; font-size: 12px; }
        QLabel#companionContextSummary { color: #615d4f; font-size: 11px; }
        QPushButton#companionContextOpen {
            background: %3; color: %2; border: none; border-radius: 7px;
            padding: 6px 9px; font-weight: bold;
        }
        QPushButton#companionContextOpen:hover { background: #e4c981; }
        QPushButton#companionContextOpen:pressed {
            background: #a78541; color: #fff8e8; padding-top: 7px; padding-bottom: 5px;
        }
        QPushButton#companionContextOpen:disabled { background: #8f7851; color: #fff8e8; }
        QLineEdit#companionSearch, QComboBox#companionCategory {
            background: #fffcf4; color: %2; border: 1px solid #c8bda6;
            border-radius: 9px; padding: 8px 10px; selection-background-color: %3;
        }
        QLineEdit#companionSearch:focus, QComboBox#companionCategory:focus {
            border: 2px solid %3;
        }
        QCheckBox#companionSpoilers { color: %2; spacing: 6px; font-size: 11px; }
        QListWidget#companionResults { background: transparent; border: none; outline: none; }
        QTextBrowser#companionDetail {
            background: #fffcf4; color: %2; border: 1px solid #d7cdb8;
            border-radius: 12px; padding: 10px;
        }
        QToolButton#companionDetailClose {
            background: transparent; color: #686352; border: none; border-radius: 13px;
            font-size: 14px; font-weight: bold;
        }
        QToolButton#companionDetailClose:hover { background: #efe5cc; color: %2; }
        QToolButton#companionDetailClose:pressed { background: %3; color: %2; }
    )")
                      .arg(appearance.parchment.name(), appearance.ink.name(),
                           appearance.gold.name()));
    auto* const layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);
    hero = new CompanionHero(appearance, this);
    layout->addWidget(hero);
    intro = new QLabel(this);
    intro->setObjectName(QStringLiteral("companionIntro"));
    intro->setWordWrap(true);
    layout->addWidget(intro);

    context_panel = new QWidget(this);
    context_panel->setObjectName(QStringLiteral("companionContext"));
    context_panel->setAttribute(Qt::WA_StyledBackground);
    auto* const context_layout = new QHBoxLayout(context_panel);
    context_layout->setContentsMargins(11, 9, 11, 9);
    context_layout->setSpacing(8);
    auto* const context_text = new QVBoxLayout;
    context_text->setSpacing(2);
    context_label = new QLabel(context_panel);
    context_label->setObjectName(QStringLiteral("companionContextLabel"));
    context_label->setWordWrap(true);
    context_text->addWidget(context_label);
    context_summary = new QLabel(context_panel);
    context_summary->setObjectName(QStringLiteral("companionContextSummary"));
    context_summary->setWordWrap(true);
    context_text->addWidget(context_summary);
    context_layout->addLayout(context_text, 1);
    context_open = new QPushButton(context_panel);
    context_open->setObjectName(QStringLiteral("companionContextOpen"));
    context_layout->addWidget(context_open);
    context_panel->hide();
    layout->addWidget(context_panel);

    search = new QLineEdit(this);
    search->setObjectName(QStringLiteral("companionSearch"));
    layout->addWidget(search);

    auto* const filters = new QHBoxLayout;
    filters->setSpacing(8);
    category = new QComboBox(this);
    category->setObjectName(QStringLiteral("companionCategory"));
    filters->addWidget(category, 1);
    spoilers = new QCheckBox(this);
    spoilers->setObjectName(QStringLiteral("companionSpoilers"));
    filters->addWidget(spoilers);
    layout->addLayout(filters);

    results_heading = new QLabel(this);
    results_heading->setObjectName(QStringLiteral("companionResultsHeading"));
    layout->addWidget(results_heading);

    results = new QListWidget(this);
    results->setObjectName(QStringLiteral("companionResults"));
    results->setItemDelegate(new CompanionCardDelegate(appearance, results));
    results->setUniformItemSizes(true);
    results->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    results->setSpacing(2);
    results->setMinimumHeight(160);
    layout->addWidget(results, 2);

    // The close button sits over the card's top right corner, left of its scroll bar.
    detail_panel = new QWidget(this);
    detail_panel->setObjectName(QStringLiteral("companionDetailPanel"));
    auto* const detail_layout = new QGridLayout(detail_panel);
    detail_layout->setContentsMargins(0, 0, 0, 0);
    detail = new QTextBrowser(detail_panel);
    detail->setObjectName(QStringLiteral("companionDetail"));
    detail->setOpenExternalLinks(true);
    detail->setMinimumHeight(170);
    detail->setFrameShape(QFrame::NoFrame);
    detail_layout->addWidget(detail, 0, 0);
    detail_close = new QToolButton(detail_panel);
    detail_close->setObjectName(QStringLiteral("companionDetailClose"));
    detail_close->setText(QStringLiteral("✕"));
    detail_close->setFixedSize(26, 26);
    detail_close->setCursor(Qt::PointingHandCursor);
    auto* const close_row = new QHBoxLayout;
    close_row->setContentsMargins(0, 8, 8, 0);
    close_row->addStretch();
    close_row->addWidget(detail_close);
    detail_layout->addLayout(close_row, 0, 0, Qt::AlignTop);
    connect(detail->verticalScrollBar(), &QScrollBar::rangeChanged, this,
            [this, close_row](int, int maximum) {
                const int bar = maximum > 0 ? detail->verticalScrollBar()->sizeHint().width() : 0;
                close_row->setContentsMargins(0, 8, 8 + bar, 0);
            });
    detail_panel->hide();
    layout->addWidget(detail_panel, 2);

    connect(search, &QLineEdit::textChanged, this, &GameCompanionWidget::RefreshList);
    connect(category, &QComboBox::currentIndexChanged, this, &GameCompanionWidget::RefreshList);
    connect(spoilers, &QCheckBox::toggled, this, &GameCompanionWidget::RefreshList);
    connect(results, &QListWidget::currentItemChanged, this, &GameCompanionWidget::RefreshDetail);
    connect(detail_close, &QToolButton::clicked, this, &GameCompanionWidget::CloseEntry);
    connect(context_open, &QPushButton::clicked, this, [this] {
        if (!scene_entry || (scene_entry->spoiler && !spoilers->isChecked())) {
            return;
        }
        search->clear();
        category->setCurrentIndex(0);
        SelectEntry(Id(*scene_entry));
        dismissed_scene = detected_scene;
        context_open->setText(tr("Opened"));
        context_open->setEnabled(false);
        QTimer::singleShot(240, this, [this, scene = detected_scene] {
            if (detected_scene == scene && dismissed_scene == scene) {
                RefreshContext();
            }
        });
    });
    SetLanguage(spanish);
}

void GameCompanionWidget::SetLanguage(bool spanish_) {
    spanish = spanish_;
    static_cast<CompanionHero*>(hero)->SetLanguage(spanish);
    intro->setText(GameCompanion::Localize(game.intro, spanish));
    search->setPlaceholderText(GameCompanion::Localize(appearance.search_hint, spanish));
    spoilers->setText(tr("Show story spoilers"));
    context_open->setText(tr("Open card"));
    detail_close->setToolTip(tr("Close card"));
    detail_close->setAccessibleName(tr("Close card"));
    const auto selected = static_cast<GameCompanion::Kind>(category->currentData().toInt());
    const QSignalBlocker blocker(category);
    category->clear();
    for (const auto kind : game.kinds) {
        category->addItem(KindName(kind), static_cast<int>(kind));
    }
    category->setCurrentIndex(category->findData(static_cast<int>(selected)));
    RefreshList();
}

void GameCompanionWidget::SetDetectedScene(QString scene) {
    if (scene == detected_scene) {
        return;
    }
    detected_scene = std::move(scene);
    dismissed_scene.clear();
    scene_entry = GameCompanion::EntryForScene(game, detected_scene.toStdString());
    RefreshContext();
}

void GameCompanionWidget::RefreshContext() {
    const bool visible = scene_entry && detected_scene != dismissed_scene &&
                         (!scene_entry->spoiler || spoilers->isChecked());
    context_panel->setVisible(visible);
    if (!visible) {
        context_label->clear();
        context_summary->clear();
        return;
    }
    context_open->setEnabled(true);
    context_open->setText(tr("Open card"));
    context_label->setText(tr("Recently loaded: %1")
                               .arg(GameCompanion::Localize(scene_entry->title, spanish)));
    context_summary->setText(GameCompanion::Localize(scene_entry->summary, spanish));
}

QString GameCompanionWidget::KindName(GameCompanion::Kind kind) const {
    switch (kind) {
    case GameCompanion::Kind::All:
        return tr("All topics");
    case GameCompanion::Kind::Place:
        return tr("Places");
    case GameCompanion::Kind::Character:
        return tr("Characters");
    case GameCompanion::Kind::Enemy:
        return tr("Enemies");
    case GameCompanion::Kind::Boss:
        return tr("Bosses");
    case GameCompanion::Kind::Dungeon:
        return tr("Dungeons");
    case GameCompanion::Kind::Item:
        return GameCompanion::Localize(game.items_topic, spanish);
    case GameCompanion::Kind::Ability:
        return game.abilities_topic.en.empty()
                   ? tr("Runes and abilities")
                   : GameCompanion::Localize(game.abilities_topic, spanish);
    case GameCompanion::Kind::Guide:
        return tr("Tips and mechanics");
    case GameCompanion::Kind::Remake:
        return tr("Switch 2 changes");
    case GameCompanion::Kind::Timeline:
        return game.timeline_topic.en.empty()
                   ? tr("Zelda timeline")
                   : GameCompanion::Localize(game.timeline_topic, spanish);
    }
    return {};
}

void GameCompanionWidget::RefreshList() {
    const QString previous = CurrentEntryId();
    const auto kind = static_cast<GameCompanion::Kind>(category->currentData().toInt());
    const auto matches = GameCompanion::Search(game, search->text(), kind, spoilers->isChecked());
    const QSignalBlocker blocker(results);
    results->clear();
    int previous_row = -1;
    for (const auto* entry : matches) {
        auto* const item = new QListWidgetItem(GameCompanion::Localize(entry->title, spanish),
                                               results);
        item->setData(Qt::UserRole, Id(*entry));
        item->setData(SummaryRole, GameCompanion::Localize(entry->summary, spanish));
        item->setData(KindRole, KindName(entry->kind));
        if (item->data(Qt::UserRole).toString() == previous) {
            previous_row = results->count() - 1;
        }
    }
    // Nothing opens by itself: the card stays closed until the player picks a topic.
    results->setCurrentRow(previous_row);
    results_heading->setText(results->count() > 0
                                 ? tr("Explore · %1 entries").arg(results->count())
                                 : tr("No matching topic. Try another search or category."));
    RefreshContext();
    RefreshDetail();
}

void GameCompanionWidget::RefreshDetail() {
    const auto* const entry = GameCompanion::FindEntry(game, CurrentEntryId().toStdString());
    if (!entry || (entry->spoiler && !spoilers->isChecked())) {
        detail->clear();
        detail_panel->hide();
        return;
    }
    detail_panel->show();
    const QString title = GameCompanion::Localize(entry->title, spanish).toHtmlEscaped();
    const QString summary = GameCompanion::Localize(entry->summary, spanish).toHtmlEscaped();
    const QString kind = KindName(entry->kind).toUpper().toHtmlEscaped();
    QString extra;
    if (!entry->detail.en.empty()) {
        QString body = GameCompanion::Localize(entry->detail, spanish).toHtmlEscaped();
        body.replace(QStringLiteral("\n\n"), QStringLiteral("</p><p>"));
        body.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
        extra = QStringLiteral("<p>%1</p>").arg(body);
    }
    const QString source = QString::fromUtf8(entry->source_url.data(), entry->source_url.size());
    detail->setHtml(
        QStringLiteral(R"(<html><body style="color:%1; font-size:12px; line-height:1.3">
            <p style="color:%2; font-size:10px; font-weight:bold; margin:4px 0 9px">%3</p>
            <h2 style="color:%4; font-size:20px; margin:0 0 11px">%5</h2>
            <p style="font-size:13px; font-weight:bold; margin:0 0 14px">%6</p>
            <div style="border-top:1px solid #ddd2bb; margin:10px 0"></div>
            %7
            <p style="margin-top:18px"><a style="color:%4; font-weight:bold" href="%8">%9 ↗</a></p>
        </body></html>)")
            .arg(appearance.ink.name(), appearance.gold.darker(145).name(), kind,
                 appearance.forest.name(), title, summary, extra, source.toHtmlEscaped(),
                 tr("Read source")));
    detail->moveCursor(QTextCursor::Start);
}

void GameCompanionWidget::SelectEntry(QString id) {
    for (int row = 0; row < results->count(); ++row) {
        if (results->item(row)->data(Qt::UserRole).toString() == id) {
            results->setCurrentRow(row);
            return;
        }
    }
}

void GameCompanionWidget::CloseEntry() {
    results->setCurrentRow(-1);
}

QString GameCompanionWidget::CurrentEntryId() const {
    const auto* const item = results->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString{};
}

void GameCompanionWidget::FocusSearch() {
    search->setFocus();
}

void GameCompanionWidget::HideSpoilers() {
    spoilers->setChecked(false);
}

GameCompanionDock::GameCompanionDock(QWidget* parent) : QDockWidget(parent) {
    setObjectName(QStringLiteral("GameCompanionDock"));
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
}

void GameCompanionDock::closeEvent(QCloseEvent* event) {
    QDockWidget::closeEvent(event);
    if (event->isAccepted()) {
        emit ClosedByUser();
    }
}

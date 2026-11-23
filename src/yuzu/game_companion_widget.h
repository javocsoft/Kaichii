// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDockWidget>
#include <QColor>
#include <QWidget>

#include "yuzu/game_companion_catalog.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTextBrowser;
class QToolButton;
class QCloseEvent;

struct GameCompanionAppearance {
    enum class Motif { None, GoldenTriangles };

    GameCompanion::Text title;
    GameCompanion::Text eyebrow;
    GameCompanion::Text subtitle;
    GameCompanion::Text search_hint;
    QColor forest;
    QColor gold;
    QColor parchment;
    QColor ink;
    Motif motif{Motif::None};

    [[nodiscard]] static GameCompanionAppearance Ocarina();
};

class GameCompanionWidget final : public QWidget {
    Q_OBJECT

public:
    explicit GameCompanionWidget(bool spanish, QWidget* parent = nullptr,
                                 GameCompanionAppearance appearance = GameCompanionAppearance::Ocarina());
    void SetLanguage(bool spanish);
    void SelectEntry(QString id);
    [[nodiscard]] QString CurrentEntryId() const;
    void FocusSearch();
    void HideSpoilers();
    void SetDetectedScene(QString scene);
    void CloseEntry();

private:
    void RefreshList();
    void RefreshDetail();
    void RefreshContext();
    [[nodiscard]] QString KindName(GameCompanion::Kind kind) const;

    bool spanish;
    GameCompanionAppearance appearance;
    QWidget* hero;
    QWidget* context_panel;
    QLabel* context_label;
    QLabel* context_summary;
    QPushButton* context_open;
    QString detected_scene;
    QString dismissed_scene;
    const GameCompanion::Entry* scene_entry{};
    QLabel* results_heading;
    QLabel* intro;
    QLineEdit* search;
    QComboBox* category;
    QCheckBox* spoilers;
    QListWidget* results;
    QWidget* detail_panel;
    QTextBrowser* detail;
    QToolButton* detail_close;
};

class GameCompanionDock final : public QDockWidget {
    Q_OBJECT

public:
    explicit GameCompanionDock(QWidget* parent = nullptr);

signals:
    void ClosedByUser();

protected:
    void closeEvent(QCloseEvent* event) override;
};

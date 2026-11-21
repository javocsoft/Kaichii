// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>

#include <QDialog>

#include "yuzu/compatibility_report.h"

class AiProviderPanel;
class AiRequest;
class ButtonSpinner;
class QLabel;
class QNetworkAccessManager;
class QPlainTextEdit;
class QPushButton;
struct AiReply;

/// Writes a compatibility report from the player's own words, in any language: the AI turns
/// them into the report's summary, progress, details and steps, in English, for the player to
/// review in the report before anything goes to GitHub.
class AiReportDraftDialog final : public QDialog {
    Q_OBJECT

public:
    AiReportDraftDialog(const CompatibilityReporting::Session& session, QWidget* parent);

    [[nodiscard]] std::optional<CompatibilityReporting::Draft> Result() const;

private:
    void Write();
    void HandleReply(const AiReply& reply);

    CompatibilityReporting::Session session;
    std::optional<CompatibilityReporting::Draft> result;
    QNetworkAccessManager* network;
    AiProviderPanel* provider_panel;
    AiRequest* request;
    QPlainTextEdit* description;
    QPushButton* write_button;
    ButtonSpinner* write_spinner;
    QLabel* status;
};

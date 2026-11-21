// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

/// What a request to the AI provider came back with.
struct AiReply {
    enum class Status {
        Ok,           ///< `result` holds the JSON object the schema asked for
        Cancelled,    ///< The user cancelled it
        NetworkError, ///< The request failed; `message` says why
        Refused,      ///< The provider declined it
        Truncated,    ///< The provider stopped before finishing
        Invalid,      ///< The reply was empty or not the JSON object asked for
    };
    Status status = Status::Invalid;
    QJsonObject result;
    QString message;
};

/// One request at a time to OpenAI (Responses API) or Anthropic (Messages API), asking for a JSON
/// object that follows `schema`. Kaichii talks to the providers over HTTP: there is no C++ SDK.
class AiRequest final : public QObject {
    Q_OBJECT

public:
    struct Parameters {
        bool anthropic = false;
        QString model;
        QString key;
        QString instructions; ///< The system prompt
        QString input;        ///< The user's message: data, never instructions
        QJsonObject schema;
        QString schema_name;
    };

    AiRequest(QNetworkAccessManager* network, QObject* parent);
    ~AiRequest() override;

    void Send(const Parameters& parameters);
    void Cancel();
    [[nodiscard]] bool Running() const;

    /// A sentence for the user about a reply that isn't Ok.
    [[nodiscard]] static QString Describe(const AiReply& reply);

signals:
    void Finished(const AiReply& reply);

private:
    void HandleReply(QNetworkReply* reply, bool anthropic);

    QNetworkAccessManager* network;
    QNetworkReply* active = nullptr;
};

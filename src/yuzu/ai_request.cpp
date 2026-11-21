// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/ai_request.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include "common/ai_optimize_policy.h"

namespace {

QString ResponseText(const QJsonObject& response, bool anthropic) {
    if (anthropic) {
        for (const QJsonValue item : response.value(QStringLiteral("content")).toArray()) {
            const auto block = item.toObject();
            if (block.value(QStringLiteral("type")).toString() == QLatin1String("text")) {
                return block.value(QStringLiteral("text")).toString();
            }
        }
        return {};
    }
    for (const QJsonValue item : response.value(QStringLiteral("output")).toArray()) {
        for (const QJsonValue content : item.toObject().value(QStringLiteral("content")).toArray()) {
            const auto block = content.toObject();
            if (block.value(QStringLiteral("type")).toString() == QLatin1String("output_text")) {
                return block.value(QStringLiteral("text")).toString();
            }
        }
    }
    return {};
}

} // Anonymous namespace

AiRequest::AiRequest(QNetworkAccessManager* network_, QObject* parent)
    : QObject(parent), network{network_} {}

AiRequest::~AiRequest() {
    if (active) {
        active->disconnect(this);
        active->abort();
    }
}

bool AiRequest::Running() const {
    return active != nullptr;
}

void AiRequest::Cancel() {
    if (active) {
        active->abort(); // Ends in HandleReply with OperationCanceledError
    }
}

void AiRequest::Send(const Parameters& p) {
    if (active) {
        return;
    }
    QJsonObject body;
    QNetworkRequest request(p.anthropic
                                ? QUrl{QStringLiteral("https://api.anthropic.com/v1/messages")}
                                : QUrl{QStringLiteral("https://api.openai.com/v1/responses")});
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    // Current models think before they answer; give them time.
    request.setTransferTimeout(120000);
    if (p.anthropic) {
        request.setRawHeader("x-api-key", p.key.toUtf8());
        request.setRawHeader("anthropic-version", "2026-10-21");
        body = {{QStringLiteral("model"), p.model},
                // Thinking can't be turned off on the current models and counts against this.
                {QStringLiteral("max_tokens"), 16000},
                {QStringLiteral("system"), p.instructions},
                {QStringLiteral("messages"),
                 QJsonArray{QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                                        {QStringLiteral("content"), p.input}}}},
                {QStringLiteral("output_config"),
                 QJsonObject{{QStringLiteral("effort"), QStringLiteral("low")},
                             {QStringLiteral("format"),
                              QJsonObject{{QStringLiteral("type"), QStringLiteral("json_schema")},
                                          {QStringLiteral("schema"), p.schema}}}}}};
        // If a safety classifier declines, the API retries on a model it picks.
        if (Common::AiOptimize::SupportsRefusalFallback(p.model.toStdString())) {
            request.setRawHeader("anthropic-beta", "server-side-fallback-2026-07-01");
            body.insert(QStringLiteral("fallbacks"), QStringLiteral("default"));
        }
    } else {
        request.setRawHeader("Authorization", "Bearer " + p.key.toUtf8());
        body = {{QStringLiteral("model"), p.model},
                {QStringLiteral("input"),
                 QJsonArray{QJsonObject{{QStringLiteral("role"), QStringLiteral("system")},
                                        {QStringLiteral("content"), p.instructions}},
                            QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                                        {QStringLiteral("content"), p.input}}}},
                {QStringLiteral("store"), false},
                {QStringLiteral("max_output_tokens"), 16000},
                {QStringLiteral("text"),
                 QJsonObject{{QStringLiteral("format"),
                              QJsonObject{{QStringLiteral("type"), QStringLiteral("json_schema")},
                                          {QStringLiteral("name"), p.schema_name},
                                          {QStringLiteral("strict"), true},
                                          {QStringLiteral("schema"), p.schema}}}}}};
    }
    active = network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(active, &QNetworkReply::finished, this,
            [this, reply = active, anthropic = p.anthropic] { HandleReply(reply, anthropic); });
}

void AiRequest::HandleReply(QNetworkReply* reply, bool anthropic) {
    active = nullptr;
    const QByteArray bytes = reply->readAll();
    const auto error = reply->error();
    const QString error_text = reply->errorString();
    reply->deleteLater();
    AiReply result;
    const QJsonObject response = QJsonDocument::fromJson(bytes).object();
    if (error == QNetworkReply::OperationCanceledError) {
        result.status = AiReply::Status::Cancelled;
    } else if (error != QNetworkReply::NoError) {
        result.status = AiReply::Status::NetworkError;
        const QString provider_error =
            response.value(QStringLiteral("error")).toObject().value(QStringLiteral("message")).toString();
        result.message = provider_error.isEmpty() ? error_text : provider_error.left(300);
    } else {
        const QString stop =
            response.value(anthropic ? QStringLiteral("stop_reason") : QStringLiteral("status"))
                .toString();
        if (stop == QLatin1String("refusal")) {
            result.status = AiReply::Status::Refused;
        } else if (stop == QLatin1String("max_tokens") || stop == QLatin1String("incomplete")) {
            result.status = AiReply::Status::Truncated;
        } else {
            const QJsonDocument document =
                QJsonDocument::fromJson(ResponseText(response, anthropic).toUtf8());
            if (document.isObject()) {
                result.status = AiReply::Status::Ok;
                result.result = document.object();
            }
        }
    }
    emit Finished(result);
}

QString AiRequest::Describe(const AiReply& reply) {
    switch (reply.status) {
    case AiReply::Status::Ok:
        return {};
    case AiReply::Status::Cancelled:
        return tr("Request cancelled. Nothing was changed.");
    case AiReply::Status::NetworkError:
        return tr("The provider could not complete the request: %1").arg(reply.message);
    case AiReply::Status::Refused:
        return tr("The provider declined this request. Try rewording the description.");
    case AiReply::Status::Truncated:
        return tr("The provider stopped before finishing its answer. Try again, or pick another "
                  "model.");
    case AiReply::Status::Invalid:
        return tr("The provider returned no usable answer. Nothing was changed.");
    }
    return {};
}

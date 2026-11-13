// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>

#include <QDialog>
#include <QList>
#include <QMap>
#include <QString>
#include <QUrl>

class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTabWidget;

namespace CompatibilityReporting {

struct Addon {
    QString name;
    QString version;
    bool enabled;
};

QString SummarizeEnabledAddons(const QList<Addon>& addons);

struct Session {
    quint64 program_id{};
    QString title;
    QString build;
    QString game_version;
    QString cpu;
    QString gpu;
    QString driver;
    QString os;
    QString ram;
    QString graphics_api;
    QString settings;
    QString mods;
    QString updates;
};

/// The issue form the reports fill in, in .github/ISSUE_TEMPLATE, and its report field.
inline constexpr char IssueTemplate[] = "compatibility_report.yml";
inline constexpr char ReportField[] = "report";

struct IssueLink {
    QUrl url;
    /// False when the report was too long for a link and was left out of it.
    bool report_included{};
};

/// The GitHub page that opens a new compatibility report on `repository` ("owner/name") with
/// `title` and `report` filled in. A report too long for a link is left out. An invalid
/// repository gives an empty URL.
[[nodiscard]] IssueLink NewIssueLink(const QString& repository, const QString& title,
                                     const QString& report);

/// Prepares a compatibility report for the last session and opens it on GitHub, where the player
/// reviews and publishes it. Kaichii itself sends nothing.
class Dialog final : public QDialog {
    Q_OBJECT

public:
    /// Opens a URL in the browser; the tests replace it.
    using UrlOpener = std::function<bool(const QUrl&)>;

    Dialog(const Session& session, const QString& repository, QWidget* parent = nullptr,
           UrlOpener open_url = {});
    ~Dialog() override;

    /// The report as Markdown, as it goes to GitHub.
    [[nodiscard]] QString Markdown() const;
    /// The issue's title.
    [[nodiscard]] QString Title() const;

private:
    friend class DialogTest;
    void Open();
    void Copy();
    void Clear();
    void Refresh();
    [[nodiscard]] QString ValidationError() const;
    [[nodiscard]] QString Text(const QString& key) const;

    QString repository;
    UrlOpener open_url;
    quint64 program_id;
    QMap<QString, QLineEdit*> fields;
    QMap<QString, QPlainTextEdit*> paragraphs;
    QComboBox* outcome;
    QComboBox* graphics_api;
    QPushButton* open;
    QPushButton* copy;
    QPushButton* clear;
    QLabel* status;
    QTabWidget* tabs;
    QPlainTextEdit* preview;
};

} // namespace CompatibilityReporting

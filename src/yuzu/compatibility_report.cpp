// SPDX-License-Identifier: GPL-3.0-or-later

#include "compatibility_report.h"

#include <QClipboard>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTabWidget>
#include <QUrlQuery>
#include <QVBoxLayout>

namespace CompatibilityReporting {
namespace {

const QMap<QString, int> Limits{
    {QStringLiteral("title"), 150},
    {QStringLiteral("build"), 100},
    {QStringLiteral("gameVersion"), 60},
    {QStringLiteral("cpu"), 160},
    {QStringLiteral("gpu"), 160},
    {QStringLiteral("driver"), 160},
    {QStringLiteral("os"), 160},
    {QStringLiteral("ram"), 60},
    {QStringLiteral("settings"), 3000},
    {QStringLiteral("progress"), 500},
    {QStringLiteral("summary"), 120},
    {QStringLiteral("details"), 6000},
    {QStringLiteral("reproduction"), 3000},
    {QStringLiteral("updates"), 500},
    {QStringLiteral("mods"), 500},
};
const QStringList OptionalFields{QStringLiteral("ram"), QStringLiteral("reproduction"),
                                 QStringLiteral("updates"), QStringLiteral("mods")};

/// GitHub answers links much longer than this with an error page.
constexpr qsizetype LinkLimit = 7000;

QString Known(const QString& value) {
    return value.trimmed().isEmpty() ? QStringLiteral("Unknown") : value.trimmed();
}

bool HasControls(const QString& value) {
    for (const auto ch : value) {
        if (ch.category() == QChar::Other_Control && ch != QLatin1Char('\t') &&
            ch != QLatin1Char('\n') && ch != QLatin1Char('\r')) {
            return true;
        }
    }
    return false;
}

QString ProgramId(quint64 program_id) {
    return QStringLiteral("%1").arg(program_id, 16, 16, QLatin1Char('0')).toUpper();
}

/// A value inside a Markdown table cell: pipes escaped, lines joined.
QString Cell(const QString& value) {
    QString cell = value.trimmed();
    cell.replace(QLatin1Char('|'), QStringLiteral("\\|"));
    cell.replace(QStringLiteral("\r\n"), QStringLiteral("<br>"));
    cell.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
    return cell.isEmpty() ? QStringLiteral("Not given") : cell;
}

} // namespace

QString SummarizeEnabledAddons(const QList<Addon>& addons) {
    QStringList labels;
    for (const auto& addon : addons) {
        if (!addon.enabled || addon.name.trimmed().isEmpty()) {
            continue;
        }
        auto label = addon.name.trimmed();
        if (!addon.version.trimmed().isEmpty()) {
            label += QStringLiteral(" (%1)").arg(addon.version.trimmed());
        }
        if (!labels.contains(label)) {
            labels.append(std::move(label));
        }
    }
    if (labels.isEmpty()) {
        return QStringLiteral("None listed as enabled at launch.");
    }
    const QString summary =
        QStringLiteral("Enabled at launch: ") + labels.join(QStringLiteral("; "));
    constexpr int limit = 500;
    if (summary.size() <= limit) {
        return summary;
    }
    const auto suffix = QStringLiteral("… (list truncated; review)");
    auto shortened = summary.left(limit - suffix.size());
    if (!shortened.isEmpty() && shortened.back().isHighSurrogate()) {
        shortened.chop(1);
    }
    return shortened + suffix;
}

IssueLink NewIssueLink(const QString& repository, const QString& title, const QString& report) {
    static const QRegularExpression repository_pattern(
        QStringLiteral("^[A-Za-z0-9-]+/[A-Za-z0-9._-]+$"));
    if (!repository_pattern.match(repository).hasMatch()) {
        return {};
    }
    const auto link = [&](const QString& body) {
        QUrl url(QStringLiteral("https://github.com/%1/issues/new").arg(repository));
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("template"), QString::fromLatin1(IssueTemplate));
        query.addQueryItem(QStringLiteral("title"),
                           QString::fromLatin1(QUrl::toPercentEncoding(title)));
        query.addQueryItem(QString::fromLatin1(ReportField),
                           QString::fromLatin1(QUrl::toPercentEncoding(body)));
        url.setQuery(query);
        return url;
    };
    const QUrl full = link(report);
    if (full.toEncoded().size() <= LinkLimit) {
        return {full, true};
    }
    return {link(QStringLiteral("Kaichii copied the full report to your clipboard: paste it "
                                "here, in place of this line.")),
            false};
}

Dialog::Dialog(const Session& session, const QString& repository_, QWidget* parent,
               UrlOpener open_url_)
    : QDialog(parent), repository{repository_}, open_url{std::move(open_url_)},
      program_id{session.program_id} {
    if (!open_url) {
        open_url = [](const QUrl& url) { return QDesktopServices::openUrl(url); };
    }
    setWindowTitle(tr("Submit compatibility report"));
    resize(760, 760);
    auto* layout = new QVBoxLayout(this);
    auto* introduction = new QLabel(
        tr("Tell how the game ran in your last session. Open on GitHub shows the report on a new "
           "issue in Kaichii's repository, where you review it and publish it with your GitHub "
           "account. Kaichii sends nothing by itself, and logs are never included."),
        this);
    introduction->setWordWrap(true);
    layout->addWidget(introduction);

    tabs = new QTabWidget(this);
    layout->addWidget(tabs, 1);
    const auto add_page = [this](const QString& title) {
        auto* content = new QWidget;
        auto* form = new QFormLayout(content);
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        auto* scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        scroll->setWidget(content);
        tabs->addTab(scroll, title);
        return form;
    };
    const auto line = [this](QFormLayout* form, const QString& key, const QString& label,
                             const QString& value = QString{}) {
        auto* edit = new QLineEdit(value, this);
        edit->setObjectName(key);
        edit->setMaxLength(Limits.value(key));
        fields.insert(key, edit);
        form->addRow(label, edit);
    };
    const auto paragraph = [this](QFormLayout* form, const QString& key, const QString& label,
                                  const QString& value = QString{}) {
        auto* edit = new QPlainTextEdit(value, this);
        edit->setObjectName(key);
        edit->setMinimumHeight(90);
        paragraphs.insert(key, edit);
        form->addRow(label, edit);
    };

    auto* experience = add_page(tr("Experience"));
    outcome = new QComboBox(this);
    outcome->setObjectName(QStringLiteral("outcome"));
    outcome->addItem(tr("Choose your assessment"), QString{});
    outcome->addItem(tr("Perfect: plays as on the console"), QStringLiteral("Perfect"));
    outcome->addItem(tr("Playable: can be played through, with small problems"),
                     QStringLiteral("Playable"));
    outcome->addItem(tr("Runs: gets into the game, but problems get in the way"),
                     QStringLiteral("Runs"));
    outcome->addItem(tr("Loads: starts, but doesn't get into the game"), QStringLiteral("Loads"));
    outcome->addItem(tr("Unplayable: crashes or doesn't start"), QStringLiteral("Unplayable"));
    experience->addRow(tr("Compatibility"), outcome);
    line(experience, QStringLiteral("summary"), tr("Summary"));
    paragraph(experience, QStringLiteral("progress"), tr("How far did you play?"));
    paragraph(experience, QStringLiteral("details"), tr("What worked or failed?"));
    paragraph(experience, QStringLiteral("reproduction"), tr("Steps to reproduce (optional)"));

    auto* setup = add_page(tr("System and settings"));
    auto* setup_notice =
        new QLabel(tr("Prefilled values describe launch settings, not measured "
                      "performance. Review changes made during play. Unknown means "
                      "not collected. Do not paste logs, paths, keys or private details."));
    setup_notice->setWordWrap(true);
    setup->addRow(setup_notice);
    line(setup, QStringLiteral("title"), tr("Game"), session.title.trimmed());
    setup->addRow(tr("Program ID"), new QLabel(ProgramId(program_id), this));
    line(setup, QStringLiteral("gameVersion"), tr("Game version"), Known(session.game_version));
    line(setup, QStringLiteral("build"), tr("Kaichii build"), Known(session.build));
    line(setup, QStringLiteral("cpu"), tr("CPU"), Known(session.cpu));
    line(setup, QStringLiteral("gpu"), tr("GPU"), Known(session.gpu));
    line(setup, QStringLiteral("driver"), tr("GPU driver / OpenGL version"), Known(session.driver));
    line(setup, QStringLiteral("os"), tr("Operating system"), Known(session.os));
    line(setup, QStringLiteral("ram"), tr("RAM (optional)"), session.ram.trimmed());
    graphics_api = new QComboBox(this);
    graphics_api->setObjectName(QStringLiteral("graphicsApi"));
    graphics_api->addItems(
        {QStringLiteral("Unknown"), QStringLiteral("Vulkan"), QStringLiteral("OpenGL")});
    graphics_api->setCurrentText(Known(session.graphics_api));
    setup->addRow(tr("Graphics API"), graphics_api);
    paragraph(setup, QStringLiteral("settings"), tr("Launch settings"), Known(session.settings));
    paragraph(setup, QStringLiteral("updates"), tr("Updates (optional)"), session.updates);
    paragraph(setup, QStringLiteral("mods"), tr("Mods (optional)"), session.mods);

    preview = new QPlainTextEdit(this);
    preview->setObjectName(QStringLiteral("preview"));
    preview->setReadOnly(true);
    tabs->addTab(preview, tr("Preview"));

    auto* privacy = new QLabel(tr("Everything in the preview will be public on GitHub. Leave out "
                                  "game files, keys, firmware, saves and private details."),
                               this);
    privacy->setWordWrap(true);
    layout->addWidget(privacy);
    status = new QLabel(this);
    status->setObjectName(QStringLiteral("status"));
    status->setTextFormat(Qt::PlainText);
    status->setWordWrap(true);
    layout->addWidget(status);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    clear = buttons->addButton(tr("Clear draft"), QDialogButtonBox::ResetRole);
    copy = buttons->addButton(tr("Copy report"), QDialogButtonBox::ActionRole);
    open = buttons->addButton(tr("Open on GitHub"), QDialogButtonBox::ActionRole);
    clear->setObjectName(QStringLiteral("clear"));
    copy->setObjectName(QStringLiteral("copy"));
    open->setObjectName(QStringLiteral("open"));
    layout->addWidget(buttons);
    for (auto* button : findChildren<QPushButton*>()) {
        button->setAutoDefault(false);
        button->setDefault(false);
    }
    connect(buttons, &QDialogButtonBox::rejected, this, &Dialog::reject);
    connect(open, &QPushButton::clicked, this, &Dialog::Open);
    connect(copy, &QPushButton::clicked, this, &Dialog::Copy);
    connect(clear, &QPushButton::clicked, this, &Dialog::Clear);
    for (auto* field : fields) {
        connect(field, &QLineEdit::textChanged, this, &Dialog::Refresh);
    }
    for (auto* field : paragraphs) {
        connect(field, &QPlainTextEdit::textChanged, this, &Dialog::Refresh);
    }
    connect(outcome, &QComboBox::currentIndexChanged, this, &Dialog::Refresh);
    connect(graphics_api, &QComboBox::currentIndexChanged, this, &Dialog::Refresh);
    Refresh();
}

Dialog::~Dialog() = default;

QString Dialog::Text(const QString& key) const {
    if (const auto it = fields.find(key); it != fields.end()) {
        return it.value()->text().trimmed();
    }
    if (const auto it = paragraphs.find(key); it != paragraphs.end()) {
        return it.value()->toPlainText().trimmed();
    }
    return {};
}

QString Dialog::Title() const {
    const QString version = Text(QStringLiteral("gameVersion"));
    QString title = QStringLiteral("[Compatibility] ") + Text(QStringLiteral("title"));
    if (!version.isEmpty() && version != QStringLiteral("Unknown")) {
        title += QStringLiteral(" (%1)").arg(version);
    }
    const QString rating = outcome->currentData().toString();
    if (!rating.isEmpty()) {
        title += QStringLiteral(": ") + rating;
    }
    return title;
}

QString Dialog::Markdown() const {
    // In English whatever the interface language, so every report reads and parses alike.
    QString text = QStringLiteral("**Compatibility:** %1\n\n%2\n\n")
                       .arg(outcome->currentData().toString(), Text(QStringLiteral("summary")));
    const auto section = [&](const QString& heading, const QString& key) {
        const QString value = Text(key);
        if (!value.isEmpty()) {
            text += QStringLiteral("**%1**\n\n%2\n\n").arg(heading, value);
        }
    };
    section(QStringLiteral("How far I played"), QStringLiteral("progress"));
    section(QStringLiteral("What worked or failed"), QStringLiteral("details"));
    section(QStringLiteral("Steps to reproduce"), QStringLiteral("reproduction"));
    text += QStringLiteral("**System and settings**\n\n| | |\n|---|---|\n");
    const auto row = [&](const QString& label, const QString& value) {
        text += QStringLiteral("| %1 | %2 |\n").arg(label, Cell(value));
    };
    row(QStringLiteral("Game"), Text(QStringLiteral("title")));
    row(QStringLiteral("Program ID"), ProgramId(program_id));
    row(QStringLiteral("Game version"), Text(QStringLiteral("gameVersion")));
    row(QStringLiteral("Kaichii build"), Text(QStringLiteral("build")));
    row(QStringLiteral("CPU"), Text(QStringLiteral("cpu")));
    row(QStringLiteral("GPU"), Text(QStringLiteral("gpu")));
    row(QStringLiteral("GPU driver"), Text(QStringLiteral("driver")));
    row(QStringLiteral("Graphics API"), graphics_api->currentText());
    row(QStringLiteral("Operating system"), Text(QStringLiteral("os")));
    row(QStringLiteral("RAM"), Text(QStringLiteral("ram")));
    row(QStringLiteral("Launch settings"), Text(QStringLiteral("settings")));
    row(QStringLiteral("Updates"), Text(QStringLiteral("updates")));
    row(QStringLiteral("Mods"), Text(QStringLiteral("mods")));
    text += QStringLiteral("\n<sub>Prepared by Kaichii's compatibility report.</sub>\n");
    return text;
}

QString Dialog::ValidationError() const {
    if (outcome->currentData().toString().isEmpty()) {
        return tr("Choose your own compatibility assessment.");
    }
    for (auto it = Limits.cbegin(); it != Limits.cend(); ++it) {
        const QString value = Text(it.key());
        if ((!OptionalFields.contains(it.key()) && value.isEmpty()) || value.size() > it.value() ||
            HasControls(value)) {
            return tr("Check %1: required fields must be filled, within their length limit, "
                      "and contain no control characters.")
                .arg(it.key());
        }
    }
    return {};
}

void Dialog::Refresh() {
    const QString validation = ValidationError();
    open->setEnabled(validation.isEmpty());
    open->setToolTip(validation);
    copy->setEnabled(validation.isEmpty());
    const QString content = Markdown();
    if (preview->toPlainText() != content) {
        preview->setPlainText(content);
    }
}

void Dialog::Copy() {
    QGuiApplication::clipboard()->setText(Markdown());
    status->setText(tr("Report copied. Paste it into a new compatibility report on GitHub."));
}

void Dialog::Open() {
    if (const QString error = ValidationError(); !error.isEmpty()) {
        status->setText(error);
        return;
    }
    const IssueLink link = NewIssueLink(repository, Title(), Markdown());
    if (link.url.isEmpty()) {
        status->setText(tr("This build doesn't name a GitHub repository for reports. Copy the "
                           "report and open it by hand."));
        return;
    }
    if (!link.report_included) {
        QGuiApplication::clipboard()->setText(Markdown());
    }
    if (!open_url(link.url)) {
        QGuiApplication::clipboard()->setText(Markdown());
        status->setText(tr("The browser didn't open. The report is in your clipboard: open %1 "
                           "and paste it there.")
                            .arg(link.url.toString(QUrl::RemoveQuery)));
        return;
    }
    status->setText(link.report_included
                        ? tr("GitHub is open in your browser. Review the report there and "
                             "publish it.")
                        : tr("The report was too long for a link, so it's in your clipboard: "
                             "paste it into the report field on GitHub, then publish it."));
}

void Dialog::Clear() {
    for (auto* field : fields) {
        field->clear();
    }
    for (auto* field : paragraphs) {
        field->clear();
    }
    outcome->setCurrentIndex(0);
    graphics_api->setCurrentIndex(0);
    status->setText(tr("Draft cleared."));
}

} // namespace CompatibilityReporting

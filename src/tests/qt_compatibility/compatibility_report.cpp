// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/compatibility_report.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QFile>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTest>
#include <QTranslator>
#include <QUrlQuery>

namespace CompatibilityReporting {

class DialogTest : public QObject {
    Q_OBJECT

public:
    static Session Seed();
    static int Preview(QApplication& app, const QString& screenshot = {});

private:
    /// Answers the assessment and the required experience fields.
    static void Fill(Dialog& dialog);
    static QString Report(const QUrl& url);

private slots:
    void opensAPrefilledIssueOnTheBuildsRepository();
    void needsTheAssessmentAndTheRequiredFields();
    void sendsNothingByItself();
    void keepsTableValuesInTheirRow();
    void longReportsGoThroughTheClipboard();
    void browserFailureKeepsTheReport();
    void refusesAnInvalidRepository();
    void clearEmptiesTheDraft();
    void matchesTheIssueTemplate();
    void dialogTranslationContext();
    void machinePrefillsRemainEditable();
    void enabledAddonPrefills();
    void asksHowSmoothlyAPlayedGameRan();
    void smoothnessIsNotAskedWhenTheGameDidNotGetIntoPlay();
    void writesTheFrameRateWhenGiven();
    void aiWritingIsOfferedOnlyWithAWriter();
    void anAiDraftFillsTheWrittenFieldsOnly();
    void anAiDraftNeedsEveryFieldAndASummary();
};

namespace {

const QString Repository = QStringLiteral("javocsoft/Kaichii");

/// Records the links the dialog opens, and answers with `opens`.
struct Browser {
    QList<QUrl> opened;
    bool opens = true;

    Dialog::UrlOpener Opener() {
        return [this](const QUrl& url) {
            opened.append(url);
            return opens;
        };
    }
};

/// For dialogs that never open a link.
const Dialog::UrlOpener NoBrowser = [](const QUrl&) { return true; };

} // namespace

Session DialogTest::Seed() {
    return {.program_id = 0x0400486024DEC000ULL,
            .title = QStringLiteral("Synthetic Switch 2 game"),
            .build = QStringLiteral("Kaichii build-91 (c5bc78e1b99a)"),
            .game_version = QStringLiteral("1.0.0"),
            .cpu = QStringLiteral("Example CPU"),
            .gpu = {},
            .driver = {},
            .os = QStringLiteral("Example OS"),
            .ram = QStringLiteral("16 GiB"),
            .graphics_api = QStringLiteral("Vulkan"),
            .settings = QStringLiteral("Launch settings: Handheld mode; resolution scale 1x."),
            .mods = {},
            .updates = {}};
}

void DialogTest::Fill(Dialog& dialog) {
    dialog.outcome->setCurrentIndex(dialog.outcome->findData(QStringLiteral("Playable")));
    dialog.smoothness->setCurrentIndex(dialog.smoothness->findData(QStringLiteral("Smooth")));
    dialog.fields[QStringLiteral("summary")]->setText(QStringLiteral("Synthetic summary"));
    dialog.paragraphs[QStringLiteral("progress")]->setPlainText(QStringLiteral("First area"));
    dialog.paragraphs[QStringLiteral("details")]->setPlainText(QStringLiteral("Menus worked."));
}

QString DialogTest::Report(const QUrl& url) {
    return QUrlQuery(url).queryItemValue(QString::fromLatin1(ReportField), QUrl::FullyDecoded);
}

int DialogTest::Preview(QApplication& app, const QString& screenshot) {
    auto session = Seed();
    session.gpu = QStringLiteral("Example active GPU");
    session.driver = QStringLiteral("Example driver 1.2.3");
    Dialog dialog(session, Repository, nullptr, [](const QUrl&) { return true; });
    Fill(dialog);
    dialog.status->setText(QStringLiteral("LOCAL MOCK ONLY. Open on GitHub opens nothing here."));
    dialog.show();
    if (!screenshot.isEmpty()) {
        dialog.tabs->setCurrentIndex(2);
        QTest::qWait(100);
        return dialog.grab().save(screenshot) ? 0 : 1;
    }
    return app.exec();
}

void DialogTest::opensAPrefilledIssueOnTheBuildsRepository() {
    Browser browser;
    Dialog dialog(Seed(), Repository, nullptr, browser.Opener());
    Fill(dialog);
    QVERIFY(dialog.open->isEnabled());
    dialog.open->click();
    QCOMPARE(browser.opened.size(), 1);
    const QUrl url = browser.opened.front();
    QCOMPARE(url.scheme(), QStringLiteral("https"));
    QCOMPARE(url.host(), QStringLiteral("github.com"));
    QCOMPARE(url.path(), QStringLiteral("/javocsoft/Kaichii/issues/new"));
    const QUrlQuery query(url);
    QCOMPARE(query.queryItemValue(QStringLiteral("template")),
             QString::fromLatin1(IssueTemplate));
    QCOMPARE(query.queryItemValue(QStringLiteral("title"), QUrl::FullyDecoded),
             QStringLiteral("[Compatibility] Synthetic Switch 2 game (1.0.0): Playable"));
    const QString report = Report(url);
    QCOMPARE(report, dialog.Markdown());
    QVERIFY(report.startsWith(QStringLiteral(
        "**Compatibility:** Playable\n\n**Smoothness:** Smooth\n\nSynthetic summary")));
    // No frame rate was given: no line for it.
    QVERIFY(!report.contains(QStringLiteral("**Frame rate:**")));
    QVERIFY(report.contains(QStringLiteral("| Program ID | 0400486024DEC000 |")));
    QVERIFY(
        report.contains(QStringLiteral("| Kaichii build | Kaichii build-91 (c5bc78e1b99a) |")));
    QVERIFY(report.contains(QStringLiteral("| GPU | Unknown |")));
    QVERIFY(!report.contains(QStringLiteral("Steps to reproduce")));
    QVERIFY(!dialog.status->text().isEmpty());
}

void DialogTest::needsTheAssessmentAndTheRequiredFields() {
    Browser browser;
    Dialog dialog(Seed(), Repository, nullptr, browser.Opener());
    QVERIFY(!dialog.open->isEnabled());
    QVERIFY(!dialog.open->toolTip().isEmpty());
    dialog.outcome->setCurrentIndex(dialog.outcome->findData(QStringLiteral("Runs")));
    QVERIFY(!dialog.open->isEnabled());
    Fill(dialog);
    QVERIFY(dialog.open->isEnabled());
    dialog.fields[QStringLiteral("title")]->clear();
    QVERIFY(!dialog.open->isEnabled());
    dialog.fields[QStringLiteral("title")]->setText(QStringLiteral("Synthetic Switch 2 game"));
    dialog.paragraphs[QStringLiteral("details")]->setPlainText(QStringLiteral("bad\u0007char"));
    QVERIFY(!dialog.open->isEnabled());
    dialog.Open();
    QVERIFY(browser.opened.isEmpty());
    // Optional fields may stay empty.
    dialog.paragraphs[QStringLiteral("details")]->setPlainText(QStringLiteral("Fine"));
    dialog.fields[QStringLiteral("ram")]->clear();
    dialog.paragraphs[QStringLiteral("mods")]->clear();
    QVERIFY(dialog.open->isEnabled());
}

void DialogTest::sendsNothingByItself() {
    Browser browser;
    {
        Dialog dialog(Seed(), Repository, nullptr, browser.Opener());
        Fill(dialog);
        dialog.copy->click();
        dialog.clear->click();
        dialog.reject();
    }
    QVERIFY(browser.opened.isEmpty());
}

void DialogTest::keepsTableValuesInTheirRow() {
    auto session = Seed();
    session.cpu = QStringLiteral("A | B");
    session.settings = QStringLiteral("first line\nsecond line");
    Dialog dialog(session, Repository, nullptr, NoBrowser);
    Fill(dialog);
    const QString report = dialog.Markdown();
    QVERIFY(report.contains(QStringLiteral("| CPU | A \\| B |")));
    QVERIFY(report.contains(QStringLiteral("| Launch settings | first line<br>second line |")));
    // The player's own paragraphs keep their lines.
    dialog.paragraphs[QStringLiteral("details")]->setPlainText(QStringLiteral("one\ntwo"));
    QVERIFY(dialog.Markdown().contains(QStringLiteral("**What worked or failed**\n\none\ntwo")));
}

void DialogTest::longReportsGoThroughTheClipboard() {
    Browser browser;
    Dialog dialog(Seed(), Repository, nullptr, browser.Opener());
    Fill(dialog);
    dialog.paragraphs[QStringLiteral("details")]->setPlainText(QString(5900, QLatin1Char('&')));
    QGuiApplication::clipboard()->clear();
    dialog.open->click();
    QCOMPARE(browser.opened.size(), 1);
    QVERIFY(browser.opened.front().toEncoded().size() < 7000);
    QVERIFY(Report(browser.opened.front()).contains(QStringLiteral("clipboard")));
    QCOMPARE(QGuiApplication::clipboard()->text(), dialog.Markdown());
    const auto link = NewIssueLink(Repository, dialog.Title(), dialog.Markdown());
    QVERIFY(!link.report_included);
    QVERIFY(NewIssueLink(Repository, dialog.Title(), QStringLiteral("short")).report_included);
}

void DialogTest::browserFailureKeepsTheReport() {
    Browser browser;
    browser.opens = false;
    Dialog dialog(Seed(), Repository, nullptr, browser.Opener());
    Fill(dialog);
    QGuiApplication::clipboard()->clear();
    dialog.open->click();
    QCOMPARE(browser.opened.size(), 1);
    QCOMPARE(QGuiApplication::clipboard()->text(), dialog.Markdown());
    QVERIFY(dialog.status->text().contains(QStringLiteral("github.com/javocsoft/Kaichii")));
}

void DialogTest::refusesAnInvalidRepository() {
    for (const auto& repository :
         {QString{}, QStringLiteral("Kaichii"), QStringLiteral("evil.example/x?y"),
          QStringLiteral("a/b/c"), QStringLiteral("owner/repo#fragment")}) {
        QVERIFY2(NewIssueLink(repository, QStringLiteral("t"), QStringLiteral("r")).url.isEmpty(),
                 qPrintable(repository));
    }
    Browser browser;
    Dialog dialog(Seed(), QString{}, nullptr, browser.Opener());
    Fill(dialog);
    dialog.open->click();
    QVERIFY(browser.opened.isEmpty());
    QVERIFY(!dialog.status->text().isEmpty());
}

void DialogTest::aiWritingIsOfferedOnlyWithAWriter() {
    {
        Dialog dialog(Seed(), Repository, nullptr, NoBrowser);
        QVERIFY(dialog.findChild<QPushButton*>(QStringLiteral("writeWithAi")) == nullptr);
    }
    bool asked = false;
    Dialog dialog(Seed(), Repository, nullptr, NoBrowser,
                  [&asked](const Session& session, QWidget*) -> std::optional<Draft> {
                      asked = true;
                      // The writer gets the session the report describes.
                      return Draft{session.title + QStringLiteral(" works"), {}, {}, {}};
                  });
    auto* const button = dialog.findChild<QPushButton*>(QStringLiteral("writeWithAi"));
    QVERIFY(button != nullptr);
    button->click();
    QVERIFY(asked);
    QCOMPARE(dialog.fields[QStringLiteral("summary")]->text(), Seed().title + QStringLiteral(" works"));
}

void DialogTest::anAiDraftFillsTheWrittenFieldsOnly() {
    Dialog dialog(Seed(), Repository, nullptr, NoBrowser);
    const QString cpu = dialog.fields[QStringLiteral("cpu")]->text();
    dialog.ApplyDraft({QStringLiteral("Boots and plays"), QStringLiteral("Second dungeon"),
                       QStringLiteral("Sound crackles in caves."), QStringLiteral("Enter a cave.")});
    QCOMPARE(dialog.fields[QStringLiteral("summary")]->text(), QStringLiteral("Boots and plays"));
    QCOMPARE(dialog.paragraphs[QStringLiteral("progress")]->toPlainText(), QStringLiteral("Second dungeon"));
    QCOMPARE(dialog.paragraphs[QStringLiteral("details")]->toPlainText(),
             QStringLiteral("Sound crackles in caves."));
    QCOMPARE(dialog.paragraphs[QStringLiteral("reproduction")]->toPlainText(),
             QStringLiteral("Enter a cave."));
    // The player still rates the game, and the machine fields stay as they were.
    QCOMPARE(dialog.outcome->currentIndex(), 0);
    QCOMPARE(dialog.smoothness->currentIndex(), 0);
    QCOMPARE(dialog.fields[QStringLiteral("cpu")]->text(), cpu);
}

void DialogTest::anAiDraftNeedsEveryFieldAndASummary() {
    QJsonObject reply{{QStringLiteral("summary"), QStringLiteral("  Runs\nwell  ")},
                      {QStringLiteral("progress"), QString{}},
                      {QStringLiteral("details"), QStringLiteral("Fine.")},
                      {QStringLiteral("reproduction"), QString{}}};
    const auto draft = DraftFromJson(reply);
    QVERIFY(draft.has_value());
    QCOMPARE(draft->summary, QStringLiteral("Runs well"));
    reply.insert(QStringLiteral("summary"), QString(500, QLatin1Char('a')));
    QCOMPARE(DraftFromJson(reply)->summary.size(), 120);
    reply.insert(QStringLiteral("summary"), QStringLiteral("   "));
    QVERIFY(!DraftFromJson(reply).has_value());
    reply.insert(QStringLiteral("summary"), QStringLiteral("Runs"));
    reply.remove(QStringLiteral("details"));
    QVERIFY(!DraftFromJson(reply).has_value());
    reply.insert(QStringLiteral("details"), 3);
    QVERIFY(!DraftFromJson(reply).has_value());
}

void DialogTest::clearEmptiesTheDraft() {
    Dialog dialog(Seed(), Repository, nullptr, NoBrowser);
    Fill(dialog);
    dialog.clear->click();
    QCOMPARE(dialog.outcome->currentIndex(), 0);
    QCOMPARE(dialog.smoothness->currentIndex(), 0);
    QVERIFY(dialog.smoothness->isEnabled());
    QVERIFY(dialog.fields[QStringLiteral("summary")]->text().isEmpty());
    QVERIFY(dialog.fields[QStringLiteral("cpu")]->text().isEmpty());
    QVERIFY(dialog.paragraphs[QStringLiteral("details")]->toPlainText().isEmpty());
    QVERIFY(!dialog.open->isEnabled());
}

void DialogTest::matchesTheIssueTemplate() {
    QFile file(QStringLiteral(ISSUE_TEMPLATE_DIR "/") + QString::fromLatin1(IssueTemplate));
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.fileName()));
    const QString form = QString::fromUtf8(file.readAll());
    QVERIFY(form.contains(QStringLiteral("id: ") + QString::fromLatin1(ReportField)));
    QVERIFY(form.contains(QStringLiteral("title: \"[Compatibility] \"")));
}

void DialogTest::dialogTranslationContext() {
    class Translator final : public QTranslator {
    public:
        QString translate(const char* context, const char* source, const char*,
                          int) const override {
            if (QByteArray(context) == "CompatibilityReporting::Dialog" &&
                QByteArray(source) == "Submit compatibility report") {
                return QStringLiteral("Translated report title");
            }
            return {};
        }
        bool isEmpty() const override {
            return false;
        }
    } translator;
    QCoreApplication::installTranslator(&translator);
    Dialog dialog(Seed(), Repository, nullptr, NoBrowser);
    const auto title = dialog.windowTitle();
    QCoreApplication::removeTranslator(&translator);
    QCOMPARE(title, QStringLiteral("Translated report title"));
}

void DialogTest::machinePrefillsRemainEditable() {
    auto session = Seed();
    session.gpu = QStringLiteral(" Active rendering GPU ");
    session.driver = QStringLiteral(" Driver 123.45 ");
    Dialog dialog(session, Repository, nullptr, NoBrowser);
    QCOMPARE(dialog.fields[QStringLiteral("gpu")]->text(), QStringLiteral("Active rendering GPU"));
    QCOMPARE(dialog.fields[QStringLiteral("driver")]->text(), QStringLiteral("Driver 123.45"));
    QCOMPARE(dialog.graphics_api->currentText(), QStringLiteral("Vulkan"));
    QCOMPARE(dialog.fields[QStringLiteral("cpu")]->text(), session.cpu);
    QCOMPARE(dialog.fields[QStringLiteral("ram")]->text(), session.ram);
    QCOMPARE(dialog.fields[QStringLiteral("os")]->text(), session.os);
    QCOMPARE(dialog.fields[QStringLiteral("title")]->text(), session.title);
    dialog.fields[QStringLiteral("driver")]->setText(QStringLiteral("Corrected driver"));
    QVERIFY(dialog.Markdown().contains(QStringLiteral("| GPU driver | Corrected driver |")));
    QVERIFY(dialog.preview->toPlainText().contains(QStringLiteral("Corrected driver")));
}

void DialogTest::enabledAddonPrefills() {
    auto session = Seed();
    session.mods = SummarizeEnabledAddons({
        {QStringLiteral("Dynamic FPS"), QStringLiteral("IPS"), true},
        {QStringLiteral("Max Resolution"), QStringLiteral("IPS"), false},
        {QStringLiteral("Dynamic FPS"), QStringLiteral("IPS"), true},
    });
    session.updates = SummarizeEnabledAddons({
        {QStringLiteral("Update"), QStringLiteral("2.0.0"), true},
        {QStringLiteral("Update"), QStringLiteral("1.0.0"), false},
    });
    QCOMPARE(session.mods, QStringLiteral("Enabled at launch: Dynamic FPS (IPS)"));
    QCOMPARE(session.updates, QStringLiteral("Enabled at launch: Update (2.0.0)"));
    QCOMPARE(SummarizeEnabledAddons({}), QStringLiteral("None listed as enabled at launch."));
    const auto long_summary = SummarizeEnabledAddons({{QString(700, QLatin1Char('x')), {}, true}});
    QVERIFY(long_summary.size() <= 500);
    QVERIFY(long_summary.endsWith(QStringLiteral("(list truncated; review)")));
    Dialog dialog(session, Repository, nullptr, NoBrowser);
    QCOMPARE(dialog.paragraphs[QStringLiteral("mods")]->toPlainText(), session.mods);
    QCOMPARE(dialog.paragraphs[QStringLiteral("updates")]->toPlainText(), session.updates);
    QVERIFY(dialog.Markdown().contains(QStringLiteral("| Mods | ") + session.mods));
}

void DialogTest::asksHowSmoothlyAPlayedGameRan() {
    Dialog dialog(Seed(), Repository, nullptr, NoBrowser);
    Fill(dialog);
    QVERIFY(dialog.open->isEnabled());
    // A game that got into play needs a smoothness, and "not applicable" isn't one.
    dialog.smoothness->setCurrentIndex(0);
    QVERIFY(!dialog.open->isEnabled());
    QVERIFY(!dialog.open->toolTip().isEmpty());
    for (const auto* level : {"Perfect", "Smooth", "Stutters", "Choppy", "Poor"}) {
        const int index = dialog.smoothness->findData(QString::fromLatin1(level));
        QVERIFY2(index > 0, level);
        dialog.smoothness->setCurrentIndex(index);
        QVERIFY2(dialog.open->isEnabled(), level);
        QVERIFY(dialog.Markdown().contains(
            QStringLiteral("**Smoothness:** %1\n").arg(QString::fromLatin1(level))));
    }
}

void DialogTest::smoothnessIsNotAskedWhenTheGameDidNotGetIntoPlay() {
    Dialog dialog(Seed(), Repository, nullptr, NoBrowser);
    Fill(dialog);
    for (const auto* rating : {"Loads", "Unplayable"}) {
        dialog.outcome->setCurrentIndex(dialog.outcome->findData(QString::fromLatin1(rating)));
        QVERIFY2(!dialog.smoothness->isEnabled(), rating);
        QCOMPARE(dialog.smoothness->currentData().toString(), QStringLiteral("Not applicable"));
        QVERIFY2(dialog.open->isEnabled(), rating);
        QVERIFY(dialog.Markdown().contains(QStringLiteral("**Smoothness:** Not applicable\n")));
    }
    // Back to a rating in play: the question is open again, not answered with "not applicable".
    dialog.outcome->setCurrentIndex(dialog.outcome->findData(QStringLiteral("Runs")));
    QVERIFY(dialog.smoothness->isEnabled());
    QCOMPARE(dialog.smoothness->currentIndex(), 0);
    QVERIFY(!dialog.open->isEnabled());
}

void DialogTest::writesTheFrameRateWhenGiven() {
    Dialog dialog(Seed(), Repository, nullptr, NoBrowser);
    Fill(dialog);
    dialog.fields[QStringLiteral("fps")]->setText(QStringLiteral("25-30"));
    QVERIFY(dialog.open->isEnabled());
    QVERIFY(dialog.Markdown().contains(
        QStringLiteral("**Smoothness:** Smooth\n\n**Frame rate:** 25-30 fps\n\nSynthetic")));
}

} // namespace CompatibilityReporting

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    if (app.arguments().contains(QStringLiteral("--preview"))) {
        return CompatibilityReporting::DialogTest::Preview(app);
    }
    const int screenshot = app.arguments().indexOf(QStringLiteral("--screenshot"));
    if (screenshot >= 0 && screenshot + 1 < app.arguments().size()) {
        return CompatibilityReporting::DialogTest::Preview(app, app.arguments()[screenshot + 1]);
    }
    CompatibilityReporting::DialogTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "compatibility_report.moc"

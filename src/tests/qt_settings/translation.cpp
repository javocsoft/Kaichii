// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// The Spanish catalog built from dist/languages/es.ts and Qt's own next to it: the screens find
// their strings under the context they ask for, and Qt's buttons follow the interface language.

#include <algorithm>

#include <QApplication>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QTranslator>
#include <QtTest>

#include "qt_common/util/guide.h"
#include "qt_common/util/translations.h"
#include "yuzu/configuration/per_game_summary.h"
#include "yuzu/first_run_assistant.h"

namespace {

const QString Directory = QStringLiteral(TRANSLATIONS_DIR);

bool ShowsText(const QWidget& root, const QString& text) {
    const auto labels = root.findChildren<QLabel*>();
    return std::ranges::any_of(labels, [&](const QLabel* label) { return label->text() == text; });
}

FirstRunAssistant::Hooks NoWindow() {
    return {
        .choose_folder = [] { return QString{}; },
        .add_game_folder = [](const QString&) {},
        .game_folders = [] { return QStringList{}; },
        .keys_present = [] { return true; },
        .install_keys = [] {},
        .configure_controls = [] {},
    };
}

} // Anonymous namespace

class TranslationTest : public QObject {
    Q_OBJECT

private slots:
    void cleanup();
    void LoadsTheInterfaceAndQtCatalogs();
    void FollowsTheSystemLanguage();
    void ClearsQtWithoutItsCatalog();
    void TranslatesTheGameSummary();
    void TranslatesTheAssistantButtons();
    void OpensTheGuideInTheInterfaceLanguage();

private:
    /// Loads `language` and installs both catalogs, as the main window does.
    void Install(const QString& language);

    /// The interface catalog's text for `source`, which must differ from the English.
    QString Spanish(const char* context, const char* source) const;

    QTranslator interface;
    QTranslator qt;
};

void TranslationTest::Install(const QString& language) {
    QVERIFY(QtCommon::Translations::Load(interface, qt, language, Directory));
    QVERIFY2(!qt.isEmpty(), qPrintable(QStringLiteral("no Qt catalog in ") + Directory));
    QApplication::installTranslator(&qt);
    QApplication::installTranslator(&interface);
}

QString TranslationTest::Spanish(const char* context, const char* source) const {
    const QString text = interface.translate(context, source);
    return text.isEmpty() || text == QLatin1String(source) ? QString{} : text;
}

void TranslationTest::cleanup() {
    QApplication::removeTranslator(&interface);
    QApplication::removeTranslator(&qt);
    QLocale::setDefault(QLocale::system());
}

void TranslationTest::LoadsTheInterfaceAndQtCatalogs() {
    Install(QStringLiteral("es"));
    QDialogButtonBox box(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    const QString cancel = box.button(QDialogButtonBox::Cancel)->text();
    QVERIFY(cancel != QStringLiteral("Cancel"));
    QCOMPARE(cancel, qt.translate("QPlatformTheme", "Cancel"));
}

void TranslationTest::FollowsTheSystemLanguage() {
    QLocale::setDefault(QLocale(QLocale::Spanish, QLocale::Spain));
    QVERIFY(QtCommon::Translations::Load(interface, qt, {}, Directory));
    QVERIFY(!interface.isEmpty());
    QVERIFY(!qt.isEmpty());
}

void TranslationTest::ClearsQtWithoutItsCatalog() {
    QVERIFY(QtCommon::Translations::Load(interface, qt, QStringLiteral("es"), Directory));
    QVERIFY(!qt.isEmpty());
    // Neither catalog exists: nothing of the language before stays.
    QVERIFY(!QtCommon::Translations::Load(interface, qt, QStringLiteral("xx"), Directory));
    QVERIFY(interface.isEmpty());
    QVERIFY(qt.isEmpty());
}

void TranslationTest::TranslatesTheGameSummary() {
    Install(QStringLiteral("es"));
    const ConfigurationShared::TranslationMap names;
    const ConfigurationShared::ComboboxTranslationMap enums;
    const Settings::HostProfile host{.gpu_name = "Test GPU",
                                     .gpu_vendor_id = 0x10DE,
                                     .gpu_memory_bytes = u64{4} << 30,
                                     .cpu_threads = 20,
                                     .ram_bytes = u64{32} << 30};
    ConfigurationShared::PerGameSummary summary(names, enums, host);
    for (const char* source : {"What this game changes from the global settings",
                               "Nothing: this game uses the global settings in every tab."}) {
        const QString text = Spanish("ConfigurationShared", source);
        QVERIFY2(!text.isEmpty(), source);
        QVERIFY2(ShowsText(summary, text), source);
    }
}

void TranslationTest::TranslatesTheAssistantButtons() {
    Install(QStringLiteral("es"));
    FirstRunAssistant assistant(nullptr, std::nullopt, {}, false, NoWindow());
    const QString title = Spanish("FirstRunAssistant", "Your games");
    QVERIFY(!title.isEmpty());
    QCOMPARE(assistant.page(assistant.startId())->title(), title);
    const QString next = assistant.button(QWizard::NextButton)->text();
    QVERIFY(next != QStringLiteral("&Next >"));
    QCOMPARE(next, qt.translate("QWizard", "&Next >"));
}

void TranslationTest::OpensTheGuideInTheInterfaceLanguage() {
    using QtCommon::Guide::PageUrl;
    const QString repository = QStringLiteral("JavocSoft/Kaichii");
    const QString english = QStringLiteral("https://javocsoft.github.io/Kaichii/");
    const QString spanish = english + QStringLiteral("es/");
    for (const char* language : {"es", "es_ES", "es-MX"}) {
        QCOMPARE(PageUrl(repository, QString::fromLatin1(language)).toString(), spanish);
    }
    for (const char* language : {"", "en", "pt_BR", "eslabon"}) {
        QCOMPARE(PageUrl(repository, QString::fromLatin1(language)).toString(), english);
    }
    QCOMPARE(PageUrl(repository, QStringLiteral("es"), QStringLiteral("Compatibility/")).toString(),
             spanish + QStringLiteral("Compatibility/"));
    for (const char* bad : {"", "Kaichii", "a/b/c", "evil.example/x?y"}) {
        QVERIFY(PageUrl(QString::fromLatin1(bad), QStringLiteral("es")).isEmpty());
    }
    for (const char* bad : {"../x/", "Compatibility", "https://evil.example/", "a/#b/"}) {
        QVERIFY(PageUrl(repository, QStringLiteral("es"), QString::fromLatin1(bad)).isEmpty());
    }
}

QTEST_MAIN(TranslationTest)
#include "translation.moc"

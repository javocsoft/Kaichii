// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFocusEvent>
#include <QtTest>

#include "common/performance_presets.h"
#include "common/settings.h"
#include "qt_common/config/setting_help.h"
#include "qt_common/config/shared_translation.h"
#include "qt_common/config/uisettings.h"
#include "qt_common/util/vk.h"
#include "yuzu/configuration/setting_help_panel.h"

using namespace ConfigurationShared;

namespace {

struct Tables {
    std::unique_ptr<TranslationMap> names = InitializeTranslations(nullptr);
    std::unique_ptr<SettingHelpMap> help = InitializeSettingHelp();
    std::unique_ptr<ComboboxTranslationMap> enums = ComboboxEnumeration(nullptr);
};

/// A machine whose recommended preset is Performance: a 4 GB discrete card.
Settings::HostProfile EntryMachine() {
    return {.gpu_name = "Test GPU",
            .gpu_vendor_id = 0x10DE,
            .gpu_memory_bytes = u64{4} << 30,
            .cpu_threads = 20,
            .ram_bytes = u64{32} << 30};
}

} // Anonymous namespace

class SettingHelpTest : public QObject {
    Q_OBJECT

private slots:
    void EveryShownSettingIsExplained();
    void HelpTextNamesEffectsAndValues();
    void PanelFollowsPointerAndFocus();
    void SoftwareRenderersShareSystemMemory();
};

void SettingHelpTest::EveryShownSettingIsExplained() {
    const Tables tables;
    QStringList problems;
    const auto check = [&](const Settings::BasicSetting& setting) {
        const QString label = QString::fromStdString(setting.GetLabel());
        const auto it = tables.names->find(setting.Id());
        if (it == tables.names->end()) {
            problems << QStringLiteral("no name (the pages show its key): ") + label;
            return;
        }
        if (it->second.first.isEmpty()) {
            return; // Not shown
        }
        if (it->second.second.trimmed().isEmpty()) {
            problems << QStringLiteral("no description: ") + label;
        }
        if (!tables.help->contains(setting.Id())) {
            problems << QStringLiteral("no level or effects in setting_help.cpp: ") + label;
        }
    };
    // The categories the configuration pages build their widgets from.
    using Settings::Category;
    for (const Category category :
         {Category::Renderer, Category::RendererAdvanced, Category::RendererHacks,
          Category::RendererExtensions, Category::LibraryApplet, Category::Audio,
          Category::SystemAudio, Category::Cpu, Category::CpuUnsafe, Category::Core,
          Category::System}) {
        for (const Settings::BasicSetting* setting :
             Settings::values.linkage.by_category[category]) {
            check(*setting);
        }
    }
    for (const Category category : {Category::UiGeneral, Category::UiAudio}) {
        for (const Settings::BasicSetting* setting :
             UISettings::values.linkage.by_category[category]) {
            check(*setting);
        }
    }
    // And the help table only names settings that are shown.
    for (const auto& [id, entry] : *tables.help) {
        const Settings::BasicSetting* setting = FindSetting(id);
        const auto it = tables.names->find(id);
        if (setting == nullptr || it == tables.names->end() || it->second.first.isEmpty()) {
            problems << QStringLiteral("help for a setting that is not shown: %1").arg(id);
        }
    }
    QVERIFY2(problems.isEmpty(), qPrintable(problems.join(QLatin1Char('\n'))));
}

void SettingHelpTest::HelpTextNamesEffectsAndValues() {
    const Tables tables;
    const Settings::HostProfile host = EntryMachine();
    QCOMPARE(Settings::RecommendedPreset(host), Settings::PerformancePreset::Performance);
    const SettingHelpContext context{
        .names = *tables.names, .help = *tables.help, .enums = *tables.enums, .host = &host};

    auto& resolution = Settings::values.resolution_setup;
    const QString text = SettingHelpHtml(resolution, context);
    QVERIFY(text.contains(QStringLiteral("<b>Resolution</b>")));
    QVERIFY(text.contains(tables.names->at(resolution.Id()).second.left(40).toHtmlEscaped()));
    QVERIFY(text.contains(QStringLiteral("Higher: ▲ sharper or cleaner image · ▼ slower")));
    QVERIFY(text.contains(QStringLiteral("Default: ") +
                          FormatSettingValue(resolution, resolution.DefaultToString(),
                                             *tables.enums)
                              .toHtmlEscaped()));
    QString preset_value;
    for (const auto& entry : Settings::PerformancePresetEntries(
             Settings::values, Settings::PerformancePreset::Performance, host)) {
        if (entry.setting == &resolution) {
            preset_value = FormatSettingValue(resolution, entry.value, *tables.enums);
        }
    }
    QVERIFY(!preset_value.isEmpty());
    QVERIFY(text.contains(QStringLiteral("Recommended for this computer (Performance preset): ") +
                          preset_value.toHtmlEscaped()));

    const QString ocarina = SettingHelpHtml(Settings::values.oot_anti_aliasing, context);
    QVERIFY(ocarina.contains(QStringLiteral("Takes effect when a game starts.")));
    QVERIFY(ocarina.contains(
        QStringLiteral("Only acts on The Legend of Zelda: Ocarina of Time.")));

    const QString frame_gen = SettingHelpHtml(Settings::values.frame_gen, context);
    QVERIFY(frame_gen.contains(QStringLiteral("When on: ▲ more frames on screen")));
    QVERIFY(frame_gen.contains(QStringLiteral("Default: off")));
    QVERIFY(!frame_gen.contains(QStringLiteral("option")));

    const QString dump = SettingHelpHtml(Settings::values.frame_gen_dump_flow, context);
    QVERIFY(dump.contains(QStringLiteral("Developer option: for debugging Lichii")));

    // Interface settings don't wait for a game, and no preset sets them.
    const QString updates = SettingHelpHtml(UISettings::values.check_for_updates, context);
    QVERIFY(!updates.contains(QStringLiteral("Takes effect when a game starts")));
    QVERIFY(!updates.contains(QStringLiteral("Recommended for this computer")));

    // Without a machine, no recommendation.
    const SettingHelpContext no_host{
        .names = *tables.names, .help = *tables.help, .enums = *tables.enums};
    QVERIFY(!SettingHelpHtml(resolution, no_host)
                 .contains(QStringLiteral("Recommended for this computer")));
}

void SettingHelpTest::PanelFollowsPointerAndFocus() {
    const Tables tables;
    QWidget page;
    auto* const row = new QWidget(&page);
    row->setProperty(SettingIdProperty, Settings::values.fsr_sharpening_slider.Id());
    auto* const combo = new QComboBox(row);
    auto* const plain = new QCheckBox(QStringLiteral("&Plain option:"), &page);
    plain->setToolTip(QStringLiteral("What the plain option does."));
    auto* const silent = new QCheckBox(QStringLiteral("Silent option"), &page);

    SettingHelpPanel panel(*tables.names, *tables.help, *tables.enums, std::nullopt);
    const QString placeholder = panel.Text();
    panel.Watch(&page);

    QEvent enter(QEvent::Enter);
    QApplication::sendEvent(combo, &enter);
    QVERIFY(panel.Text().contains(QStringLiteral("<b>Sharpening</b>")));

    QFocusEvent focus(QEvent::FocusIn);
    QApplication::sendEvent(plain, &focus);
    QVERIFY(panel.Text().contains(QStringLiteral("<b>Plain option</b>")));
    QVERIFY(panel.Text().contains(QStringLiteral("What the plain option does.")));

    // A control with nothing to say keeps the last explanation.
    QApplication::sendEvent(silent, &enter);
    QVERIFY(panel.Text().contains(QStringLiteral("What the plain option does.")));

    // Controls outside the watched page don't change it.
    QWidget outside;
    outside.setProperty(SettingIdProperty, Settings::values.resolution_setup.Id());
    QApplication::sendEvent(&outside, &enter);
    QVERIFY(panel.Text().contains(QStringLiteral("What the plain option does.")));
    QVERIFY(panel.Text() != placeholder);
}

void SettingHelpTest::SoftwareRenderersShareSystemMemory() {
    QVERIFY(VkDeviceInfo::SharesSystemMemory(VK_PHYSICAL_DEVICE_TYPE_CPU));
    QVERIFY(VkDeviceInfo::SharesSystemMemory(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU));
    QVERIFY(!VkDeviceInfo::SharesSystemMemory(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU));
    // llvmpipe reports the whole RAM as device-local memory: shared, it is the weakest class.
    Settings::HostProfile software = EntryMachine();
    software.gpu_memory_bytes = u64{31} << 30;
    software.gpu_integrated = VkDeviceInfo::SharesSystemMemory(VK_PHYSICAL_DEVICE_TYPE_CPU);
    QCOMPARE(Settings::ClassifyHost(software), Settings::HardwareTier::Entry);
}

QTEST_MAIN(SettingHelpTest)
#include "setting_help.moc"

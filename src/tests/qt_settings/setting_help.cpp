// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <map>
#include <memory>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFocusEvent>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QtTest>

#include "common/performance_presets.h"
#include "common/settings.h"
#include "qt_common/config/setting_help.h"
#include "qt_common/config/shared_translation.h"
#include "qt_common/config/uisettings.h"
#include "qt_common/util/vk.h"
#include "yuzu/configuration/setting_help_panel.h"
#include "yuzu/configuration/settings_organizer.h"

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
    void GraphicsSectionsPlaceEveryGraphicsSetting();
    void SearchFoldsCaseAndAccents();
    void OrganizerMovesAndGatesRows();
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

void SettingHelpTest::GraphicsSectionsPlaceEveryGraphicsSetting() {
    const Tables tables;
    std::map<u32, int> placed;
    for (const GoalSection& section : GraphicsSections()) {
        for (const u32 id : section.ids) {
            ++placed[id];
        }
    }
    QStringList problems;
    using Settings::Category;
    for (const Category category : {Category::Renderer, Category::RendererAdvanced,
                                    Category::RendererHacks, Category::RendererExtensions}) {
        for (const Settings::BasicSetting* setting :
             Settings::values.linkage.by_category[category]) {
            const auto it = tables.names->find(setting->Id());
            if (it == tables.names->end() || it->second.first.isEmpty()) {
                continue; // Not shown
            }
            if (placed[setting->Id()] != 1) {
                problems << QStringLiteral("in %1 sections: %2")
                                .arg(placed[setting->Id()])
                                .arg(QString::fromStdString(setting->GetLabel()));
            }
        }
    }
    for (const auto& [id, count] : placed) {
        const auto it = tables.names->find(id);
        if (FindSetting(id) == nullptr || it == tables.names->end() || it->second.first.isEmpty()) {
            problems << QStringLiteral("a section names a setting that is not shown: %1").arg(id);
        }
    }
    QVERIFY2(problems.isEmpty(), qPrintable(problems.join(QLatin1Char('\n'))));
}

void SettingHelpTest::SearchFoldsCaseAndAccents() {
    QVERIFY(MatchesSearch(QStringLiteral("Resolución"), QStringLiteral("resolucion")));
    QVERIFY(MatchesSearch(QStringLiteral("Activar la generación de fotogramas"),
                          QStringLiteral("fotogramas GENERACIÓN")));
    QVERIFY(!MatchesSearch(QStringLiteral("Volumen"), QStringLiteral("volumen alto")));
    // Words match from their start: the theme, not the system.
    QVERIFY(MatchesSearch(QStringLiteral("Tema:"), QStringLiteral("tem")));
    QVERIFY(!MatchesSearch(QStringLiteral("Sistema"), QStringLiteral("tema")));
    QVERIFY(MatchesSearch(QStringLiteral("VSync Mode (Vulkan only)"), QStringLiteral("vulkan")));
    QVERIFY(!MatchesSearch(QStringLiteral("Volumen"), QStringLiteral("  ")));
}

void SettingHelpTest::OrganizerMovesAndGatesRows() {
    const Tables tables;
    QWidget source;
    auto* const source_layout = new QVBoxLayout(&source);
    const auto row = [&](const Settings::BasicSetting& setting) {
        auto* const widget = new QWidget(&source);
        widget->setProperty(SettingIdProperty, setting.Id());
        source_layout->addWidget(widget);
        return widget;
    };
    QWidget* const resolution = row(Settings::values.resolution_setup); // Basic
    QWidget* const anisotropy = row(Settings::values.max_anisotropy);    // Advanced
    QWidget* const dump = row(Settings::values.frame_gen_dump_flow);     // Developer
    QWidget* const ocarina = row(Settings::values.oot_anti_aliasing);    // Ocarina of Time only
    // A group a page wraps a setting in is its row.
    QWidget* const dlss = row(Settings::values.nvidia_dlss);
    auto* const inner = new QWidget(dlss);
    inner->setProperty(SettingIdProperty, Settings::values.nvidia_dlss.Id());
    QCOMPARE(FindSettingRow(&source, Settings::values.nvidia_dlss.Id()), dlss);

    SettingsOrganizer global(*tables.help, 0);
    const std::array<QWidget*, 1> sources{&source};
    std::vector<PageSection> sections;
    sections.push_back({.title = "Picture",
                        .ids = {Settings::values.oot_anti_aliasing.Id(),
                                Settings::values.max_anisotropy.Id(),
                                Settings::values.resolution_setup.Id(),
                                Settings::values.frame_gen_dump_flow.Id()}});
    sections.push_back({.title = "Nothing here", .ids = {Settings::values.sample_shading.Id()}});
    const std::unique_ptr<QWidget> page{global.BuildPage(sources, std::move(sections))};

    // In the order given, and without the section that found nothing.
    const auto boxes = page->findChildren<QGroupBox*>();
    QCOMPARE(boxes.size(), 1);
    QLayout* const box_layout = boxes[0]->layout();
    QVERIFY(box_layout->indexOf(ocarina) < box_layout->indexOf(anisotropy));
    QVERIFY(box_layout->indexOf(anisotropy) < box_layout->indexOf(resolution));
    QCOMPARE(dlss->parentWidget(), &source);

    global.Gate(page.get());
    QVERIFY(resolution->isVisibleTo(page.get()));
    QVERIFY(!anisotropy->isVisibleTo(page.get()));
    QVERIFY(!dump->isVisibleTo(page.get()));
    QVERIFY(!ocarina->isVisibleTo(page.get()));

    global.SetShowAdvanced(true);
    QVERIFY(anisotropy->isVisibleTo(page.get()));
    QVERIFY(!dump->isVisibleTo(page.get()));
    global.SetDeveloperMode(true);
    QVERIFY(dump->isVisibleTo(page.get()));
    // Another game's option never shows in the global settings.
    QVERIFY(!ocarina->isVisibleTo(page.get()));

    // A row its page hid stays hidden when its level shows again, and its wrapper takes no room.
    anisotropy->setVisible(false);
    global.SetShowAdvanced(false);
    global.SetShowAdvanced(true);
    QVERIFY(!anisotropy->isVisibleTo(page.get()));
    QVERIFY(!anisotropy->parentWidget()->isVisibleTo(page.get()));
    anisotropy->setVisible(true);
    QVERIFY(anisotropy->isVisibleTo(page.get()));

    // Gating a page again doesn't wrap its rows again.
    const auto widgets = page->findChildren<QWidget*>().size();
    global.Gate(page.get());
    QCOMPARE(page->findChildren<QWidget*>().size(), widgets);

    // In Ocarina of Time's own configuration, its option shows.
    QWidget game_page;
    auto* const game_layout = new QVBoxLayout(&game_page);
    auto* const game_row = new QWidget(&game_page);
    game_row->setProperty(SettingIdProperty, Settings::values.oot_anti_aliasing.Id());
    game_layout->addWidget(game_row);
    SettingsOrganizer ocarina_settings(*tables.help, 0x0400486024DEC000);
    ocarina_settings.Gate(&game_page);
    QVERIFY(game_row->isVisibleTo(&game_page));
}

QTEST_MAIN(SettingHelpTest)
#include "setting_help.moc"

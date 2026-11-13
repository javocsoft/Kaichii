// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "qt_common/config/setting_help.h"

#include <algorithm>

#include <QCoreApplication>
#include <QRegularExpression>
#include <QStringList>
#include <QTextDocument>

#include "common/performance_presets.h"
#include "common/settings.h"
#include "qt_common/config/uisettings.h"

namespace ConfigurationShared {

namespace {

constexpr u64 OcarinaOfTime = 0x0400486024DEC000;
constexpr u64 MarioKartWorld = 0x0400C3F00006E000;

QString Translate(const char* text) {
    return QCoreApplication::translate("ConfigurationShared", text);
}

bool IsGameSetting(const Settings::BasicSetting& setting) {
    const auto& by_key = Settings::values.linkage.by_key;
    const auto it = by_key.find(setting.GetLabel());
    return it != by_key.end() && it->second == &setting;
}

/// Paragraphs closer than Qt's default, so a setting's facts fit the panel without scrolling.
constexpr char ParagraphStart[] = "<p style=\"margin-top:0px; margin-bottom:6px;\">";

QString Paragraph(const QString& text) {
    return QLatin1String(ParagraphStart) +
           text.toHtmlEscaped().replace(QLatin1Char('\n'), QStringLiteral("<br>")) +
           QStringLiteral("</p>");
}

QString EffectText(SettingEffect effect) {
    const auto& tr = Translate;
    switch (effect) {
    case SettingEffect::Faster:
        return tr("▲ faster");
    case SettingEffect::Slower:
        return tr("▼ slower");
    case SettingEffect::BetterImage:
        return tr("▲ sharper or cleaner image");
    case SettingEffect::WorseImage:
        return tr("▼ image quality");
    case SettingEffect::LessStutter:
        return tr("▲ less stutter");
    case SettingEffect::MoreFrames:
        return tr("▲ more frames on screen");
    case SettingEffect::MoreLag:
        return tr("▼ more input lag");
    case SettingEffect::MoreVideoMemory:
        return tr("▲ more video memory");
    case SettingEffect::LessVideoMemory:
        return tr("▼ less video memory");
    case SettingEffect::MayCauseGlitches:
        return tr("⚠ can cause graphical glitches");
    case SettingEffect::MayFreeze:
        return tr("⚠ can freeze or crash games");
    }
    return {};
}

QString LevelText(SettingLevel level) {
    const auto& tr = Translate;
    switch (level) {
    case SettingLevel::Basic:
        return {};
    case SettingLevel::Advanced:
        return tr("Advanced option");
    case SettingLevel::Expert:
        return tr("Expert option: change it only to fix a problem in a game");
    case SettingLevel::Developer:
        return tr("Developer option: for debugging Kaichii");
    }
    return {};
}

} // Anonymous namespace

std::unique_ptr<SettingHelpMap> InitializeSettingHelp() {
    auto help = std::make_unique<SettingHelpMap>();
    using enum SettingLevel;
    using enum SettingEffect;
    const auto add = [&](const Settings::BasicSetting& setting, SettingHelp entry) {
        help->insert_or_assign(setting.Id(), std::move(entry));
    };
    auto& v = Settings::values;
    auto& ui = UISettings::values;

    // Applets
    for (const Settings::BasicSetting* applet :
         {static_cast<const Settings::BasicSetting*>(&v.cabinet_applet_mode),
          static_cast<const Settings::BasicSetting*>(&v.controller_applet_mode),
          static_cast<const Settings::BasicSetting*>(&v.data_erase_applet_mode),
          static_cast<const Settings::BasicSetting*>(&v.error_applet_mode),
          static_cast<const Settings::BasicSetting*>(&v.net_connect_applet_mode),
          static_cast<const Settings::BasicSetting*>(&v.player_select_applet_mode),
          static_cast<const Settings::BasicSetting*>(&v.swkbd_applet_mode),
          static_cast<const Settings::BasicSetting*>(&v.mii_edit_applet_mode),
          static_cast<const Settings::BasicSetting*>(&v.web_applet_mode),
          static_cast<const Settings::BasicSetting*>(&v.shop_applet_mode),
          static_cast<const Settings::BasicSetting*>(&v.photo_viewer_applet_mode),
          static_cast<const Settings::BasicSetting*>(&v.offline_web_applet_mode),
          static_cast<const Settings::BasicSetting*>(&v.login_share_applet_mode),
          static_cast<const Settings::BasicSetting*>(&v.wifi_web_auth_applet_mode),
          static_cast<const Settings::BasicSetting*>(&v.my_page_applet_mode)}) {
        add(*applet, {.level = Advanced});
    }
    add(v.enable_overlay, {.level = Advanced});

    // Audio
    add(v.sink_id, {.level = Advanced});
    add(v.audio_output_device_id, {});
    add(v.audio_input_device_id, {});
    add(v.audio_muted, {});
    add(v.volume, {});
    add(ui.mute_when_in_background, {});
    add(v.sound_index, {.level = Advanced});

    // Core
    add(v.use_multi_core, {.level = Expert, .effects = {Faster}});
    add(v.memory_layout_mode, {.level = Advanced});
    add(v.speed_limit, {});
    add(v.turbo_speed_limit, {});
    add(v.slow_speed_limit, {});
    add(v.sync_core_speed, {.level = Advanced});

    // CPU
    add(v.cpu_accuracy, {.level = Expert});
    add(v.cpu_backend, {.level = Expert});
    add(v.cpu_clock, {.level = Advanced});
    add(v.cpu_ticks, {.level = Expert, .effects = {Faster, MayFreeze}});
    add(v.cpuopt_unsafe_host_mmu, {.level = Expert, .effects = {Faster}});
    add(v.cpuopt_unsafe_unfuse_fma, {.level = Expert, .effects = {Faster, MayCauseGlitches}});
    add(v.cpuopt_unsafe_reduce_fp_error, {.level = Expert, .effects = {Faster, MayCauseGlitches}});
    add(v.cpuopt_unsafe_ignore_standard_fpcr,
        {.level = Expert, .effects = {Faster, MayCauseGlitches}});
    add(v.cpuopt_unsafe_inaccurate_nan, {.level = Expert, .effects = {Faster, MayCauseGlitches}});
    add(v.cpuopt_unsafe_fastmem_check, {.level = Expert, .effects = {Faster, MayFreeze}});
    add(v.cpuopt_unsafe_ignore_global_monitor, {.level = Expert, .effects = {Faster, MayFreeze}});

    // Graphics
    add(v.renderer_backend, {});
    add(v.vulkan_device, {});
    add(v.resolution_setup, {.effects = {BetterImage, Slower, MoreVideoMemory}});
    add(v.scaling_filter, {});
    add(v.game_dlss_network, {.level = Advanced, .applies_at_boot = true});
    add(v.fsr_sharpening_slider, {});
    add(v.nvidia_dlss, {.level = Advanced, .applies_at_boot = true});
    add(v.anti_aliasing, {});
    add(v.oot_anti_aliasing, {.applies_at_boot = true, .program_ids = {OcarinaOfTime}});
    add(v.post_shader_enabled, {});
    add(v.fullscreen_mode, {});
    add(v.aspect_ratio, {});
    add(v.use_disk_shader_cache, {.level = Advanced, .effects = {LessStutter}});
    add(v.use_asynchronous_gpu_emulation, {.level = Expert, .effects = {Faster}});
    add(v.nvdec_emulation, {.level = Advanced});
    add(v.accelerate_astc, {});
    add(v.astc_recompression, {.level = Advanced});
    add(v.frame_pacing_mode, {.level = Advanced});
    add(v.vram_usage_mode, {});
    add(v.skip_cpu_inner_invalidation, {.level = Expert, .effects = {Faster, MayFreeze}});
    add(v.fps_limit, {});
    add(v.vsync_mode, {});
    add(v.frame_gen, {.effects = {MoreFrames, MoreLag, Slower}});
    add(v.frame_gen_type, {});
    add(v.frame_gen_multiplier, {.effects = {MoreFrames, MoreLag}});
    add(v.frame_gen_target_rate, {.level = Advanced});
    add(v.frame_gen_flow_scale_auto, {.level = Advanced});
    add(v.frame_gen_flow_scale, {.level = Advanced, .effects = {BetterImage, Slower}});
    add(v.frame_gen_queue_target, {.level = Advanced, .effects = {LessStutter, MoreLag}});
    add(v.frame_gen_dump_flow, {.level = Developer});

    // Advanced graphics
    add(v.sync_memory_operations, {.level = Expert, .effects = {Slower}});
    add(v.async_presentation, {.level = Advanced, .effects = {Faster}});
    add(v.renderer_force_max_clock, {.level = Advanced});
    add(v.max_anisotropy, {.level = Advanced, .effects = {BetterImage, Slower}});
    add(v.gpu_accuracy, {.level = Advanced});
    add(v.dma_accuracy, {.level = Expert});
    add(v.gpu_fence_behavior, {.level = Expert});
    add(v.native_compute_write_mark, {.level = Expert});
    add(v.use_asynchronous_shaders,
        {.level = Advanced, .effects = {LessStutter, MayCauseGlitches}});
    add(v.gpu_clock, {.level = Advanced});
    add(v.gpu_unswizzle_enabled, {.level = Expert, .effects = {Faster, MayCauseGlitches}});
    add(v.gpu_unswizzle_texture_size, {.level = Expert});
    add(v.gpu_unswizzle_stream_size, {.level = Expert});
    add(v.gpu_unswizzle_chunk_size, {.level = Expert});
    add(v.pin_gpu_threads, {.level = Advanced, .applies_at_boot = true});
    add(v.use_vulkan_driver_pipeline_cache, {.level = Advanced, .effects = {LessStutter}});
    add(v.native_async_draws, {});
    add(v.shader_preload_memory, {.effects = {LessStutter}, .applies_at_boot = true});
    add(v.native_pipeline_wait_ms, {.level = Advanced});
    add(v.enable_compute_pipelines, {.level = Expert, .effects = {MayFreeze}});
    add(v.use_reactive_flushing, {.level = Expert, .effects = {Faster}});
    add(v.use_video_framerate, {.level = Advanced});
    add(v.mkw_30fps, {.level = Advanced, .applies_at_boot = true, .program_ids = {MarioKartWorld}});
    add(v.mkw_dynamic_fps,
        {.level = Advanced, .applies_at_boot = true, .program_ids = {MarioKartWorld}});
    add(v.barrier_feedback_loops, {.level = Expert, .effects = {BetterImage, Slower}});
    add(v.enable_buffer_history, {.level = Expert});
    add(v.fix_bloom_effects, {.level = Expert});

    // Vulkan extensions
    add(v.dyna_state, {.level = Expert});
    add(v.vertex_input_dynamic_state, {.level = Expert, .effects = {LessStutter}});
    add(v.sample_shading, {.level = Expert, .effects = {BetterImage, Slower}});

    // System
    add(v.rng_seed, {.level = Advanced});
    add(v.device_name, {.level = Advanced});
    add(v.program_args, {.level = Developer});
    add(v.custom_rtc, {.level = Advanced});
    add(v.custom_rtc_offset, {.level = Advanced});
    add(v.language_index, {});
    add(v.region_index, {});
    add(v.time_zone_index, {});
    add(v.use_docked_mode, {});

    // Interface
    add(ui.select_user_on_boot, {});
    add(ui.pause_when_in_background, {});
    add(ui.confirm_before_stopping, {});
    add(ui.hide_mouse, {});
    add(ui.check_for_updates, {});
    add(ui.enable_gamemode, {.level = Advanced});
#ifdef __unix__
    add(ui.gui_force_x11, {.level = Advanced});
#endif
#ifdef __linux__
    add(ui.desktop_integration, {});
#endif

    return help;
}

QString PresetName(Settings::PerformancePreset preset) {
    const auto& tr = Translate;
    switch (preset) {
    case Settings::PerformancePreset::Performance:
        return tr("Performance");
    case Settings::PerformancePreset::Balanced:
        return tr("Balanced");
    case Settings::PerformancePreset::Quality:
        return tr("Quality");
    case Settings::PerformancePreset::Ultra:
        return tr("Ultra");
    }
    return {};
}

QString GameName(u64 program_id) {
    switch (program_id) {
    case OcarinaOfTime:
        return QStringLiteral("The Legend of Zelda: Ocarina of Time");
    case MarioKartWorld:
        return QStringLiteral("Mario Kart World");
    default:
        return QStringLiteral("%1").arg(program_id, 16, 16, QLatin1Char('0')).toUpper();
    }
}

Settings::BasicSetting* FindSetting(u32 id) {
    const auto has_id = [id](const Settings::BasicSetting* setting) { return setting->Id() == id; };
    for (const Settings::Linkage* linkage :
         {&Settings::values.linkage, &UISettings::values.linkage}) {
        for (const auto& [category, settings] : linkage->by_category) {
            const auto it = std::ranges::find_if(settings, has_id);
            if (it != settings.end()) {
                return *it;
            }
        }
    }
    return nullptr;
}

QString FormatSettingValue(const Settings::BasicSetting& setting, const std::string& value,
                           const ComboboxTranslationMap& enums) {
    const auto& tr = Translate;
    // The Graphics page names VSync's modes itself, from what the device offers, so they are not
    // in `enums`; the same words.
    if (setting.IsEnum() &&
        setting.EnumIndex() == Settings::EnumMetadata<Settings::VSyncMode>::Index() &&
        !enums.contains(setting.EnumIndex())) {
        switch (static_cast<Settings::VSyncMode>(QString::fromStdString(value).toUInt())) {
        case Settings::VSyncMode::Immediate:
            return QStringLiteral("Immediate (%1)")
                .arg(QCoreApplication::translate("ConfigureGraphics", "VSync Off"));
        case Settings::VSyncMode::Mailbox:
            return QStringLiteral("Mailbox");
        case Settings::VSyncMode::Fifo:
            return QStringLiteral("FIFO (%1)")
                .arg(QCoreApplication::translate("ConfigureGraphics", "VSync On"));
        case Settings::VSyncMode::FifoRelaxed:
            return QStringLiteral("FIFO Relaxed");
        }
    }
    if (setting.IsEnum()) {
        if (const auto it = enums.find(setting.EnumIndex()); it != enums.end()) {
            bool ok = false;
            const u32 index = QString::fromStdString(value).toUInt(&ok);
            for (const auto& [id, text] : it->second) {
                if (ok && id == index) {
                    return text;
                }
            }
        }
    }
    if (value == "true") {
        return tr("on");
    }
    if (value == "false") {
        return tr("off");
    }
    QString text = QString::fromStdString(value);
    if ((setting.Specialization() & Settings::Specialization::Percentage) != 0) {
        text += QLatin1Char('%');
    }
    return text;
}

QString SettingHelpHtml(const Settings::BasicSetting& setting, const SettingHelpContext& context) {
    const auto& tr = Translate;
    QString name = QString::fromStdString(setting.GetLabel());
    QString description;
    if (const auto it = context.names.find(setting.Id()); it != context.names.end()) {
        name = it->second.first.trimmed();
        description = it->second.second;
    }
    if (name.endsWith(QLatin1Char(':'))) {
        name.chop(1);
    }
    const auto help_it = context.help.find(setting.Id());
    const SettingHelp help = help_it != context.help.end() ? help_it->second : SettingHelp{};

    QString html;
    if (!name.isEmpty()) {
        html += QLatin1String(ParagraphStart) + QStringLiteral("<b>") + name.toHtmlEscaped() +
                QStringLiteral("</b>");
        if (const QString level = LevelText(help.level); !level.isEmpty()) {
            html += QStringLiteral(" &nbsp;<i>") + level.toHtmlEscaped() + QStringLiteral("</i>");
        }
        html += QStringLiteral("</p>");
    }
    if (!description.isEmpty()) {
        html += Paragraph(description);
    }

    QStringList facts;
    if (!help.effects.empty()) {
        QStringList effects;
        for (const SettingEffect effect : help.effects) {
            effects << EffectText(effect);
        }
        const QString when = setting.TypeId() == "bool" ? tr("When on:") : tr("Higher:");
        facts << when + QLatin1Char(' ') + effects.join(QStringLiteral(" · "));
    }
    QStringList notes;
    if (IsGameSetting(setting) && (help.applies_at_boot || !setting.RuntimeModifiable())) {
        notes << tr("Takes effect when a game starts.");
    }
    if (!help.program_ids.empty()) {
        QStringList games;
        for (const u64 program_id : help.program_ids) {
            games << GameName(program_id);
        }
        notes << tr("Only acts on %1.").arg(games.join(QStringLiteral(", ")));
    }
    if (!notes.isEmpty()) {
        facts << notes.join(QLatin1Char(' '));
    }
    QStringList values;
    // The default of a list filled at run time (devices) is an index, and a date's is a number.
    const u32 type = setting.Specialization() & Settings::SpecializationTypeMask;
    const bool readable_default =
        type != Settings::Specialization::RuntimeList && type != Settings::Specialization::Time;
    if (const QString value = FormatSettingValue(setting, setting.DefaultToString(), context.enums);
        readable_default && !value.isEmpty()) {
        values << tr("Default: %1").arg(value);
    }
    if (context.host != nullptr && IsGameSetting(setting)) {
        const auto preset = Settings::RecommendedPreset(*context.host);
        for (const Settings::PresetEntry& entry :
             Settings::PerformancePresetEntries(Settings::values, preset, *context.host)) {
            if (entry.setting == &setting) {
                values << tr("Recommended for this computer (%1 preset): %2")
                              .arg(PresetName(preset),
                                   FormatSettingValue(setting, entry.value, context.enums));
                break;
            }
        }
    }
    if (!values.isEmpty()) {
        facts << values.join(QStringLiteral(" · "));
    }
    for (const QString& fact : facts) {
        html += Paragraph(fact);
    }
    return html;
}

std::vector<GoalSection> GraphicsSections() {
    auto& v = Settings::values;
    using enum GoalPage;
    return {
        {Image, QT_TRANSLATE_NOOP("ConfigurationShared", "Graphics card"),
         {v.renderer_backend.Id(), v.vulkan_device.Id()}},
        {Image, QT_TRANSLATE_NOOP("ConfigurationShared", "Resolution and upscaling"),
         {v.resolution_setup.Id(), v.scaling_filter.Id(), v.fsr_sharpening_slider.Id(),
          v.game_dlss_network.Id(), v.nvidia_dlss.Id()}},
        {Image, QT_TRANSLATE_NOOP("ConfigurationShared", "Picture"),
         {v.anti_aliasing.Id(), v.oot_anti_aliasing.Id(), v.max_anisotropy.Id(),
          v.post_shader_enabled.Id(), v.aspect_ratio.Id()}},
        {Image, QT_TRANSLATE_NOOP("ConfigurationShared", "Frame rate and display"),
         {v.vsync_mode.Id(), v.fps_limit.Id(), v.frame_gen.Id(), v.frame_gen_type.Id(),
          v.frame_gen_multiplier.Id(), v.frame_gen_target_rate.Id(),
          v.frame_gen_flow_scale_auto.Id(), v.frame_gen_flow_scale.Id(),
          v.frame_gen_queue_target.Id(), v.frame_gen_dump_flow.Id(), v.mkw_30fps.Id(),
          v.mkw_dynamic_fps.Id(), v.fullscreen_mode.Id()}},
        {Smoothness, QT_TRANSLATE_NOOP("ConfigurationShared", "Shaders"),
         {v.use_asynchronous_shaders.Id(), v.native_async_draws.Id(),
          v.native_pipeline_wait_ms.Id(), v.shader_preload_memory.Id(),
          v.use_disk_shader_cache.Id(), v.use_vulkan_driver_pipeline_cache.Id()}},
        {Smoothness, QT_TRANSLATE_NOOP("ConfigurationShared", "Textures and video memory"),
         {v.accelerate_astc.Id(), v.astc_recompression.Id(), v.vram_usage_mode.Id()}},
        {Smoothness, QT_TRANSLATE_NOOP("ConfigurationShared", "GPU"),
         {v.gpu_accuracy.Id(), v.frame_pacing_mode.Id(), v.async_presentation.Id(),
          v.renderer_force_max_clock.Id(), v.pin_gpu_threads.Id(), v.nvdec_emulation.Id(),
          v.use_video_framerate.Id()}},
        {GraphicsEmulation, QT_TRANSLATE_NOOP("ConfigurationShared", "GPU emulation accuracy"),
         {v.dma_accuracy.Id(), v.gpu_fence_behavior.Id(), v.native_compute_write_mark.Id(),
          v.sync_memory_operations.Id(), v.use_reactive_flushing.Id(),
          v.barrier_feedback_loops.Id(), v.enable_buffer_history.Id(),
          v.use_asynchronous_gpu_emulation.Id(), v.enable_compute_pipelines.Id()}},
        {GraphicsEmulation, QT_TRANSLATE_NOOP("ConfigurationShared", "Workarounds"),
         {v.skip_cpu_inner_invalidation.Id(), v.fix_bloom_effects.Id(),
          v.gpu_unswizzle_enabled.Id(), v.gpu_unswizzle_texture_size.Id(),
          v.gpu_unswizzle_stream_size.Id(), v.gpu_unswizzle_chunk_size.Id()}},
        {GraphicsEmulation, QT_TRANSLATE_NOOP("ConfigurationShared", "Vulkan extensions"),
         {v.dyna_state.Id(), v.vertex_input_dynamic_state.Id(), v.sample_shading.Id()}},
    };
}

QString SearchKey(const QString& text) {
    QString key;
    const QString decomposed = text.normalized(QString::NormalizationForm_KD);
    key.reserve(decomposed.size());
    for (const QChar c : decomposed) {
        const auto category = c.category();
        if (category == QChar::Mark_NonSpacing || category == QChar::Mark_SpacingCombining ||
            category == QChar::Mark_Enclosing) {
            continue;
        }
        key += c.toCaseFolded();
    }
    return key;
}

bool MatchesSearch(const QString& text, const QString& query) {
    // Words start where a letter or digit follows anything else.
    static const QRegularExpression separators(QStringLiteral("[^\\p{L}\\p{N}]+"));
    const QStringList words = SearchKey(text).split(separators, Qt::SkipEmptyParts);
    const QStringList wanted = SearchKey(query).split(separators, Qt::SkipEmptyParts);
    // Each word typed starts a word of the text, so "tema" doesn't find "sistema".
    return !wanted.isEmpty() && std::ranges::all_of(wanted, [&](const QString& prefix) {
        return std::ranges::any_of(words,
                                   [&](const QString& word) { return word.startsWith(prefix); });
    });
}

QString ControlHelpHtml(const QString& name, const QString& description) {
    QString html;
    QString title = name.trimmed();
    title.remove(QLatin1Char('&'));
    if (title.endsWith(QLatin1Char(':'))) {
        title.chop(1);
    }
    if (!title.isEmpty()) {
        html += QLatin1String(ParagraphStart) + QStringLiteral("<b>") + title.toHtmlEscaped() +
                QStringLiteral("</b></p>");
    }
    // Some tooltips are written as rich text already.
    if (Qt::mightBeRichText(description)) {
        return html + description;
    }
    return html + Paragraph(description);
}

} // namespace ConfigurationShared

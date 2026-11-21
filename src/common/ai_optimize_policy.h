// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <string_view>

namespace Common::AiOptimize {

struct Option {
    std::string_view key;
    std::string_view values;
    std::string_view purpose;
    bool at_boot = false;
    bool switch2_only = false;
    bool ocarina_only = false;
};

// Only these settings may be written by an AI proposal. Values are the exact strings accepted by
// the settings serializer; the model never gets to choose a key or value outside this catalog.
inline constexpr std::array Options{
    Option{"resolution_setup", "0|1|2|3|4|5",
           "Render scale: 0=quarter, 1=half, 2=three quarters, 3=native, 4=1.25x, 5=1.5x; lower saves GPU time and VRAM", true},
    Option{"scaling_filter", "6|15", "6=FSR, 15=FSR3 upscaling", true},
    Option{"anti_aliasing", "0|1|2", "0=off, 1=FXAA, 2=SMAA; smoothing costs GPU time", true},
    Option{"vram_usage_mode", "0|1", "0=conservative, 1=aggressive VRAM use", true},
    Option{"accelerate_astc", "0|1|2", "ASTC texture decoding: 0=CPU, 1=GPU, 2=asynchronous CPU", true},
    Option{"astc_recompression", "0|1|2", "Texture storage: 0=uncompressed, 1=BC1, 2=BC3; compression can save VRAM", true},
    Option{"gpu_accuracy", "0|1", "0=fast GPU accuracy, 1=high accuracy for graphics problems", true},
    Option{"use_asynchronous_shaders", "false|true", "false=wait for shaders, true=build asynchronously; true can briefly omit objects", true},
    Option{"use_disk_shader_cache", "false|true", "false=off, true=reuse shaders across sessions", true},
    Option{"use_vulkan_driver_pipeline_cache", "false|true", "false=off, true=reuse Vulkan driver pipelines", true},
    Option{"pin_gpu_threads", "false|true", "false=default scheduler, true=pin GPU threads to performance cores", true,
           true},
    Option{"native_async_draws", "0|1", "0=wait for new shaders, 1=bounded draw skips to reduce stalls; may briefly omit objects", true,
           true},
    Option{"shader_preload_memory", "0|1", "0=off, 1=automatically preload known shaders in system RAM", true, true},
    Option{"oot_anti_aliasing", "0|1", "0=Ocarina's native image network, 1=skip its sharpening pass", true,
           true, true},
};

constexpr const Option* FindOption(std::string_view key) {
    for (const Option& option : Options) {
        if (option.key == key) {
            return &option;
        }
    }
    return nullptr;
}

constexpr bool ContainsValue(std::string_view choices, std::string_view value) {
    while (!choices.empty()) {
        const auto end = choices.find('|');
        const auto item = choices.substr(0, end);
        if (item == value) {
            return true;
        }
        if (end == std::string_view::npos) {
            break;
        }
        choices.remove_prefix(end + 1);
    }
    return false;
}

constexpr bool ValidChange(std::string_view key, std::string_view value, bool switch2,
                           bool ocarina) {
    const Option* option = FindOption(key);
    return option != nullptr && ContainsValue(option->values, value) &&
           (!option->switch2_only || switch2) && (!option->ocarina_only || ocarina);
}

enum class Provider : unsigned { OpenAI = 0, Anthropic = 1 };

struct Model {
    std::string_view id;
    std::string_view note; ///< Shown beside the id in the model selector
};

// The main models of each provider, first the default. The selector is editable and can load the
// provider's own list, so a model released later is one click away.
inline constexpr std::array OpenAIModels{
    Model{"gpt-6.2-sol", "most capable"},
    Model{"gpt-6.2-astra", "newest"},
    Model{"gpt-5.6-terra", "balance of cost and capability"},
    Model{"gpt-5.6-luna", "lowest cost"},
};
inline constexpr std::array AnthropicModels{
    Model{"claude-opus-5-5", "recommended"},
    Model{"claude-sonnet-5-5", "faster, lower cost"},
    Model{"claude-haiku-5-5", "lowest cost"},
    Model{"claude-fable-5-2", "most capable, highest cost"},
};

// Where the user creates an API key for the provider.
constexpr std::string_view KeyPageUrl(Provider provider) {
    return provider == Provider::Anthropic ? "https://platform.claude.com/settings/keys"
                                           : "https://platform.openai.com/api-keys";
}

constexpr std::string_view DefaultModel(Provider provider) {
    return provider == Provider::Anthropic ? AnthropicModels[0].id : OpenAIModels[0].id;
}

constexpr bool Contains(std::string_view text, std::string_view part) {
    return text.find(part) != std::string_view::npos;
}

// Whether a model id from the provider's model list can answer a text request with structured
// output: the lists also hold embedding, audio, image and moderation models.
constexpr bool IsTextModel(Provider provider, std::string_view id) {
    if (id.empty() || id.size() > 64) {
        return false;
    }
    if (provider == Provider::Anthropic) {
        return id.starts_with("claude-");
    }
    if (!id.starts_with("gpt-")) {
        return false;
    }
    for (const std::string_view other : {"audio", "realtime", "transcribe", "tts", "image",
                                         "search", "embedding", "moderation", "instruct"}) {
        if (Contains(id, other)) {
            return false;
        }
    }
    return true;
}

// Whether Anthropic's server-side refusal fallback applies to the model: the current Opus and
// Sonnet and Fable 5.2 accept it, Haiku and older models don't.
constexpr bool SupportsRefusalFallback(std::string_view id) {
    return id == "claude-opus-5-5" || id == "claude-sonnet-5-5" || id == "claude-fable-5-2";
}

// What a stretch of play measured: before and after an AI change, in the same scene.
struct Measurement {
    double fps = 0.0;          ///< Average of the game's own frames
    double low_1pct_fps = 0.0; ///< The slowest 1 % of frames
    unsigned stutters = 0;
};

enum class Verdict { Better, Worse, NoClearChange, NotComparable };

// Whether `after` is clearly better or worse than `before`. A few percent either way is the
// noise between two runs of one scene, so it reads as no clear change.
constexpr Verdict Compare(const Measurement& before, const Measurement& after) {
    if (before.fps <= 0.0 || after.fps <= 0.0) {
        return Verdict::NotComparable;
    }
    const double fps = after.fps / before.fps;
    const double low = before.low_1pct_fps > 0.0 ? after.low_1pct_fps / before.low_1pct_fps : 1.0;
    const bool fewer_stutters = before.stutters >= 3 && after.stutters * 10 <= before.stutters * 7;
    const bool more_stutters = after.stutters >= 3 && after.stutters * 7 >= before.stutters * 10;
    if (fps <= 0.95 || low <= 0.85 || (more_stutters && fps < 1.05)) {
        return Verdict::Worse;
    }
    if ((fps >= 1.05 && low > 0.9) || (low >= 1.1 && fps > 0.95) || (fewer_stutters && fps > 0.95)) {
        return Verdict::Better;
    }
    return Verdict::NoClearChange;
}

} // namespace Common::AiOptimize

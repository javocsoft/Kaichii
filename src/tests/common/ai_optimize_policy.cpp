// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>

#include <catch2/catch_test_macros.hpp>

#include "common/ai_optimize_policy.h"
#include "common/settings.h"
#include "common/settings_enums.h"

TEST_CASE("AI optimizer accepts only catalogued values for the right game", "[common][ai_optimize]") {
    using Common::AiOptimize::ValidChange;

    CHECK(ValidChange("resolution_setup", "1", false, false));
    CHECK(ValidChange("shader_preload_memory", "1", true, false));
    CHECK(ValidChange("oot_anti_aliasing", "1", true, true));
    CHECK_FALSE(ValidChange("shader_preload_memory", "1", false, false));
    CHECK_FALSE(ValidChange("oot_anti_aliasing", "1", true, false));
    CHECK_FALSE(ValidChange("resolution_setup", "999", true, false));
    CHECK_FALSE(ValidChange("resolution_setup", "-1", true, false));
    CHECK_FALSE(ValidChange("resolution_setup", "1;rm -rf", true, false));
    CHECK_FALSE(ValidChange("cpuopt_unsafe_fastmem_check", "true", true, false));
    CHECK_FALSE(ValidChange("vram_usage_mode", "aggressive", true, false));
}

TEST_CASE("AI option numbers match the settings they can change", "[common][ai_optimize]") {
    using Common::AiOptimize::ContainsValue;
    using Common::AiOptimize::FindOption;

    CHECK(ContainsValue(FindOption("resolution_setup")->values,
                        std::to_string(static_cast<int>(Settings::ResolutionSetup::Res1_2X))));
    CHECK(ContainsValue(FindOption("resolution_setup")->values,
                        std::to_string(static_cast<int>(Settings::ResolutionSetup::Res3_2X))));
    CHECK_FALSE(ContainsValue(FindOption("resolution_setup")->values,
                              std::to_string(static_cast<int>(Settings::ResolutionSetup::Res2X))));
    CHECK(ContainsValue(FindOption("scaling_filter")->values,
                        std::to_string(static_cast<int>(Settings::ScalingFilter::Fsr))));
    CHECK(ContainsValue(FindOption("scaling_filter")->values,
                        std::to_string(static_cast<int>(Settings::ScalingFilter::Fsr3))));
    CHECK(ContainsValue(FindOption("vram_usage_mode")->values,
                        std::to_string(static_cast<int>(Settings::VramUsageMode::Conservative))));
    CHECK(ContainsValue(FindOption("astc_recompression")->values,
                        std::to_string(static_cast<int>(Settings::AstcRecompression::Bc3))));
    CHECK(ContainsValue(FindOption("gpu_accuracy")->values,
                        std::to_string(static_cast<int>(Settings::GpuAccuracy::High))));
    CHECK(ContainsValue(FindOption("accelerate_astc")->values,
                        std::to_string(static_cast<int>(Settings::AstcDecodeMode::CpuAsynchronous))));
    CHECK(ContainsValue(FindOption("shader_preload_memory")->values,
                        std::to_string(static_cast<int>(Settings::ShaderPreloadMemory::Auto))));
    CHECK_FALSE(ContainsValue(FindOption("shader_preload_memory")->values,
                              std::to_string(static_cast<int>(Settings::ShaderPreloadMemory::Gib8))));
    CHECK(ContainsValue(FindOption("native_async_draws")->values,
                        std::to_string(static_cast<int>(Settings::NativeAsyncDraws::Bounded))));
    CHECK_FALSE(ContainsValue(FindOption("native_async_draws")->values,
                              std::to_string(static_cast<int>(Settings::NativeAsyncDraws::Skip))));
    CHECK(ContainsValue(FindOption("oot_anti_aliasing")->values,
                        std::to_string(static_cast<int>(Settings::OotAntiAliasing::NoSharpening))));
}

TEST_CASE("Every AI setting exists and can be saved for one game", "[common][ai_optimize]") {
    for (const auto& option : Common::AiOptimize::Options) {
        const auto it = Settings::values.linkage.by_key.find(std::string{option.key});
        INFO(option.key);
        REQUIRE(it != Settings::values.linkage.by_key.end());
        CHECK(it->second->Switchable());
        CHECK(it->second->Save());
    }
}

TEST_CASE("AI assistant model lists keep only text models", "[common][ai_optimize]") {
    using Common::AiOptimize::DefaultModel;
    using Common::AiOptimize::IsTextModel;
    using Common::AiOptimize::Provider;

    CHECK(DefaultModel(Provider::Anthropic) == "claude-opus-5-5");
    CHECK(IsTextModel(Provider::OpenAI, DefaultModel(Provider::OpenAI)));
    for (const auto& model : Common::AiOptimize::AnthropicModels) {
        CHECK(IsTextModel(Provider::Anthropic, model.id));
    }
    for (const auto& model : Common::AiOptimize::OpenAIModels) {
        CHECK(IsTextModel(Provider::OpenAI, model.id));
    }

    // Provider lists hold more than text models.
    CHECK_FALSE(IsTextModel(Provider::OpenAI, "text-embedding-3-large"));
    CHECK_FALSE(IsTextModel(Provider::OpenAI, "gpt-6-audio"));
    CHECK_FALSE(IsTextModel(Provider::OpenAI, "gpt-6-realtime"));
    CHECK_FALSE(IsTextModel(Provider::OpenAI, "gpt-image-2"));
    CHECK_FALSE(IsTextModel(Provider::OpenAI, "dall-e-3"));
    CHECK_FALSE(IsTextModel(Provider::Anthropic, "gpt-6.2-sol"));
    CHECK_FALSE(IsTextModel(Provider::OpenAI, ""));
}

TEST_CASE("Refusal fallback is asked for only on models that take it", "[common][ai_optimize]") {
    using Common::AiOptimize::SupportsRefusalFallback;
    CHECK(SupportsRefusalFallback("claude-opus-5-5"));
    CHECK(SupportsRefusalFallback("claude-sonnet-5-5"));
    CHECK_FALSE(SupportsRefusalFallback("claude-haiku-5-5"));
    CHECK_FALSE(SupportsRefusalFallback("claude-opus-4-8"));
}

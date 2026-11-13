// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <vector>

#include "common/common_types.h"

namespace Common::AutoOptimize {

/// What a stretch of the running game measured.
struct Sample {
    double seconds = 0.0;
    double fps = 0.0;          ///< The game's own presents, not generated frames
    double low_1pct_fps = 0.0;
    size_t stutters = 0;
};

/// Device memory against what the driver lets this process use.
struct VideoMemory {
    u64 used = 0;
    u64 budget = 0;
    /// Allocated in system memory instead: where images go once video memory is full.
    u64 spilled = 0;
};

/// Everything the calibration measured and the settings it ran with. Resolution cannot change
/// while a game runs (the emulator hangs), so only frame generation is tried live.
struct Measurements {
    Sample base;
    /// With frame generation switched on for the trial; empty when it was on already or the
    /// renderer could not run it.
    std::optional<Sample> frame_gen_trial;
    /// Frames generated per second during the trial.
    double generated_fps = 0.0;
    std::optional<VideoMemory> memory;
    bool frame_gen_enabled = false;
    /// Index into Settings::ResolutionSetup; `lowest_resolution` is the smallest scale offered.
    u32 resolution = 0;
    u32 lowest_resolution = 0;
    /// Whether draws already skip while their shaders build (native_async_draws other than Wait).
    bool skips_unbuilt_draws = false;
};

enum class Action : u8 {
    EnableFrameGeneration,
    LowerResolution,       ///< One step down, at the next boot
    SkipUnbuiltDraws,      ///< native_async_draws = Bounded
};

enum class Reason : u8 {
    FrameGenerationCostsLittle, ///< The trial kept nearly all the real frames
    VideoMemoryFull,            ///< Much of the game's images live in system memory
    ManyStutters,
};

struct Recommendation {
    Action action;
    Reason reason;
    bool needs_restart = false;
};

/// Why frame generation is not recommended, for the summary.
enum class FrameGenerationVerdict : u8 {
    Recommended,
    AlreadyOn,
    NotTried,       ///< The renderer could not run it
    TooFewFrames,   ///< Under MinFpsForFrameGeneration real frames a second
    CostsTooMuch,   ///< The trial lost more than MaxFrameGenerationLoss of the real frames
    NotGenerating,  ///< It ran but generated next to nothing
};

/// Interpolating between frames further apart than this shows artefacts and input lag.
inline constexpr double MinFpsForFrameGeneration = 24.0;
/// Share of the real frame rate frame generation may cost.
inline constexpr double MaxFrameGenerationLoss = 0.07;
/// System memory use, as a share of the video memory budget, from which a lower resolution is
/// proposed. Device memory itself says little: the texture cache keeps it under the budget by
/// spilling. Ocarina of Time on a 4 GB card (3.5 GiB budget) settles at 0.7 GiB there at half
/// resolution (33 fps), 0.4 GiB at native (30 fps) and 2.1 GiB at 1.5x, where it runs at 7 fps.
/// Loading a save peaks near 2.9 GiB for a moment, so measure once the scene has settled.
inline constexpr double SpilledVideoMemoryShare = 0.5;
/// Stutters a minute from which draws should skip while their shaders build.
inline constexpr double ManyStuttersPerMinute = 20.0;

[[nodiscard]] constexpr FrameGenerationVerdict JudgeFrameGeneration(const Measurements& m) {
    if (m.frame_gen_enabled) {
        return FrameGenerationVerdict::AlreadyOn;
    }
    if (!m.frame_gen_trial) {
        return FrameGenerationVerdict::NotTried;
    }
    if (m.base.fps < MinFpsForFrameGeneration) {
        return FrameGenerationVerdict::TooFewFrames;
    }
    if (m.generated_fps < m.frame_gen_trial->fps * 0.5) {
        return FrameGenerationVerdict::NotGenerating;
    }
    if (m.frame_gen_trial->fps < m.base.fps * (1.0 - MaxFrameGenerationLoss)) {
        return FrameGenerationVerdict::CostsTooMuch;
    }
    return FrameGenerationVerdict::Recommended;
}

/// The changes worth proposing, most important first. Empty when the settings already fit.
[[nodiscard]] inline std::vector<Recommendation> Recommend(const Measurements& m) {
    std::vector<Recommendation> result;
    const bool memory_full =
        m.memory && m.memory->budget != 0 &&
        static_cast<double>(m.memory->spilled) >=
            static_cast<double>(m.memory->budget) * SpilledVideoMemoryShare;
    if (memory_full && m.resolution > m.lowest_resolution) {
        result.push_back({Action::LowerResolution, Reason::VideoMemoryFull, true});
    }
    if (JudgeFrameGeneration(m) == FrameGenerationVerdict::Recommended) {
        result.push_back(
            {Action::EnableFrameGeneration, Reason::FrameGenerationCostsLittle, false});
    }
    if (!m.skips_unbuilt_draws && m.base.seconds > 0.0 &&
        static_cast<double>(m.base.stutters) * 60.0 / m.base.seconds >= ManyStuttersPerMinute) {
        result.push_back({Action::SkipUnbuiltDraws, Reason::ManyStutters, false});
    }
    return result;
}

} // namespace Common::AutoOptimize

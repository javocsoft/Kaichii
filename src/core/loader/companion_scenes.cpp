// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/loader/companion_scenes.h"

#include <algorithm>
#include <mutex>
#include <string>

#include "common/logging.h"
#include "core/loader/loader.h"

namespace Loader {
namespace {

constexpr std::string_view ScenePrefix = "/Banc/GameBancParam/";
constexpr std::string_view SceneSuffix = ".game__banc__GameBancParam.bgyml";
constexpr std::string_view BakeScenePrefix = "/Bake/Scene/";
constexpr std::string_view BakeSceneSuffix = ".bkres.zs";

bool IsSceneName(std::string_view name) {
    return !name.empty() && name.find('/') == std::string_view::npos &&
           std::all_of(name.begin(), name.end(), [](char character) {
               return (character >= 'A' && character <= 'Z') ||
                      (character >= 'a' && character <= 'z') ||
                      (character >= '0' && character <= '9') || character == '_';
           });
}

} // namespace

void SceneTracker::Note(std::string_view scene) {
    std::lock_guard lock(mutex);
    if (last != scene) {
        last = scene;
        LOG_DEBUG(Loader, "Companion scene loaded: {}", scene);
    }
}

std::string SceneTracker::Last() const {
    std::lock_guard lock(mutex);
    return last;
}

std::string_view OcarinaSceneFromRomFSPath(std::string_view path) {
    if (path.starts_with(ScenePrefix) && path.ends_with(SceneSuffix) &&
        path.size() > ScenePrefix.size() + SceneSuffix.size()) {
        const auto name = path.substr(ScenePrefix.size(),
                                      path.size() - ScenePrefix.size() - SceneSuffix.size());
        return IsSceneName(name) ? name : std::string_view{};
    }
    if (path.starts_with(BakeScenePrefix) && path.ends_with(BakeSceneSuffix)) {
        const auto name_start = path.substr(BakeScenePrefix.size());
        const auto grid = name_start.find("_G_");
        if (grid != std::string_view::npos && grid > 0) {
            const auto name = name_start.substr(0, grid);
            const auto tile = name_start.substr(grid + 3,
                                                name_start.size() - grid - 3 - BakeSceneSuffix.size());
            if (IsSceneName(name) && !tile.empty() && tile.find('/') == std::string_view::npos &&
                std::all_of(tile.begin(), tile.end(), [](char character) {
                    return (character >= '0' && character <= '9') || character == '_';
                })) {
                return name;
            }
        }
    }
    return {};
}

std::string_view BreathOfTheWildSceneFromRomFSPath(std::string_view path) {
    // Shrines and Divine Beasts each load their own pack on entry. The open world streams its map
    // sections from Map/MainField, so those reads mean Link is back outside.
    constexpr std::string_view PackPrefix = "/Pack/";
    constexpr std::string_view PackSuffix = ".pack";
    if (path.starts_with("/Map/MainField/")) {
        return "MainField";
    }
    if (!path.starts_with(PackPrefix) || !path.ends_with(PackSuffix)) {
        return {};
    }
    const auto name = path.substr(PackPrefix.size(),
                                  path.size() - PackPrefix.size() - PackSuffix.size());
    if (name.size() == 10 && name.starts_with("Dungeon") &&
        std::all_of(name.begin() + 7, name.end(),
                    [](char character) { return character >= '0' && character <= '9'; })) {
        return name;
    }
    for (const std::string_view beast :
         {"RemainsElectric", "RemainsFire", "RemainsWater", "RemainsWind"}) {
        if (name == beast) {
            return name;
        }
    }
    return {};
}

FileSys::RomFSReadObserver CompanionSceneObserver(u64 program_id,
                                                  std::shared_ptr<SceneTracker> tracker) {
    std::string_view (*scene_of)(std::string_view) = nullptr;
    if (program_id == OcarinaSwitch2ProgramId) {
        scene_of = OcarinaSceneFromRomFSPath;
    } else if (program_id == BreathOfTheWildProgramId) {
        scene_of = BreathOfTheWildSceneFromRomFSPath;
    }
    if (!scene_of || !tracker) {
        return {};
    }
    return {[scene_of](std::string_view path) { return !scene_of(path).empty(); },
            [scene_of, tracker = std::move(tracker)](std::string_view path) {
                tracker->Note(scene_of(path));
            }};
}

} // namespace Loader

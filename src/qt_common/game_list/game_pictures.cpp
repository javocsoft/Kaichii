// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <system_error>

#include <QImageReader>
#include <QString>
#include <fmt/format.h>

#include "common/fs/path_util.h"
#include "qt_common/game_list/game_pictures.h"

namespace QtCommon {

namespace {

using Common::FS::EdenPath;

constexpr std::array Extensions{".png", ".jpg", ".jpeg", ".webp"};

std::filesystem::path Stem(EdenPath dir, u64 program_id) {
    return Common::FS::GetEdenPath(dir) / fmt::format("{:016X}", program_id);
}

QString ToQString(const std::filesystem::path& path) {
    return QString::fromStdString(Common::FS::PathToUTF8String(path));
}

bool HasTransparency(const QImage& picture) {
    if (!picture.hasAlphaChannel()) {
        return false;
    }
    const QImage argb = picture.convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < argb.height(); ++y) {
        const auto* line = reinterpret_cast<const QRgb*>(argb.constScanLine(y));
        for (int x = 0; x < argb.width(); ++x) {
            if (qAlpha(line[x]) < 255) {
                return true;
            }
        }
    }
    return false;
}

std::optional<std::filesystem::path> FindIn(EdenPath dir, u64 program_id) {
    if (program_id == 0) {
        return std::nullopt;
    }
    const std::filesystem::path stem = Stem(dir, program_id);
    for (const char* extension : Extensions) {
        std::filesystem::path path = stem;
        path += extension;
        std::error_code error;
        if (std::filesystem::is_regular_file(path, error)) {
            return path;
        }
    }
    return std::nullopt;
}

QImage LoadFrom(EdenPath dir, u64 program_id) {
    const auto path = FindIn(dir, program_id);
    if (!path) {
        return {};
    }
    QImageReader reader(ToQString(*path));
    reader.setAutoTransform(true);
    return reader.read();
}

bool RemoveFrom(EdenPath dir, u64 program_id) {
    if (program_id == 0) {
        return true;
    }
    const std::filesystem::path stem = Stem(dir, program_id);
    bool removed_all = true;
    for (const char* extension : Extensions) {
        std::filesystem::path path = stem;
        path += extension;
        std::error_code error;
        std::filesystem::remove(path, error);
        removed_all = removed_all && !error;
    }
    return removed_all;
}

/// Replaces the game's picture in `dir` with `picture`, written as `format` with `extension`.
bool StoreIn(EdenPath dir, u64 program_id, const QImage& picture, const char* extension,
             const char* format, int quality = -1) {
    std::error_code error;
    std::filesystem::create_directories(Common::FS::GetEdenPath(dir), error);
    if (error || !RemoveFrom(dir, program_id)) {
        return false;
    }
    std::filesystem::path path = Stem(dir, program_id);
    path += extension;
    return picture.save(ToQString(path), format, quality);
}

} // Anonymous namespace

namespace BoxArt {

std::optional<std::filesystem::path> Find(u64 program_id) {
    return FindIn(EdenPath::BoxArtDir, program_id);
}

QImage Load(u64 program_id) {
    return LoadFrom(EdenPath::BoxArtDir, program_id);
}

bool Save(u64 program_id, const QImage& picture) {
    if (program_id == 0 || picture.isNull()) {
        return false;
    }
    QImage stored = picture;
    if (stored.height() > MaxStoredHeight) {
        stored = stored.scaledToHeight(MaxStoredHeight, Qt::SmoothTransformation);
    }
    if (HasTransparency(stored)) {
        return StoreIn(EdenPath::BoxArtDir, program_id, stored, ".png", "PNG");
    }
    return StoreIn(EdenPath::BoxArtDir, program_id, stored.convertToFormat(QImage::Format_RGB32),
                   ".jpg", "JPEG", 92);
}

bool Remove(u64 program_id) {
    return RemoveFrom(EdenPath::BoxArtDir, program_id);
}

} // namespace BoxArt

namespace CustomIcon {

std::optional<std::filesystem::path> Find(u64 program_id) {
    return FindIn(EdenPath::CustomIconsDir, program_id);
}

std::vector<u8> ReadBytes(u64 program_id) {
    const auto path = Find(program_id);
    if (!path) {
        return {};
    }
    std::ifstream file(*path, std::ios::binary);
    return std::vector<u8>(std::istreambuf_iterator<char>(file), {});
}

QImage Load(u64 program_id) {
    return LoadFrom(EdenPath::CustomIconsDir, program_id);
}

QImage Squared(const QImage& picture) {
    if (picture.isNull()) {
        return {};
    }
    // A square from the middle; a tall picture keeps its upper part, where titles usually are.
    const int side = std::min(picture.width(), picture.height());
    const int x = (picture.width() - side) / 2;
    const int y = (picture.height() - side) / (picture.height() > picture.width() ? 3 : 2);
    return picture.copy(x, y, side, side)
        .convertToFormat(QImage::Format_ARGB32)
        .scaled(StoredSide, StoredSide, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

bool Save(u64 program_id, const QImage& picture) {
    if (program_id == 0 || picture.isNull()) {
        return false;
    }
    return StoreIn(EdenPath::CustomIconsDir, program_id, Squared(picture), ".png", "PNG");
}

bool Remove(u64 program_id) {
    return RemoveFrom(EdenPath::CustomIconsDir, program_id);
}

} // namespace CustomIcon

} // namespace QtCommon

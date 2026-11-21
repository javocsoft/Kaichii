// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <system_error>

#include <QImageReader>
#include <QString>
#include <fmt/format.h>

#include "common/fs/path_util.h"
#include "qt_common/game_list/box_art.h"

namespace QtCommon::BoxArt {

namespace {

constexpr std::array Extensions{".png", ".jpg", ".jpeg", ".webp"};

std::filesystem::path Stem(u64 program_id) {
    return Common::FS::GetEdenPath(Common::FS::EdenPath::BoxArtDir) /
           fmt::format("{:016X}", program_id);
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

QString ToQString(const std::filesystem::path& path) {
    return QString::fromStdString(Common::FS::PathToUTF8String(path));
}

} // Anonymous namespace

std::optional<std::filesystem::path> Find(u64 program_id) {
    if (program_id == 0) {
        return std::nullopt;
    }
    const std::filesystem::path stem = Stem(program_id);
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

QImage Load(u64 program_id) {
    const auto path = Find(program_id);
    if (!path) {
        return {};
    }
    QImageReader reader(ToQString(*path));
    reader.setAutoTransform(true);
    return reader.read();
}

bool Save(u64 program_id, const QImage& picture) {
    if (program_id == 0 || picture.isNull()) {
        return false;
    }
    QImage stored = picture;
    if (stored.height() > MaxStoredHeight) {
        stored = stored.scaledToHeight(MaxStoredHeight, Qt::SmoothTransformation);
    }
    const bool transparent = HasTransparency(stored);

    std::error_code error;
    std::filesystem::create_directories(Stem(program_id).parent_path(), error);
    if (error) {
        return false;
    }
    if (!Remove(program_id)) {
        return false;
    }
    std::filesystem::path path = Stem(program_id);
    path += transparent ? ".png" : ".jpg";
    return transparent ? stored.save(ToQString(path), "PNG")
                       : stored.convertToFormat(QImage::Format_RGB32)
                             .save(ToQString(path), "JPEG", 92);
}

bool Remove(u64 program_id) {
    if (program_id == 0) {
        return true;
    }
    const std::filesystem::path stem = Stem(program_id);
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

} // namespace QtCommon::BoxArt

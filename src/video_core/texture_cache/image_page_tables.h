// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <optional>
#include <utility>

namespace VideoCommon {

template <typename Image, typename TableStorage, typename FindStorageId>
[[nodiscard]] auto RegisteredImagePageTables(const Image& image, TableStorage& tables,
                                             FindStorageId&& find_storage_id)
    -> std::optional<std::pair<typename TableStorage::value_type*,
                               typename TableStorage::value_type*>> {
    const std::optional<size_t> storage_id = find_storage_id(image.address_space);
    if (!storage_id || *storage_id >= tables.size() / 2) {
        return std::nullopt;
    }
    return std::pair{&tables[*storage_id * 2], &tables[*storage_id * 2 + 1]};
}

} // namespace VideoCommon

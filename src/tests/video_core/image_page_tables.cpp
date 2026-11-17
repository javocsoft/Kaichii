// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <optional>
#include <unordered_map>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "video_core/texture_cache/image_page_tables.h"

TEST_CASE("Image removal uses the address space where it was registered",
          "[video_core][image_page_tables]") {
    struct Image {
        size_t address_space;
    };
    using PageTable = std::unordered_map<int, std::vector<int>>;
    std::array<PageTable, 4> tables;
    tables[0][7] = {11};
    tables[2][7] = {42};

    const Image image{.address_space = 9};
    const auto find_storage_id = [](size_t address_space) -> std::optional<size_t> {
        if (address_space == 3) {
            return 0;
        }
        if (address_space == 9) {
            return 1;
        }
        return std::nullopt;
    };

    const auto owner_tables =
        VideoCommon::RegisteredImagePageTables(image, tables, find_storage_id);
    REQUIRE(owner_tables);
    owner_tables->first->at(7).clear();
    REQUIRE(tables[2].at(7).empty());
    REQUIRE(tables[0].at(7) == std::vector<int>{11});
    REQUIRE(owner_tables->second == &tables[3]);

    REQUIRE_FALSE(VideoCommon::RegisteredImagePageTables(Image{12}, tables, find_storage_id));
}

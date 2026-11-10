// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <future>
#include <thread>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "video_core/nx2.h"

namespace {

using namespace Tegra;
using Ranges = std::vector<std::pair<u64, u64>>;

constexpr u64 Page = 0x1000;
constexpr u64 AddressSpaceEnd = u64{1} << 36;
constexpr u64 Region = 0x10'0000'0000;
constexpr u64 Block = 0x1'0000;

/// These are process-wide tables, reset as a new emulation session would.
struct ScopedTables {
    ScopedTables() {
        Reset();
    }
    ~ScopedTables() {
        Reset();
    }
    static void Reset() {
        Nx2ClearGpuWrittenPages();
        Nx2ClearSemaphoreRegions();
        Nx2ClearSparseRegions();
        Nx2SetSparseBacking(nullptr);
    }
};

} // namespace

TEST_CASE("GPU written pages cover exactly the bytes written", "[video_core][nx2_pages]") {
    ScopedTables tables;
    Nx2MarkGpuWritten(Page + Page - 1, 2);
    CHECK_FALSE(Nx2IsGpuWritten(0));
    CHECK(Nx2IsGpuWritten(Page));
    CHECK(Nx2IsGpuWritten(2 * Page));
    CHECK_FALSE(Nx2IsGpuWritten(3 * Page));
}

TEST_CASE("A write ending on a page boundary leaves the next page alone",
          "[video_core][nx2_pages]") {
    ScopedTables tables;
    Nx2MarkGpuWritten(Page, Page);
    CHECK(Nx2IsGpuWritten(Page));
    CHECK_FALSE(Nx2IsGpuWritten(2 * Page));
}

TEST_CASE("An empty write marks nothing", "[video_core][nx2_pages]") {
    ScopedTables tables;
    Nx2MarkGpuWritten(Page, 0);
    CHECK_FALSE(Nx2IsGpuWritten(Page));
}

TEST_CASE("GPU written pages stay inside the device address space", "[video_core][nx2_pages]") {
    ScopedTables tables;
    Nx2MarkGpuWritten(AddressSpaceEnd - Page, 16 * Page);
    CHECK(Nx2IsGpuWritten(AddressSpaceEnd - Page));
    CHECK_FALSE(Nx2IsGpuWritten(AddressSpaceEnd));
}

TEST_CASE("A new session forgets written pages", "[video_core][nx2_pages]") {
    ScopedTables tables;
    Nx2MarkGpuWritten(Page, 1);
    Nx2MarkCommandListPage(Page);
    REQUIRE(g_nx2_has_command_list_pages.load());
    Nx2ClearGpuWrittenPages();
    CHECK_FALSE(Nx2IsGpuWritten(Page));
    CHECK_FALSE(Nx2IsCommandListPage(Page));
    CHECK_FALSE(g_nx2_has_command_list_pages.load());
}

TEST_CASE("Command list pages ignore addresses past the address space", "[video_core][nx2_pages]") {
    ScopedTables tables;
    Nx2MarkCommandListPage(AddressSpaceEnd);
    CHECK_FALSE(g_nx2_has_command_list_pages.load());
    CHECK_FALSE(Nx2IsCommandListPage(AddressSpaceEnd));
}

TEST_CASE("Semaphore regions are unique and removable", "[video_core][nx2_pages]") {
    ScopedTables tables;
    const auto count = [] {
        std::size_t regions{};
        for (const auto& region : g_nx2_semaphore_regions) {
            regions += region.load() != 0;
        }
        return regions;
    };
    Nx2AddSemaphoreRegion(0x1000);
    Nx2AddSemaphoreRegion(0x1000);
    Nx2AddSemaphoreRegion(0x2000);
    CHECK(count() == 2);
    Nx2RemoveSemaphoreRegion(0x1000);
    CHECK(count() == 1);
    for (u64 i = 0; i < Nx2MaxSemaphoreRegions + 4; ++i) {
        Nx2AddSemaphoreRegion(0x10000 + i);
    }
    CHECK(count() == Nx2MaxSemaphoreRegions);
}

TEST_CASE("Sparse writes reach the handler only inside the span", "[video_core][nx2_pages]") {
    ScopedTables tables;
    int calls{};
    Nx2SetSparseBacking([&](u64, u64, const u8*) { ++calls; });
    Nx2AddSparseSpan(Region, 4 * Block);

    const u8 data{};
    Nx2BackSparseWrite(Region - 16, 16, &data);
    CHECK(calls == 0);
    Nx2BackSparseWrite(Region + 4 * Block, 16, &data);
    CHECK(calls == 0);
    Nx2BackSparseWrite(Region - 16, 17, &data);
    CHECK(calls == 1);
    Nx2BackSparseWrite(Region + 4 * Block - 1, 16, &data);
    CHECK(calls == 2);
}

TEST_CASE("Sparse write readiness asks only inside the span and never waits",
          "[video_core][nx2_pages]") {
    ScopedTables tables;
    int asks{};
    bool answer{};
    Nx2SetSparseBacking([](u64, u64, const u8*) {});
    Nx2AddSparseSpan(Region, 4 * Block);
    // Without a readiness handler every write is ready.
    CHECK(Nx2SparseWriteReady(Region, 16));

    Nx2SetSparseBacking([](u64, u64, const u8*) {}, [&](u64, u64) {
        ++asks;
        return answer;
    });
    Nx2AddSparseSpan(Region, 4 * Block);
    CHECK(Nx2SparseWriteReady(Region - 16, 16));
    CHECK(Nx2SparseWriteReady(Region + 4 * Block, 16));
    CHECK(Nx2SparseWriteReady(Region, 0));
    CHECK(asks == 0);
    CHECK_FALSE(Nx2SparseWriteReady(Region + Block, 16));
    answer = true;
    CHECK(Nx2SparseWriteReady(Region - 16, 17));
    CHECK(asks == 2);

    // Another thread mapping memory for a write holds the backing mutex: not ready, no wait.
    std::promise<void> locked;
    std::promise<void> release;
    std::thread writer{[&] {
        std::scoped_lock lk{g_nx2_sparse_backing_mutex};
        locked.set_value();
        release.get_future().wait();
    }};
    locked.get_future().wait();
    CHECK_FALSE(Nx2SparseWriteReady(Region, 16));
    release.set_value();
    writer.join();
    CHECK(asks == 2);
    CHECK(Nx2SparseWriteReady(Region, 16));
}

TEST_CASE("Sparse blocks are unknown until written", "[video_core][nx2_pages]") {
    ScopedTables tables;
    Nx2SparseRegionCreated(Region, 4 * Block, Block);
    CHECK_FALSE(Nx2SparseBlockIsWritten(Region, 1));
    Nx2SparseBlockWritten(Region, 1);
    CHECK(Nx2SparseBlockIsWritten(Region, 1));
    CHECK_FALSE(Nx2SparseBlockIsWritten(Region, 2));

    // Outside what is tracked, assume data: dropping it would be worse than backing it.
    CHECK(Nx2SparseBlockIsWritten(Region, 4));
    CHECK(Nx2SparseBlockIsWritten(Region + 0x1000'0000, 0));
    Nx2SparseRegionDestroyed(Region);
    CHECK(Nx2SparseBlockIsWritten(Region, 2));
}

TEST_CASE("Sparse data ranges merge written blocks", "[video_core][nx2_pages]") {
    ScopedTables tables;
    Nx2SparseRegionCreated(Region, 4 * Block, Block);
    Nx2SparseBlockWritten(Region, 1);
    Nx2SparseBlockWritten(Region, 2);

    CHECK(Nx2SparseDataRanges(Region, 4 * Block) == Ranges{{Block, 2 * Block}});
    // Offsets count from the address asked about, clipped to it.
    CHECK(Nx2SparseDataRanges(Region + Block + 0x100, 0x200) == Ranges{{0, 0x200}});
    CHECK(Nx2SparseDataRanges(Region + 0x100, Block) == Ranges{{Block - 0x100, 0x100}});
    CHECK(Nx2SparseDataRanges(Region, 0).empty());
    CHECK(Nx2SparseDataRanges(Region, 5 * Block).empty());
    CHECK(Nx2SparseDataRanges(Region - 1, 1).empty());
}

TEST_CASE("Sparse tiles the game maps replace written blocks", "[video_core][nx2_pages]") {
    ScopedTables tables;
    Nx2SparseRegionCreated(Region, 4 * Nx2SparseTileSize, Nx2SparseTileSize);
    Nx2SparseBlockWritten(Region, 0);

    Nx2SparseTilesMapped(Region, 2 * Nx2SparseTileSize, Nx2SparseTileSize, true);
    CHECK(Nx2SparseDataRanges(Region, 4 * Nx2SparseTileSize) ==
          Ranges{{2 * Nx2SparseTileSize, Nx2SparseTileSize}});

    Nx2SparseTilesMapped(Region, 2 * Nx2SparseTileSize, Nx2SparseTileSize, false);
    CHECK(Nx2SparseDataRanges(Region, 4 * Nx2SparseTileSize).empty());
}

TEST_CASE("Sparse tile maps stop at the end of the region", "[video_core][nx2_pages]") {
    ScopedTables tables;
    Nx2SparseRegionCreated(Region, 2 * Nx2SparseTileSize, Nx2SparseTileSize);
    Nx2SparseTilesMapped(Region, Nx2SparseTileSize, 8 * Nx2SparseTileSize, true);
    CHECK(Nx2SparseDataRanges(Region, 2 * Nx2SparseTileSize) ==
          Ranges{{Nx2SparseTileSize, Nx2SparseTileSize}});
}

TEST_CASE("Rendering into a sparse region counts as writing it", "[video_core][nx2_pages]") {
    ScopedTables tables;
    Nx2AddSparseSpan(Region, 4 * Block);
    Nx2SparseRegionCreated(Region, 4 * Block, Block);

    CHECK(Nx2InSparseRegion(Region, 4 * Block));
    CHECK_FALSE(Nx2InSparseRegion(Region, 4 * Block + 1));
    CHECK_FALSE(Nx2InSparseRegion(Region - 1, 1));
    CHECK_FALSE(Nx2InSparseRegion(Region, 0));

    Nx2SparseMarkWritten(Region + Block - 1, 2);
    CHECK(Nx2SparseDataRanges(Region, 4 * Block) == Ranges{{0, 2 * Block}});
}

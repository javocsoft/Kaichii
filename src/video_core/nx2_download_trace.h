// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/common_types.h"
#include "common/logging.h"

// EDEN_NX2_DOWNLOAD_TRACE: what the buffer cache copies back to guest memory at fences. Every
// 10 s it logs the GPU writes that queued the most bytes for download (by written range), the
// 64 KiB regions copied back the most and the pages whose guest CPU reads of GPU-written data
// flushed or marked the most, and the first 2000 of those reads (a Preflushable mark sends later
// GPU writes over the range to a download at every fence).
namespace VideoCommon::Nx2DownloadTrace {

inline const bool g_enabled = std::getenv("EDEN_NX2_DOWNLOAD_TRACE") != nullptr;

struct Totals {
    u64 bytes{};
    u64 count{};
};

struct State {
    std::unordered_map<u64, Totals> writes;  ///< (address << 24 | size) of the GPU write
    std::unordered_map<u64, Totals> regions; ///< 64 KiB region copied back
    std::unordered_map<u64, Totals> reads;   ///< Page a CPU read of GPU-written data started at
    std::chrono::steady_clock::time_point last{std::chrono::steady_clock::now()};
    u64 marks{};
};

inline State& Get() {
    static State state;
    return state;
}

enum class KeyKind { Write, Region, Page };

inline void LogTop(const char* title, const std::unordered_map<u64, Totals>& map, KeyKind kind) {
    std::vector<std::pair<u64, Totals>> sorted(map.begin(), map.end());
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) { return a.second.bytes > b.second.bytes; });
    u64 total = 0;
    for (const auto& entry : sorted) {
        total += entry.second.bytes;
    }
    LOG_INFO(HW_GPU, "Download trace, {} in 10 s: {} KiB over {} keys", title, total / 1024,
             sorted.size());
    for (size_t i = 0; i < (std::min<size_t>)(sorted.size(), 16); ++i) {
        const auto& [key, totals] = sorted[i];
        switch (kind) {
        case KeyKind::Write:
            LOG_INFO(HW_GPU, "  write [{:#x} +{:#x}): {} KiB in {}", key >> 24, key & 0xffffff,
                     totals.bytes / 1024, totals.count);
            break;
        case KeyKind::Region:
            LOG_INFO(HW_GPU, "  region {:#x}: {} KiB in {} copies", key << 16, totals.bytes / 1024,
                     totals.count);
            break;
        case KeyKind::Page:
            LOG_INFO(HW_GPU, "  page {:#x}: {} reads over {} KiB", key << 12, totals.count,
                     totals.bytes / 1024);
            break;
        }
    }
}

inline void MaybeReport() {
    auto& state = Get();
    const auto now = std::chrono::steady_clock::now();
    if (now - state.last < std::chrono::seconds{10}) {
        return;
    }
    state.last = now;
    LogTop("queued by GPU write", state.writes, KeyKind::Write);
    LogTop("copied back by region", state.regions, KeyKind::Region);
    LogTop("read by the CPU by page", state.reads, KeyKind::Page);
    state.writes.clear();
    state.regions.clear();
    state.reads.clear();
}

/// A GPU write over [address, address + size) queued `queued` bytes for the next fence.
inline void Queued(u64 address, u64 size, u64 queued) {
    auto& totals = Get().writes[(address << 24) | (std::min<u64>)(size, 0xffffff)];
    totals.bytes += queued;
    ++totals.count;
}

/// Buffer data copied back at a fence.
inline void Copied(u64 address, u64 size) {
    auto& totals = Get().regions[address >> 16];
    totals.bytes += size;
    ++totals.count;
    MaybeReport();
}

/// A guest CPU read of GPU-written [address, address + size) that flushes: the first one there,
/// or one that came before the download a fence took of the Preflushable range landed.
inline void Marked(u64 address, u64 size, bool preflushable) {
    auto& state = Get();
    if (state.marks++ < 2000) {
        LOG_INFO(HW_GPU, "Download trace: CPU read of GPU-written [{:#x} +{:#x}) {}", address,
                 size, preflushable ? "before its fence download landed" : "for the first time");
    }
    auto& totals = state.reads[address >> 12];
    totals.bytes += size;
    ++totals.count;
    // Native titles download nothing at fences, so Copied never reports for them.
    MaybeReport();
}

} // namespace VideoCommon::Nx2DownloadTrace

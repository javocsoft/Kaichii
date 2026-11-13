// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <chrono>

#include "common/common_types.h"

namespace Common::HardwareTest {

/// The machine the test's figures are compared against, where every preset was measured: an
/// RTX 3050 Laptop GPU (4 GB) with an i7-12700H. Ocarina of Time runs there at about 30 fps
/// outdoors at half resolution, limited by the game's main thread.
inline constexpr char ReferenceMachine[] = "RTX 3050 Laptop GPU, i7-12700H";

/// The CPU workload: integer arithmetic, loads and stores in a table that fits in the L1 cache,
/// and branches that depend on the data, the mix a recompiler's code runs. Returns a checksum so
/// the work can't be dropped; the same `rounds` give the same checksum on every machine.
[[nodiscard]] constexpr u64 CpuWorkload(u64 rounds, u64 seed = 0x9E3779B97F4A7C15ULL) {
    // 32 KiB: the first level data cache of current cores.
    std::array<u64, 4096> table{};
    u64 state = seed | 1;
    u64 checksum = 0;
    for (u64 round = 0; round < rounds; ++round) {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        u64& entry = table[(state ^ checksum) & (table.size() - 1)];
        if ((state >> 32) & 1) {
            entry += state * 0x2545F4914F6CDD1DULL;
        } else {
            entry ^= (state >> 11) + checksum;
        }
        checksum += entry >> 5;
        if ((checksum & 7) == 3) {
            checksum = (checksum << 1) | (checksum >> 63);
        }
    }
    return checksum ^ table[checksum & (table.size() - 1)];
}

/// Rounds of CpuWorkload a second, on the calling thread, measured for about `duration`.
[[nodiscard]] inline double MeasureCpuRate(std::chrono::milliseconds duration) {
    constexpr u64 Batch = 1 << 20;
    const auto begin = std::chrono::steady_clock::now();
    u64 rounds = 0;
    volatile u64 sink = 0;
    while (std::chrono::steady_clock::now() - begin < duration) {
        sink = sink + CpuWorkload(Batch, rounds + 1);
        rounds += Batch;
    }
    const double seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count();
    return static_cast<double>(rounds) / seconds;
}

/// CpuWorkload rounds a second on the reference machine's fastest core: 200 to 215 million over
/// eight runs (2026-11-13).
inline constexpr double ReferenceCpuRate = 210e6;
/// Fused multiply-add throughput the GPU test measures on the reference machine, in TFLOPS: 7.11
/// to 7.55 over eight runs, the card's peak (2048 cores at about 1.75 GHz). The test counts
/// single-issue FMAs, so a GPU whose peak counts dual issue (RDNA 3) measures about half of it,
/// which matches how games run on it better than the peak does.
inline constexpr double ReferenceGpuTflops = 7.3;

} // namespace Common::HardwareTest

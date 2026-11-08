// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>

#include "common/core_pinning.h"

namespace Common {

GpuThreadPinning PlanGpuThreadPinning(std::span<const LogicalProcessor> processors) {
    GpuThreadPinning plan;
    if (processors.empty()) {
        return plan;
    }
    const u8 fastest =
        std::ranges::max(processors, {}, &LogicalProcessor::efficiency_class).efficiency_class;
    std::vector<u32> cores;
    for (const LogicalProcessor& processor : processors) {
        if (processor.efficiency_class == fastest) {
            cores.push_back(processor.core);
        }
    }
    std::ranges::sort(cores);
    cores.erase(std::unique(cores.begin(), cores.end()), cores.end());
    plan.performance_cores = cores.size();
    if (cores.size() < MinCoresForGpuThreadPinning) {
        return plan;
    }
    const u32 front_core = cores[1];
    const u32 back_core = cores[2];
    for (const LogicalProcessor& processor : processors) {
        if (processor.efficiency_class != fastest) {
            continue;
        }
        if (processor.core == front_core) {
            plan.front.push_back(processor.id);
        } else if (processor.core == back_core) {
            plan.back.push_back(processor.id);
        } else {
            plan.guest.push_back(processor.id);
        }
    }
    return plan;
}

std::vector<u32> ParseCpuList(std::string_view list) {
    std::vector<u32> cpus;
    const auto number = [&](u32& value) {
        if (list.empty() || list.front() < '0' || list.front() > '9') {
            return false;
        }
        value = 0;
        while (!list.empty() && list.front() >= '0' && list.front() <= '9') {
            value = value * 10 + static_cast<u32>(list.front() - '0');
            if (value > 0xFFFF) {
                return false;
            }
            list.remove_prefix(1);
        }
        return true;
    };
    while (!list.empty()) {
        u32 first{};
        if (!number(first)) {
            break;
        }
        u32 last = first;
        if (!list.empty() && list.front() == '-') {
            list.remove_prefix(1);
            if (!number(last) || last < first) {
                break;
            }
        }
        for (u32 cpu = first; cpu <= last; ++cpu) {
            cpus.push_back(cpu);
        }
        if (list.empty() || list.front() != ',') {
            break;
        }
        list.remove_prefix(1);
    }
    return cpus;
}

} // namespace Common

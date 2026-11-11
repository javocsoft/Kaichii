// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>

#include "common/logging.h"
#include "core/hle/kernel/guest_profile.h"

namespace Kernel::GuestProfile {

bool Enabled() {
    static const bool enabled = [] {
        const char* value = std::getenv("EDEN_NX2_GUEST_PROFILE");
        return value != nullptr && value[0] != '0';
    }();
    return enabled;
}

void Table::AddOutside(u64 thread, u64 ns) {
    threads[thread].outside_ns += ns;
}

void Table::AddSvc(u64 thread, s32 priority, u32 svc, std::string_view detail, u64 ns) {
    Thread& entry = threads[thread];
    entry.priority = priority;
    entry.svc_ns += ns;
    ++entry.svc_count;
    Wait& wait = entry.waits[{svc, std::string{detail}}];
    wait.ns += ns;
    ++wait.count;
}

std::string Table::Format(double seconds, size_t max_threads, size_t max_waits,
                          bool per_second) const {
    const double scale = per_second && seconds > 0.0 ? 1.0 / seconds : 1.0;
    const char* unit = per_second ? " ms/s" : " ms";
    const auto ms = [scale](u64 ns) { return static_cast<double>(ns) / 1e6 * scale; };
    const auto count = [scale, per_second](u64 n) {
        return per_second ? fmt::format("{:.0f}/s", static_cast<double>(n) * scale)
                          : fmt::format("{}x", n);
    };
    std::vector<const std::pair<const u64, Thread>*> order;
    for (const auto& entry : threads) {
        order.push_back(&entry);
    }
    std::ranges::stable_sort(order, [](const auto* a, const auto* b) {
        return a->second.outside_ns > b->second.outside_ns;
    });
    std::string text;
    for (size_t i = 0; i < (std::min)(max_threads, order.size()); ++i) {
        const auto& [id, thread] = *order[i];
        text += fmt::format("  t{} p{}: outside SVCs {:.1f}{}, in SVCs {:.1f}{} ({})", id,
                            thread.priority, ms(thread.outside_ns), unit, ms(thread.svc_ns), unit,
                            count(thread.svc_count));
        std::vector<const std::pair<const std::pair<u32, std::string>, Wait>*> waits;
        for (const auto& wait : thread.waits) {
            waits.push_back(&wait);
        }
        std::ranges::stable_sort(waits, [](const auto* a, const auto* b) {
            return a->second.ns > b->second.ns;
        });
        for (size_t w = 0; w < (std::min)(max_waits, waits.size()); ++w) {
            const auto& [key, wait] = *waits[w];
            text += fmt::format("{} {}{}{}{} {:.1f}{} ({})", w == 0 ? ";" : ",", SvcName(key.first),
                                key.second.empty() ? "" : "[", key.second,
                                key.second.empty() ? "" : "]", ms(wait.ns), unit,
                                count(wait.count));
        }
        text += '\n';
    }
    return text;
}

std::string_view SvcName(u32 svc) {
    switch (svc) {
    case 0x01: return "SetHeapSize";
    case 0x06: return "QueryMemory";
    case 0x08: return "CreateThread";
    case 0x09: return "StartThread";
    case 0x0A: return "ExitThread";
    case 0x0B: return "SleepThread";
    case 0x0D: return "SetThreadPriority";
    case 0x0F: return "SetThreadCoreMask";
    case 0x11: return "SignalEvent";
    case 0x12: return "ClearEvent";
    case 0x16: return "CloseHandle";
    case 0x17: return "ResetSignal";
    case 0x18: return "WaitSynchronization";
    case 0x1A: return "ArbitrateLock";
    case 0x1B: return "ArbitrateUnlock";
    case 0x1C: return "WaitProcessWideKeyAtomic";
    case 0x1D: return "SignalProcessWideKey";
    case 0x21: return "SendSyncRequest";
    case 0x22: return "SendSyncRequestWithUserBuffer";
    case 0x29: return "GetInfo";
    case 0x34: return "WaitForAddress";
    case 0x35: return "SignalToAddress";
    case 0x43: return "ReplyAndReceive";
    case 0x80: return "SetGpuMemoryAttribute";
    default: return "svc";
    }
}

namespace {

struct State {
    std::mutex mutex;
    Table window;
    Table frame;
    std::unordered_map<u64, u64> last_exit_ns;
    std::unordered_map<const KServerSession*, std::string> session_names;
    std::chrono::steady_clock::time_point window_start = std::chrono::steady_clock::now();
};

State& GetState() {
    static State state;
    return state;
}

u64 SlowFrameNs() {
    static const u64 ns = [] {
        const char* value = std::getenv("EDEN_NX2_SLOW_FRAME_MS");
        const u64 ms = value != nullptr ? std::strtoull(value, nullptr, 10) : 0;
        return (ms != 0 ? ms : 60) * 1'000'000ULL;
    }();
    return ns;
}

} // Anonymous namespace

void OnSvc(u64 thread, s32 priority, u32 svc, std::string_view detail, u64 entered_ns,
           u64 left_ns) {
    State& state = GetState();
    std::scoped_lock lock{state.mutex};
    const auto [it, inserted] = state.last_exit_ns.try_emplace(thread, left_ns);
    if (!inserted) {
        const u64 outside = entered_ns > it->second ? entered_ns - it->second : 0;
        state.window.AddOutside(thread, outside);
        state.frame.AddOutside(thread, outside);
        it->second = left_ns;
    }
    const u64 inside = left_ns > entered_ns ? left_ns - entered_ns : 0;
    // The SVC an ID alone can't tell apart keeps its number.
    const std::string with_id =
        SvcName(svc) == "svc" ? fmt::format("{:#x}{}{}", svc, detail.empty() ? "" : " ", detail)
                              : std::string{detail};
    state.window.AddSvc(thread, priority, svc, with_id, inside);
    state.frame.AddSvc(thread, priority, svc, with_id, inside);
}

void NameSession(const KServerSession* session, std::string_view name) {
    State& state = GetState();
    std::scoped_lock lock{state.mutex};
    state.session_names[session] = std::string{name};
}

std::string SessionName(const KServerSession* session) {
    State& state = GetState();
    std::scoped_lock lock{state.mutex};
    const auto it = state.session_names.find(session);
    return it != state.session_names.end() ? it->second : std::string{};
}

void OnPresent(u64 present, u64 interval_ns) {
    State& state = GetState();
    std::string slow;
    std::string report;
    double window_seconds = 0.0;
    {
        std::scoped_lock lock{state.mutex};
        if (interval_ns > SlowFrameNs() && !state.frame.Empty()) {
            slow = state.frame.Format(static_cast<double>(interval_ns) / 1e9, 6, 4, false);
        }
        state.frame.Clear();
        const auto now = std::chrono::steady_clock::now();
        window_seconds = std::chrono::duration<double>(now - state.window_start).count();
        if (window_seconds >= 10.0) {
            report = state.window.Format(window_seconds, 8, 5, true);
            state.window.Clear();
            state.window_start = now;
        }
    }
    if (!slow.empty()) {
        LOG_INFO(Kernel, "Guest threads during slow frame {} ({:.1f} ms):\n{}", present,
                 static_cast<double>(interval_ns) / 1e6, slow);
    }
    if (!report.empty()) {
        LOG_INFO(Kernel, "Guest threads over the last {:.1f} s:\n{}", window_seconds, report);
    }
}

} // namespace Kernel::GuestProfile

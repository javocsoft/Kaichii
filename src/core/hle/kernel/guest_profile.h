// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <map>
#include <string>
#include <string_view>
#include <utility>

#include "common/common_types.h"

namespace Kernel {
class KServerSession;
}

// EDEN_NX2_GUEST_PROFILE=1: where the game's threads spend their time. Each SVC is timed from
// entry to exit on the thread that made it, and filed under the SVC and what it waited on (the
// service of an IPC request, the event, the lock or condition variable address). The time between
// a thread's SVCs is "outside SVCs": guest code running, or the thread ready but without a core.
// Every 10 s the busiest threads are logged, and so is each slow frame (EDEN_NX2_SLOW_FRAME_MS,
// 60 ms by default) with what the threads did since the previous present.
namespace Kernel::GuestProfile {

[[nodiscard]] bool Enabled();

/// The time of each thread, outside SVCs and in each SVC. The caller locks.
class Table {
public:
    void AddOutside(u64 thread, u64 ns);
    void AddSvc(u64 thread, s32 priority, u32 svc, std::string_view detail, u64 ns);

    /// The `max_threads` threads with the most time outside SVCs, each with its `max_waits`
    /// longest SVCs. As rates over `seconds` when `per_second`, else as totals.
    [[nodiscard]] std::string Format(double seconds, size_t max_threads, size_t max_waits,
                                     bool per_second) const;

    [[nodiscard]] bool Empty() const {
        return threads.empty();
    }

    void Clear() {
        threads.clear();
    }

private:
    struct Wait {
        u64 ns{};
        u64 count{};
    };
    struct Thread {
        s32 priority{-1};
        u64 outside_ns{};
        u64 svc_ns{};
        u64 svc_count{};
        std::map<std::pair<u32, std::string>, Wait> waits;
    };
    std::map<u64, Thread> threads;
};

[[nodiscard]] std::string_view SvcName(u32 svc);

/// From Svc::Call, with steady clock times in nanoseconds.
void OnSvc(u64 thread, s32 priority, u32 svc, std::string_view detail, u64 entered_ns,
           u64 left_ns);

/// The service an HLE server session answers for, named when it is registered.
void NameSession(const KServerSession* session, std::string_view name);
[[nodiscard]] std::string SessionName(const KServerSession* session);

/// At each present, `interval_ns` after the previous one.
void OnPresent(u64 present, u64 interval_ns);

} // namespace Kernel::GuestProfile

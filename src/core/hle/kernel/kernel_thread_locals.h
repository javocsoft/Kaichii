// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/assert.h"
#include "common/common_types.h"

namespace Kernel {
class KThread;
}

// The kernel's per host thread state: the guest thread running there and the host thread ID.
//
// Guest threads are fibers, and a fiber that blocks in an SVC resumes on whichever core thread
// schedules it next. A caller that inlined a thread_local read could keep the TLS block address
// across that switch and read the thread it left: with clang-cl and ThinLTO, Svc::Call took the core
// from the old address after a blocking SVC and loaded the results into another core's registers
// (every game aborted with Break 0xE401 on Windows). These are the only accesses, and never inline.
namespace Kernel::ThreadLocals {

YUZU_NO_INLINE KThread* CurrentThread();
YUZU_NO_INLINE void SetCurrentThread(KThread* thread);

YUZU_NO_INLINE u8 HostThreadId();
YUZU_NO_INLINE void SetHostThreadId(u8 id);

} // namespace Kernel::ThreadLocals

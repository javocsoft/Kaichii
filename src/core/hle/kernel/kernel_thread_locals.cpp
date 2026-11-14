// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/hle/kernel/kernel_thread_locals.h"

namespace Kernel::ThreadLocals {

namespace {

// Constinit and trivially destructible, so an access needs no TLS wrapper or guard.
constinit thread_local KThread* t_current_thread = nullptr;
constinit thread_local u8 t_host_thread_id = UINT8_MAX;

} // Anonymous namespace

KThread* CurrentThread() {
    return t_current_thread;
}

void SetCurrentThread(KThread* thread) {
    t_current_thread = thread;
}

u8 HostThreadId() {
    return t_host_thread_id;
}

void SetHostThreadId(u8 id) {
    t_host_thread_id = id;
}

} // namespace Kernel::ThreadLocals

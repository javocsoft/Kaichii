// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <memory>
#include <thread>
#include <catch2/catch_test_macros.hpp>

#include "common/fiber.h"
#include "core/hle/kernel/kernel_thread_locals.h"

// A guest thread blocked in an SVC resumes on another core thread. On Windows b36 (clang-cl,
// ThinLTO), Svc::Call kept the TLS address it read before blocking and loaded the SVC's results into
// the core it left, so every game aborted with Break 0xE401. A test build has no LTO and cannot
// reproduce that inlining; this checks the contract the kernel relies on: reads after a fiber moves
// see the thread it moved to.
TEST_CASE("Kernel thread locals follow a fiber to the thread that resumes it", "[kernel]") {
    using namespace Kernel::ThreadLocals;
    int thread_a_marker{};
    int thread_b_marker{};
    auto* const thread_a_current = reinterpret_cast<Kernel::KThread*>(&thread_a_marker);
    auto* const thread_b_current = reinterpret_cast<Kernel::KThread*>(&thread_b_marker);

    std::shared_ptr<Common::Fiber> thread_a;
    std::shared_ptr<Common::Fiber> thread_b;
    std::shared_ptr<Common::Fiber> guest;
    u8 id_before{};
    u8 id_after{};
    Kernel::KThread* current_before{};
    Kernel::KThread* current_after{};

    guest = std::make_shared<Common::Fiber>([&] {
        id_before = HostThreadId();
        current_before = CurrentThread();
        // Block: thread A goes on, and the fiber is resumed by thread B.
        Common::Fiber::YieldTo(guest, *thread_a);
        id_after = HostThreadId();
        current_after = CurrentThread();
        Common::Fiber::YieldTo(guest, *thread_b);
    });

    std::thread a{[&] {
        SetHostThreadId(1);
        SetCurrentThread(thread_a_current);
        thread_a = Common::Fiber::ThreadToFiber();
        Common::Fiber::YieldTo(thread_a, *guest);
        thread_a->Exit();
    }};
    a.join();

    std::thread b{[&] {
        SetHostThreadId(2);
        SetCurrentThread(thread_b_current);
        thread_b = Common::Fiber::ThreadToFiber();
        Common::Fiber::YieldTo(thread_b, *guest);
        thread_b->Exit();
    }};
    b.join();

    REQUIRE(id_before == 1);
    REQUIRE(current_before == thread_a_current);
    REQUIRE(id_after == 2);
    REQUIRE(current_after == thread_b_current);
}

TEST_CASE("Kernel thread locals start unset on a new host thread", "[kernel]") {
    using namespace Kernel::ThreadLocals;
    u8 id{};
    Kernel::KThread* current{};
    std::thread t{[&] {
        id = HostThreadId();
        current = CurrentThread();
    }};
    t.join();
    REQUIRE(id == UINT8_MAX);
    REQUIRE(current == nullptr);
}

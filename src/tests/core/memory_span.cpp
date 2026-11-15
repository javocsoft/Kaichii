// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "common/page_table.h"
#include "common/scope_exit.h"
#include "core/core.h"
#include "core/device_memory.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/kernel.h"
#include "core/memory.h"
#include "tests/core/system_test_context.h"

TEST_CASE("Guest memory spans check all pages and the exclusive address-space end", "[core][memory]") {
    ScopedSystemTestContext context;
    Core::System system;
    system.Initialize();
    Kernel::KProcess process{system.Kernel()};
    SCOPE_EXIT { system.Kernel().UnregisterKernelObject(&process); };
    auto& table = process.GetPageTable().GetImpl();
    table.Resize(20, 12);
    u8* const backing = system.DeviceMemory().buffer.BackingBasePointer();
    const auto map = [&](u64 page, u16 block, u8* host) {
        table.entries.GetAndFault(page).Store(false, Common::PageType::Memory, block,
                                              reinterpret_cast<uintptr_t>(host) - (page << 12));
    };
    auto& memory = process.GetMemory();
    memory.SetCurrentPageTable(process);

    map(4, 1, backing + 0x10000);
    map(5, 1, backing + 0x11000);
    map(6, 2, backing + 0x40000);
    CHECK(memory.GetSpan(0x4000, 0x2000) == backing + 0x10000);
    CHECK(memory.GetSpan(0x4000, 0x2001) == nullptr);

    map(8, 3, backing + 0x20000);
    map(9, 4, backing + 0x50000);
    map(10, 3, backing + 0x22000);
    CHECK(memory.GetSpan(0x8000, 0x2001) == nullptr);

    map(12, Common::PageTable::PageEntryData::INVALID_BLOCK_ID, backing + 0x30000);
    map(13, 5, backing + 0x31000);
    CHECK(memory.GetSpan(0xC000, 0x2000) == backing + 0x30000);

    const u64 end = u64{1} << table.GetAddressSpaceBits();
    CHECK(memory.GetSpan(end, 0) == nullptr);
    CHECK(memory.GetSpan(end, 1) == nullptr);
}

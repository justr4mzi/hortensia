// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdint>
#include <limits>

#include <catch2/catch_test_macros.hpp>

#include "core/hle/kernel/address_arbiter_read.h"
#include "core/hle/kernel/k_memory_layout.h"

namespace {
struct TestMemory {
    bool mapped = true;
    int checks = 0;
    int reads = 0;

    bool IsValidVirtualAddressRange(std::uint64_t, std::uint64_t) {
        ++checks;
        return mapped;
    }
    std::uint64_t Read64(std::uint64_t) {
        ++reads;
        return 42;
    }
};
} // namespace

TEST_CASE("64-bit address arbiter reads reject wrapping ranges", "[core][address_arbiter]") {
    constexpr auto max = std::numeric_limits<std::uint64_t>::max();
    TestMemory memory;
    std::int64_t value = -1;

    // This address is 8-byte aligned and lies above the kernel virtual address range.
    CHECK((max - 7) % 8 == 0);
    CHECK_FALSE(Kernel::IsKernelAddress(max - 7));
    CHECK_FALSE(Kernel::Detail::ReadArbiterValue64(memory, max - 7, value));
    CHECK(memory.checks == 0);
    CHECK(memory.reads == 0);
    CHECK(value == -1);

    // The last non-wrapping range still reaches the mapping check.
    memory.mapped = false;
    CHECK_FALSE(Kernel::Detail::ReadArbiterValue64(memory, max - 8, value));
    CHECK(memory.checks == 1);
    CHECK(memory.reads == 0);

    memory.mapped = true;
    CHECK(Kernel::Detail::ReadArbiterValue64(memory, 0x1000, value));
    CHECK(value == 42);
    CHECK(memory.checks == 2);
    CHECK(memory.reads == 1);
}

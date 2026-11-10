// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <functional>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/gpu_dirty_memory_manager.h"

namespace {
using Written = std::vector<std::pair<PAddr, size_t>>;
}

TEST_CASE("GPUDirtyMemoryManager: writes are reported again after each gather",
          "[core][gpu_dirty_memory]") {
    // The GPU holds a reported page back until a gather applies its writes, so the first write to
    // the page after that gather has to be reported again.
    Core::GPUDirtyMemoryManager manager;
    Core::GPUDirtyMemoryManager::ReportedPage last;
    Written gathered;
    std::function<void(PAddr, size_t)> gather = [&](PAddr addr, size_t size) {
        gathered.emplace_back(addr, size);
    };
    int reports = 0;
    const auto report = [&] {
        ++reports;
        return true;
    };
    constexpr PAddr addr = 0x10040;
    manager.CollectWrite(addr, 0x40, addr >> 12, last, report);
    manager.CollectWrite(addr, 0x40, addr >> 12, last, report);
    CHECK(reports == 1);
    manager.Gather(gather);
    CHECK(gathered == Written{{addr, 0x40}});
    manager.CollectWrite(addr, 0x40, addr >> 12, last, report);
    CHECK(reports == 2);
    manager.Gather(gather);

    // Each gather that took writes moves the generation; one with nothing collected does not.
    manager.CollectWrite(addr, 0x40, addr >> 12, last, report);
    CHECK(reports == 3);
    manager.Gather(gather);
    const u64 generation = manager.Generation();
    manager.Gather(gather);
    CHECK(manager.Generation() == generation);

    // A write refused by the GPU is not collected.
    gathered.clear();
    manager.Gather(gather);
    gathered.clear();
    manager.CollectWrite(0x18000, 0x40, 0x18, last, [] { return false; });
    manager.Gather(gather);
    CHECK(gathered.empty());
}

TEST_CASE("GPUDirtyMemoryManager: a gather during a report reports the write again",
          "[core][gpu_dirty_memory]") {
    // The GPU thread gathers while a core reports a write: that gather's round releases the page,
    // and the write lands in the next list, so the page must be held again for it.
    Core::GPUDirtyMemoryManager manager;
    Core::GPUDirtyMemoryManager::ReportedPage last;
    Written gathered;
    std::function<void(PAddr, size_t)> gather = [&](PAddr addr, size_t size) {
        gathered.emplace_back(addr, size);
    };
    int reports = 0;
    constexpr PAddr addr = 0x20080;
    manager.CollectWrite(addr, 0x40, addr >> 12, last, [&] {
        if (reports++ == 0) {
            manager.Collect(0x30000, 0x40); // an earlier write for that gather to take
            manager.Gather(gather);
        }
        return true;
    });
    CHECK(reports == 2);
    CHECK(gathered == Written{{0x30000, 0x40}});
    gathered.clear();
    manager.Gather(gather);
    CHECK(gathered == Written{{addr, 0x40}});
}

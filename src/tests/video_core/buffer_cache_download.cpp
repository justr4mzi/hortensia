// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include "common/scope_exit.h"
#include "core/core.h"
#include "core/device_memory.h"
#include "core/hle/kernel/k_page_table.h"
#include "core/hle/kernel/k_process.h"
#include "core/memory.h"
#include "video_core/buffer_cache/buffer_cache.h"
#include "video_core/buffer_cache/memory_tracker_base.h"
#include "video_core/host1x/gpu_device_memory_manager.h"

namespace {
struct FixtureRuntime;
struct FixtureBuffer : VideoCommon::BufferBase {
    std::vector<u8> bytes;
    FixtureBuffer(FixtureRuntime&, VideoCommon::NullBufferParams tag) : BufferBase(tag) {}
    FixtureBuffer(FixtureRuntime&, DAddr addr, u32 size, bool)
        : BufferBase(addr, size), bytes(size) {}
    explicit FixtureBuffer(size_t size)
        : BufferBase(VideoCommon::NullBufferParams{}), bytes(size) {}
    // (offset, size) of every MarkUsage call: what the cache told the runtime its GPU work uses.
    std::vector<std::pair<u64, u64>> marked;
    void MarkUsage(u64 offset, u64 size) {
        marked.emplace_back(offset, size);
    }
};
struct FixtureStage {
    // Shared: the cache keeps a copy of a fence download's stage until its fence pops.
    std::shared_ptr<FixtureBuffer> buffer;
    size_t offset{};
    std::span<u8> mapped_span;
    explicit FixtureStage(size_t size)
        : buffer(std::make_shared<FixtureBuffer>(size)), mapped_span(buffer->bytes) {}
};
struct FixtureRuntime {
    bool out_of_bounds{};
    std::vector<std::pair<u64, u64>> copied;
    FixtureStage DownloadStagingBuffer(size_t size, bool = false) {
        return FixtureStage(size);
    }
    FixtureStage UploadStagingBuffer(size_t size) {
        return FixtureStage(size);
    }
    bool CanReportMemoryUsage() {
        return false;
    }
    u64 GetDeviceLocalMemory() {
        return 0;
    }
    void Finish() {}
    u64 CurrentTick() {
        return 0;
    }
    bool IsFree(u64) {
        return true;
    }
    void Wait(u64) {}
    template <class... T>
    bool CanReorderUpload(T&&...) {
        return false;
    }
    void ClearBuffer(FixtureBuffer& b, u64 off, u64 size, u32) {
        std::fill_n(b.bytes.begin() + off, size, 0);
    }
    void PreCopyBarrier() {}
    void PostCopyBarrier() {}
    template <class C>
    void CopyBuffer(std::shared_ptr<FixtureBuffer>& dst, FixtureBuffer& src, const C& copies,
                    bool sync, bool reorder = false) {
        CopyBuffer(*dst, src, copies, sync, reorder);
    }
    template <class C>
    void CopyBuffer(FixtureBuffer& dst, std::shared_ptr<FixtureBuffer>& src, const C& copies,
                    bool sync, bool reorder = false) {
        CopyBuffer(dst, *src, copies, sync, reorder);
    }
    template <class C>
    void CopyBuffer(FixtureBuffer& dst, FixtureBuffer& src, const C& copies, bool, bool = false) {
        for (const auto& c : copies) {
            copied.emplace_back(src.CpuAddr(), c.src_offset);
            if (c.src_offset > src.bytes.size() || c.size > src.bytes.size() - c.src_offset ||
                c.dst_offset > dst.bytes.size() || c.size > dst.bytes.size() - c.dst_offset) {
                out_of_bounds = true;
                continue;
            }
            std::memcpy(dst.bytes.data() + c.dst_offset, src.bytes.data() + c.src_offset, c.size);
        }
    }
};
struct FixtureParams {
    using Buffer = FixtureBuffer;
    using Runtime = FixtureRuntime;
    using Async_Buffer = FixtureStage;
    using MemoryTracker = VideoCommon::MemoryTrackerBase<Tegra::MaxwellDeviceMemoryManager>;
    static constexpr bool IS_OPENGL = false, HAS_PERSISTENT_UNIFORM_BUFFER_BINDINGS = false,
                          HAS_FULL_INDEX_AND_PRIMITIVE_SUPPORT = false,
                          NEEDS_BIND_UNIFORM_INDEX = false, NEEDS_BIND_STORAGE_INDEX = false,
                          USE_MEMORY_MAPS = true, SEPARATE_IMAGE_BUFFER_BINDINGS = false,
                          USE_MEMORY_MAPS_FOR_UPLOADS = true;
};
} // namespace
TEST_CASE("BufferCache: readback spanning separate buffers", "[video_core][buffer_cache]") {
    const bool old_multi_core = Settings::values.use_multi_core.GetValue();
    SCOPE_EXIT {
        Settings::values.use_multi_core.SetValue(old_multi_core);
    };
    Settings::values.use_multi_core.SetValue(false);
    Core::System system;
    system.Initialize();
    Kernel::KProcess process(system.Kernel());
    SCOPE_EXIT {
        system.Kernel().UnregisterKernelObject(&process);
    };
    auto& cpu = process.GetMemory();
    auto& table = process.GetPageTable().GetImpl();
    table.Resize(32, 12);
    cpu.SetCurrentPageTable(process);
    constexpr u64 phys = 0x80000, guest = 0x200000, dev = 0x70000, gap = 0x10000;
    system.DeviceMemory().buffer.ClearBackingRegion(phys, 3 * gap, 0);
    cpu.MapMemoryRegion(table, Common::ProcessAddress{guest}, 3 * gap,
                        Common::PhysicalAddress{Core::DramMemoryMap::Base + phys},
                        Common::MemoryPermission::ReadWrite, false);
    Tegra::MaxwellDeviceMemoryManager manager(system.DeviceMemory());
    auto asid = manager.RegisterProcess(&cpu);
    manager.Map(dev, guest, 3 * gap, asid);
    FixtureRuntime runtime;
    auto cache = std::make_unique<VideoCommon::BufferCache<FixtureParams>>(manager, runtime);
    auto* first =
        cache
            ->ObtainCPUBuffer(dev, gap, VideoCommon::ObtainBufferSynchronize::FullSynchronize,
                              VideoCommon::ObtainBufferOperation::MarkAsWritten)
            .first;
    std::fill(first->bytes.begin(), first->bytes.end(), 0x31);
    auto* second = cache
                       ->ObtainCPUBuffer(dev + 2 * gap, gap,
                                         VideoCommon::ObtainBufferSynchronize::FullSynchronize,
                                         VideoCommon::ObtainBufferOperation::MarkAsWritten)
                       .first;
    std::fill(second->bytes.begin(), second->bytes.end(), 0x72);
    runtime.copied.clear();
    REQUIRE(cache->IsRegionGpuModified(dev, gap));
    REQUIRE(cache->IsRegionGpuModified(dev + 2 * gap, gap));
    cache->DownloadMemory(dev + gap, gap);
    REQUIRE(runtime.copied.empty());
    REQUIRE(cache->IsRegionGpuModified(dev, gap));
    REQUIRE(cache->IsRegionGpuModified(dev + 2 * gap, gap));
    cache->DownloadMemory(dev, 3 * gap);
    CHECK_FALSE(runtime.out_of_bounds);
    const std::vector<std::pair<u64, u64>> expected{{dev, 0}, {dev + 2 * gap, 0}};
    CHECK(runtime.copied == expected);
    std::vector<u8> out1(gap), out2(gap);
    manager.ReadBlockUnsafe(dev, out1.data(), out1.size());
    manager.ReadBlockUnsafe(dev + 2 * gap, out2.data(), out2.size());
    CHECK(std::ranges::all_of(out1, [](u8 x) { return x == 0x31; }));
    CHECK(std::ranges::all_of(out2, [](u8 x) { return x == 0x72; }));
    CHECK_FALSE(cache->IsRegionGpuModified(dev, gap));
    CHECK_FALSE(cache->IsRegionGpuModified(dev + 2 * gap, gap));
}

TEST_CASE("BufferCache: downloads keep held-back CPU writes", "[video_core][buffer_cache]") {
    // Three buffers the GPU wrote (host bytes 0x31), three pages each, one per case below. In
    // each, the guest CPU writes code into the middle page and the core holds the write back
    // (OnCPUWrite) until the GPU thread applies it (a round of CachedWriteMemory).
    const bool old_multi_core = Settings::values.use_multi_core.GetValue();
    SCOPE_EXIT {
        Settings::values.use_multi_core.SetValue(old_multi_core);
    };
    Settings::values.use_multi_core.SetValue(false);
    Core::System system;
    system.Initialize();
    Kernel::KProcess process(system.Kernel());
    SCOPE_EXIT {
        system.Kernel().UnregisterKernelObject(&process);
    };
    auto& cpu = process.GetMemory();
    auto& table = process.GetPageTable().GetImpl();
    table.Resize(32, 12);
    cpu.SetCurrentPageTable(process);
    constexpr u64 phys = 0x80000, guest = 0x200000, dev = 0x70000, size = 0x3000;
    system.DeviceMemory().buffer.ClearBackingRegion(phys, 3 * size, 0);
    cpu.MapMemoryRegion(table, Common::ProcessAddress{guest}, 3 * size,
                        Common::PhysicalAddress{Core::DramMemoryMap::Base + phys},
                        Common::MemoryPermission::ReadWrite, false);
    Tegra::MaxwellDeviceMemoryManager manager(system.DeviceMemory());
    manager.Map(dev, guest, 3 * size, manager.RegisterProcess(&cpu));
    FixtureRuntime runtime;
    auto cache = std::make_unique<VideoCommon::BufferCache<FixtureParams>>(manager, runtime);
    const auto gpu_write = [&](u64 base, u8 value) {
        auto* buffer = cache
                           ->ObtainCPUBuffer(base, static_cast<u32>(size),
                                             VideoCommon::ObtainBufferSynchronize::FullSynchronize,
                                             VideoCommon::ObtainBufferOperation::MarkAsWritten)
                           .first;
        std::fill(buffer->bytes.begin(), buffer->bytes.end(), value);
    };
    const auto cpu_write = [&](u64 base) {
        const std::vector<u8> code(0x100, 0xab);
        manager.WriteBlockUnsafe(base + 0x1080, code.data(), code.size());
        REQUIRE(cache->OnCPUWrite(base + 0x1080, code.size()));
    };
    const auto guest_byte = [&](u64 addr) {
        u8 value{};
        manager.ReadBlockUnsafe(addr, &value, 1);
        return value;
    };
    cache->NoteCpuRead(dev, 3 * size); // GPU writes there are copied back at fences

    // A readback of the whole buffer leaves the CPU's newer bytes alone.
    const u64 sync = dev;
    gpu_write(sync, 0x31);
    cpu_write(sync);
    cache->DownloadMemory(sync, size);
    CHECK(guest_byte(sync) == 0x31);
    CHECK(guest_byte(sync + 0x1080) == 0xab);
    CHECK(guest_byte(sync + 0x2000) == 0x31);
    // The round that applies the write releases the page: what the GPU writes there later is
    // read back.
    const auto apply_round = [&](auto&& during) {
        cache->BeginCachedWrites();
        during();
        cache->CachedWriteMemory(sync + 0x1080, 0x100);
        cache->EndCachedWrites();
    };
    apply_round([] {});
    gpu_write(sync, 0x5c);
    cache->DownloadMemory(sync, size);
    CHECK(guest_byte(sync + 0x1080) == 0x5c);
    // Unless the core held the page again during the round: that write waits for the next one.
    gpu_write(sync, 0x31);
    apply_round([&] { cpu_write(sync); });
    gpu_write(sync, 0x5c);
    cache->DownloadMemory(sync, size);
    CHECK(guest_byte(sync) == 0x5c);
    CHECK(guest_byte(sync + 0x1080) == 0xab);
    apply_round([] {});
    gpu_write(sync, 0x5c);
    cache->DownloadMemory(sync, size);
    CHECK(guest_byte(sync + 0x1080) == 0x5c);
    cache->CommitAsyncFlushes();
    cache->PopAsyncFlushes();

    // A fence download taken before the CPU write does not write over it when it pops.
    const u64 before = dev + size;
    gpu_write(before, 0x31);
    REQUIRE(cache->HasUncommittedFlushes());
    cache->CommitAsyncFlushes();
    cpu_write(before);
    cache->PopAsyncFlushes();
    CHECK(guest_byte(before) == 0x31);
    CHECK(guest_byte(before + 0x1080) == 0xab);

    // Nor does one committed while the write is still held back.
    const u64 held = dev + 2 * size;
    gpu_write(held, 0x31);
    cpu_write(held);
    cache->CommitAsyncFlushes();
    cache->PopAsyncFlushes();
    CHECK(guest_byte(held + 0x2000) == 0x31);
    CHECK(guest_byte(held + 0x1080) == 0xab);
    cache.reset();
}

TEST_CASE("BufferCache: obtained buffers count as used by this command buffer",
          "[video_core][buffer_cache]") {
    // DMA buffer/image copies, indirect dispatches and query writes get their buffer through
    // ObtainCPUBuffer. Unless the bytes are marked used, CanReorderUpload lets a later upload to
    // them in the same command buffer run in front of that GPU work (Yoshi and the Mysterious
    // Book: a texture copied from a buffer took the next upload's bytes for one frame).
    const bool old_multi_core = Settings::values.use_multi_core.GetValue();
    SCOPE_EXIT {
        Settings::values.use_multi_core.SetValue(old_multi_core);
    };
    Settings::values.use_multi_core.SetValue(false);
    Core::System system;
    system.Initialize();
    Kernel::KProcess process(system.Kernel());
    SCOPE_EXIT {
        system.Kernel().UnregisterKernelObject(&process);
    };
    auto& cpu = process.GetMemory();
    auto& table = process.GetPageTable().GetImpl();
    table.Resize(32, 12);
    cpu.SetCurrentPageTable(process);
    constexpr u64 phys = 0x80000, guest = 0x200000, dev = 0x70000, size = 0x3000;
    system.DeviceMemory().buffer.ClearBackingRegion(phys, size, 0);
    cpu.MapMemoryRegion(table, Common::ProcessAddress{guest}, size,
                        Common::PhysicalAddress{Core::DramMemoryMap::Base + phys},
                        Common::MemoryPermission::ReadWrite, false);
    Tegra::MaxwellDeviceMemoryManager manager(system.DeviceMemory());
    manager.Map(dev, guest, size, manager.RegisterProcess(&cpu));
    FixtureRuntime runtime;
    auto cache = std::make_unique<VideoCommon::BufferCache<FixtureParams>>(manager, runtime);

    constexpr u64 addr = dev + 0x1100;
    constexpr u32 bytes = 0x340;
    const auto check = [&](VideoCommon::ObtainBufferSynchronize sync,
                           VideoCommon::ObtainBufferOperation op) {
        const auto [buffer, offset] = cache->ObtainCPUBuffer(addr, bytes, sync, op);
        REQUIRE(buffer->CpuAddr() + offset == addr);
        REQUIRE_FALSE(buffer->marked.empty());
        CHECK(buffer->marked.back() == std::pair<u64, u64>{offset, bytes});
    };
    // An upload source (the copy reads it) and a download target (the copy writes it).
    check(VideoCommon::ObtainBufferSynchronize::FullSynchronize,
          VideoCommon::ObtainBufferOperation::DoNothing);
    check(VideoCommon::ObtainBufferSynchronize::FullSynchronize,
          VideoCommon::ObtainBufferOperation::MarkAsWritten);
    check(VideoCommon::ObtainBufferSynchronize::NoSynchronize,
          VideoCommon::ObtainBufferOperation::DiscardWrite);

    // Uploads mark their bytes too. One that stays in this command buffer is otherwise overtaken
    // by a later upload to the same bytes, and its older data wins.
    const auto covered = [](const FixtureBuffer& buffer, u64 offset, u64 length) {
        return std::ranges::any_of(buffer.marked, [&](const std::pair<u64, u64>& mark) {
            return mark.first <= offset && offset + length <= mark.first + mark.second;
        });
    };
    constexpr u64 inline_addr = dev + 0x2040;
    const auto [written, written_offset] =
        cache->ObtainCPUBuffer(inline_addr, 0x80, VideoCommon::ObtainBufferSynchronize::FullSynchronize,
                               VideoCommon::ObtainBufferOperation::MarkAsWritten);
    written->marked.clear();
    const std::array<u8, 0x20> words{};
    REQUIRE(cache->InlineMemory(inline_addr + 0x10, words.size(), words));
    CHECK(covered(*written, written_offset + 0x10, words.size()));

    constexpr u64 cpu_addr = dev + 0x400;
    cache->WriteMemory(cpu_addr, 0x40);
    const auto [synced, synced_offset] =
        cache->ObtainCPUBuffer(cpu_addr, 0x40, VideoCommon::ObtainBufferSynchronize::FullSynchronize,
                               VideoCommon::ObtainBufferOperation::DoNothing);
    REQUIRE(synced->marked.size() >= 2);
    synced->marked.pop_back(); // ObtainCPUBuffer's own mark
    CHECK(covered(*synced, synced_offset, 0x40));
    cache.reset();
}

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/hle/service/filesystem/fsp/fs_read_trace.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "common/logging.h"

namespace Service::FileSystem {
namespace {

using Clock = std::chrono::steady_clock;

struct ReadStats {
    const char* label;
    std::mutex mutex{};
    Clock::time_point window_start{};
    std::vector<Clock::duration> latencies{};
    u64 window_bytes{};
    u64 total_reads{};
    u64 total_bytes{};
    Clock::duration total_time{};
};

std::array<ReadStats, 2> stats{{{"storage"}, {"file"}}};

double Micros(Clock::duration duration) {
    return std::chrono::duration<double, std::micro>(duration).count();
}

double MiB(u64 bytes) {
    return static_cast<double>(bytes) / (1024.0 * 1024.0);
}

/// Available memory from /proc/meminfo in bytes, or ~0 when unreadable.
u64 AvailableMemory() {
    std::ifstream meminfo{"/proc/meminfo"};
    std::string key;
    u64 kib = 0;
    std::string unit;
    while (meminfo >> key >> kib >> unit) {
        if (key == "MemAvailable:") {
            return kib << 10;
        }
    }
    return ~u64{0};
}

/// Asks the kernel to reclaim `mib` from this process's cgroup (memory.reclaim): directly when
/// the cgroup lets this process write it (a native run in its own scope), otherwise through the
/// launcher's helper (inside a container the cgroup file belongs to the container's root), which
/// watches $XDG_RUNTIME_DIR/lichii-bc250-reclaim and writes the container scope's file. Blocks
/// for as long as the kernel takes (seconds), so it runs on a thread of its own.
void RequestReclaim(u64 mib) {
    std::thread{[mib] {
        const std::string amount = std::to_string(mib) + "M";
        std::string cgroup;
        if (std::ifstream self{"/proc/self/cgroup"}; self) {
            std::string line;
            while (std::getline(self, line)) {
                if (line.rfind("0::", 0) == 0) {
                    cgroup = line.substr(3);
                }
            }
        }
        if (!cgroup.empty() && cgroup != "/") {
            const std::string path = "/sys/fs/cgroup" + cgroup + "/memory.reclaim";
            if (std::FILE* file = std::fopen(path.c_str(), "w")) {
                const auto start = Clock::now();
                std::fputs(amount.c_str(), file);
                std::fclose(file);
                LOG_INFO(Service_FS, "Area load: reclaimed up to {} from {} in {:.1f} s", amount,
                         cgroup, std::chrono::duration<double>(Clock::now() - start).count());
                return;
            }
        }
        const char* runtime_dir = std::getenv("XDG_RUNTIME_DIR");
        if (runtime_dir == nullptr) {
            LOG_INFO(Service_FS, "Area load: no cgroup to reclaim from and no XDG_RUNTIME_DIR");
            return;
        }
        const std::string request = std::string{runtime_dir} + "/lichii-bc250-reclaim";
        if (std::FILE* file = std::fopen(request.c_str(), "w")) {
            std::fputs(amount.c_str(), file);
            std::fclose(file);
            LOG_INFO(Service_FS, "Area load: asked the launcher to reclaim {}", amount);
        } else {
            LOG_INFO(Service_FS, "Area load: could not write {}: {}", request, std::strerror(errno));
        }
    }}.detach();
}

} // namespace

void NoteLoadRead(u64 bytes) {
    // An area load reads the next area's files at 20-50 MiB/s for a few seconds, 2-5 s before
    // its textures and render targets arrive; play reads under 2 MiB/s. With RAM full those
    // ~250 GPU allocations waited for the kernel to swap out what they needed (Castle Town entry:
    // 2.2-3.8 s frozen). Asking the kernel to reclaim cold memory as the reads start halved that
    // (0.9-1.3 s). Reclaiming a fixed 1.5 GB at every burst was too much in play (8 times in
    // 10 minutes, often with 0.6-1.4 GB already free): memory still in use went to swap and came
    // back at up to 15,000 pages/s, with short audio stutters in Kakariko. So it reclaims only up
    // to EDEN_NX2_PRE_RECLAIM_TARGET_MIB available (default 2048), at most
    // EDEN_NX2_PRE_RECLAIM_MIB at once (default 1536, 0: off), nothing under 256 MiB, at most
    // once a minute, for bursts of 16 MiB or more within a second.
    static const u64 reclaim_mib = [] {
        const char* value = std::getenv("EDEN_NX2_PRE_RECLAIM_MIB");
        return value != nullptr ? std::strtoull(value, nullptr, 10) : u64{1536};
    }();
    static const u64 target_mib = [] {
        const char* value = std::getenv("EDEN_NX2_PRE_RECLAIM_TARGET_MIB");
        return value != nullptr ? std::strtoull(value, nullptr, 10) : u64{2048};
    }();
#if !defined(__linux__)
    static_cast<void>(bytes);
    return;
#else
    if (reclaim_mib == 0) {
        return;
    }
    static std::mutex mutex;
    static Clock::time_point window_start{};
    static u64 window_bytes = 0;
    static Clock::time_point last_request{};
    std::scoped_lock lock{mutex};
    const auto now = Clock::now();
    if (now - window_start > std::chrono::seconds{1}) {
        window_start = now;
        window_bytes = 0;
    }
    window_bytes += bytes;
    if (window_bytes < (u64{16} << 20)) {
        return;
    }
    window_bytes = 0;
    if (last_request != Clock::time_point{} && now - last_request < std::chrono::seconds{60}) {
        return;
    }
    const u64 available_mib = AvailableMemory() >> 20;
    const u64 wanted = target_mib > available_mib ? target_mib - available_mib : 0;
    const u64 amount = (std::min)(wanted, reclaim_mib);
    if (amount < 256) {
        return;
    }
    last_request = now;
    LOG_INFO(Service_FS, "Area load: 16+ MiB of guest reads in under 1 s with {} MiB available",
             available_mib);
    RequestReclaim(amount);
#endif
}

void RecordRead(ReadTraceSource source, u64 bytes, Clock::duration elapsed) {
    auto& s = stats[static_cast<std::size_t>(source)];
    std::scoped_lock lock{s.mutex};
    const auto now = Clock::now();
    if (s.latencies.empty()) {
        s.window_start = now - elapsed;
    }
    s.latencies.push_back(elapsed);
    s.window_bytes += bytes;
    ++s.total_reads;
    s.total_bytes += bytes;
    s.total_time += elapsed;
    if (now - s.window_start < std::chrono::seconds{1}) {
        return;
    }

    auto& l = s.latencies;
    std::sort(l.begin(), l.end());
    const auto at = [&](double q) { return l[static_cast<std::size_t>(q * (l.size() - 1))]; };
    const auto over = [&](auto limit) {
        return l.end() - std::upper_bound(l.begin(), l.end(), Clock::duration{limit});
    };
    LOG_INFO(Service_FS,
             "Guest {} reads: {} in {:.2f} s, {:.2f} MiB, p50 {:.1f} us, p99 {:.1f} us, max {:.1f} "
             "us, {} over 1 ms, {} over 16 ms; since boot {} reads, {:.1f} MiB, {:.3f} s reading",
             s.label, l.size(), std::chrono::duration<double>(now - s.window_start).count(),
             MiB(s.window_bytes), Micros(at(0.50)), Micros(at(0.99)), Micros(l.back()),
             over(std::chrono::milliseconds{1}), over(std::chrono::milliseconds{16}), s.total_reads,
             MiB(s.total_bytes), std::chrono::duration<double>(s.total_time).count());
    l.clear();
    s.window_bytes = 0;
}

} // namespace Service::FileSystem

// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <optional>
#include <source_location>
#include "common/common_types.h"

namespace Common::UnmappedReadDiagnostic {

inline bool Enabled() noexcept {
    static const bool enabled = [] {
        const char* value = std::getenv("EDEN_NX2_UNMAPPED_READ_TRACE");
        return value && value[0] == '1' && value[1] == '\0';
    }();
    return enabled;
}

struct Context {
    bool has_gpu{};
    u64 gpu_address{};
    size_t gpu_size{};
    std::source_location gpu_caller{};
    bool gpu_translated{};
    u64 gpu_chunk_address{};
    size_t gpu_chunk_size{};
    bool has_sync{};
    u64 sync_address{};
    size_t sync_size{};
    std::source_location sync_caller{};
};
inline thread_local Context current{};

class Scope {
public:
    explicit Scope(bool enabled = Enabled()) noexcept : active{enabled} {
        if (active) {
            previous.emplace(current);
        }
    }
    ~Scope() {
        if (active) {
            current = *previous;
        }
    }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
    void Gpu(u64 address, size_t size, std::source_location caller) noexcept {
        if (active) {
            current.has_gpu = true;
            current.gpu_address = address;
            current.gpu_size = size;
            current.gpu_caller = caller;
            current.gpu_translated = false;
        }
    }
    void TranslatedGpu(u64 address, size_t size) noexcept {
        if (active) {
            current.gpu_translated = true;
            current.gpu_chunk_address = address;
            current.gpu_chunk_size = size;
        }
    }
    void Sync(u64 address, size_t size, std::source_location caller) noexcept {
        if (active) {
            current.has_sync = true;
            current.sync_address = address;
            current.sync_size = size;
            current.sync_caller = caller;
            current.gpu_translated = false;
        }
    }

private:
    bool active{};
    std::optional<Context> previous;
};

class Limiter {
public:
    bool Claim(size_t size) noexcept {
        const u32 bit = size == 69632 ? 1U : size == 20480 ? 2U : 0U;
        return bit && !(seen.fetch_or(bit, std::memory_order_relaxed) & bit);
    }

private:
    std::atomic<u32> seen{};
};
inline Limiter limiter;
} // namespace Common::UnmappedReadDiagnostic

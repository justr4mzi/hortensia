// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <atomic>
#include <stdexcept>
#include <thread>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include "common/unmapped_read_diagnostic.h"

namespace diag = Common::UnmappedReadDiagnostic;

TEST_CASE("Unmapped read diagnostic limits each target independently",
          "[video_core][unmapped_trace]") {
    diag::Limiter limit;
    REQUIRE_FALSE(limit.Claim(1));
    REQUIRE(limit.Claim(69632));
    REQUIRE_FALSE(limit.Claim(69632));
    REQUIRE(limit.Claim(20480));
    REQUIRE_FALSE(limit.Claim(20480));
}

TEST_CASE("Unmapped read diagnostic claims are thread safe", "[video_core][unmapped_trace]") {
    diag::Limiter limit;
    std::atomic<unsigned> first{}, second{};
    std::vector<std::thread> threads;
    for (int i = 0; i < 32; ++i) {
        threads.emplace_back([&] {
            for (int j = 0; j < 128; ++j) {
                first += limit.Claim(69632);
                second += limit.Claim(20480);
            }
        });
    }
    for (auto& thread : threads)
        thread.join();
    REQUIRE(first == 1);
    REQUIRE(second == 1);
}

TEST_CASE("Unmapped read scopes restore nested contexts and stay thread local",
          "[video_core][unmapped_trace]") {
    REQUIRE_FALSE(diag::current.has_gpu);
    {
        diag::Scope outer(true);
        outer.Gpu(0x1000, 90000, std::source_location::current());
        try {
            diag::Scope inner(true);
            inner.Gpu(0x2000, 69632, std::source_location::current());
            inner.TranslatedGpu(0x2800, 20480);
            REQUIRE(diag::current.gpu_translated);
            REQUIRE(diag::current.gpu_chunk_address == 0x2800);
            REQUIRE(diag::current.gpu_chunk_size == 20480);
            {
                diag::Scope sync(true);
                sync.Sync(0x3000, 20480, std::source_location::current());
                REQUIRE(diag::current.gpu_address == 0x2000);
                REQUIRE(diag::current.has_sync);
                REQUIRE_FALSE(diag::current.gpu_translated);
            }
            REQUIRE(diag::current.gpu_translated);
            REQUIRE_FALSE(diag::current.has_sync);
            std::atomic<bool> isolated{};
            std::thread other(
                [&] { isolated = !diag::current.has_gpu && !diag::current.has_sync; });
            other.join();
            REQUIRE(isolated);
            throw std::runtime_error("scope test");
        } catch (const std::runtime_error&) {
        }
        REQUIRE(diag::current.gpu_address == 0x1000);
        REQUIRE(diag::current.gpu_size == 90000);
        REQUIRE_FALSE(diag::current.has_sync);
        diag::Scope disabled(false);
        disabled.Gpu(0x4000, 1, std::source_location::current());
        REQUIRE(diag::current.gpu_address == 0x1000);
    }
    REQUIRE_FALSE(diag::current.has_gpu);
    REQUIRE_FALSE(diag::current.has_sync);
}

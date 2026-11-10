// SPDX-License-Identifier: GPL-3.0-or-later

#include <limits>

#include <catch2/catch_test_macros.hpp>

#include "common/literals.h"
#include "video_core/texture_cache/memory_budget.h"
#include "video_core/vulkan_common/memory_budget.h"

using namespace Common::Literals;

TEST_CASE("Host commit pressure limits reclaimable cache budgets", "[memory_budget]") {
    using Vulkan::LimitBudgetForHostCommit;
    REQUIRE(LimitBudgetForHostCommit(10_GiB, 8_GiB, 20_GiB, 4_GiB) == 10_GiB);
    REQUIRE(LimitBudgetForHostCommit(10_GiB, 8_GiB, 5_GiB, 4_GiB) == 9_GiB);
    REQUIRE(LimitBudgetForHostCommit(10_GiB, 8_GiB, 4_GiB, 4_GiB) == 8_GiB);
    REQUIRE(LimitBudgetForHostCommit(10_GiB, 8_GiB, 3_GiB, 4_GiB) == 7_GiB);
    REQUIRE(LimitBudgetForHostCommit(10_GiB, 8_GiB, 0, 4_GiB) == 4_GiB);
    REQUIRE(LimitBudgetForHostCommit(10_GiB, 1_GiB, 0, 4_GiB) == 0);
    REQUIRE(LimitBudgetForHostCommit(10_GiB, 8_GiB, 5_GiB, 4_GiB) ==
            LimitBudgetForHostCommit(10_GiB, 7_GiB, 6_GiB, 4_GiB));
    REQUIRE(LimitBudgetForHostCommit(6_GiB, 8_GiB, 20_GiB, 4_GiB) == 6_GiB);
    REQUIRE(LimitBudgetForHostCommit(6_GiB, 8_GiB, 3_GiB, 4_GiB) == 6_GiB);
    REQUIRE(LimitBudgetForHostCommit(10_GiB, 8_GiB, std::numeric_limits<u64>::max(), 4_GiB) ==
            10_GiB);
}

TEST_CASE("Budget-aware texture cache retains fitting Overworld working sets", "[memory_budget]") {
    using VideoCommon::ClampTextureCacheThresholds;
    // An 8-GiB working set fits a 10/11-GiB driver-adjusted policy; the legacy
    // ceiling would collect it repeatedly despite several GiB of safe headroom.
    const auto roomy = ClampTextureCacheThresholds({10_GiB, 11_GiB, 2_GiB}, true, 5_GiB);
    CHECK(8_GiB < roomy.expected);
    CHECK(roomy.critical == 11_GiB);
    CHECK(roomy.expected < roomy.critical);
    const auto fallback = ClampTextureCacheThresholds({10_GiB, 11_GiB, 2_GiB}, false, 5_GiB);
    CHECK(fallback.critical == 5_GiB);
    CHECK(fallback.expected < 5_GiB);
    // Budget shrink and low-memory devices must keep their lower thresholds.
    const auto smaller = ClampTextureCacheThresholds({2_GiB, 3_GiB, 1536_MiB}, true, 5_GiB);
    CHECK(smaller.expected == 2_GiB);
    CHECK(smaller.critical == 3_GiB);
    CHECK(smaller.minimum == 1_GiB);
    const auto tiny = ClampTextureCacheThresholds({256_MiB, 512_MiB, 128_MiB}, true, 5_GiB);
    CHECK(tiny.expected == 0);
    CHECK(tiny.minimum == 0);
}

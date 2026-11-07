// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "video_core/renderer_vulkan/async_draw_policy.h"

using Vulkan::DecideHeavyPending;
using Vulkan::DefaultHeavySkipWords;
using Vulkan::HeavyPendingAction;
using Vulkan::ParseHeavySkipWords;

TEST_CASE("Heavy skip is on unless EDEN_NX2_HEAVY_SKIP turns it off", "[video_core][heavy_skip]") {
    REQUIRE(ParseHeavySkipWords(nullptr) == DefaultHeavySkipWords);
    REQUIRE(ParseHeavySkipWords("") == DefaultHeavySkipWords);
    REQUIRE(ParseHeavySkipWords("1") == DefaultHeavySkipWords);
    REQUIRE(ParseHeavySkipWords("on") == DefaultHeavySkipWords);
    REQUIRE(ParseHeavySkipWords("0") == 0);
}

TEST_CASE("The words off, false and no turn the heavy skip off", "[video_core][heavy_skip]") {
    REQUIRE(ParseHeavySkipWords("off") == 0);
    REQUIRE(ParseHeavySkipWords("OFF") == 0);
    REQUIRE(ParseHeavySkipWords("false") == 0);
    REQUIRE(ParseHeavySkipWords("no") == 0);
    REQUIRE(ParseHeavySkipWords("nope") == DefaultHeavySkipWords);
}

TEST_CASE("A number in EDEN_NX2_HEAVY_SKIP sets the heavy threshold", "[video_core][heavy_skip]") {
    REQUIRE(ParseHeavySkipWords("6000") == 6000);
    REQUIRE(ParseHeavySkipWords("2") == 2);
}

TEST_CASE("Only native pipelines at or over the threshold are judged", "[video_core][heavy_skip]") {
    REQUIRE(DecideHeavyPending({.native = false, .source_words = 9000, .target_redrawn = true},
                               4000) == HeavyPendingAction::NotHeavy);
    REQUIRE(DecideHeavyPending({.native = true, .source_words = 9000, .target_redrawn = true}, 0) ==
            HeavyPendingAction::NotHeavy);
    REQUIRE(DecideHeavyPending({.native = true, .source_words = 3999, .target_redrawn = true},
                               4000) == HeavyPendingAction::NotHeavy);
    REQUIRE(DecideHeavyPending({.native = true, .source_words = 4000, .target_redrawn = true},
                               4000) == HeavyPendingAction::Skip);
}

TEST_CASE("A heavy draw that could lose data for good is held", "[video_core][heavy_skip]") {
    // Storage writes and transform feedback hold the draw even on a target redrawn every frame.
    REQUIRE(DecideHeavyPending(
                {.native = true, .source_words = 9000, .keeps_data = true, .target_redrawn = true},
                4000) == HeavyPendingAction::HoldStorage);
    REQUIRE(DecideHeavyPending({.native = true, .source_words = 9000, .target_not_cleared = true},
                               4000) == HeavyPendingAction::HoldNotCleared);
    REQUIRE(DecideHeavyPending({.native = true, .source_words = 9000}, 4000) ==
            HeavyPendingAction::HoldNewTarget);
}

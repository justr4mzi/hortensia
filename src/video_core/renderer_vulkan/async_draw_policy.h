// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <cstdlib>

#include "common/common_types.h"

namespace Vulkan {

/// Whether a draw whose graphics pipeline is still building may be skipped (asynchronous
/// shaders) instead of waiting for the build.
/// @param native         The draw comes from a Switch 2 (native Turing macro) program.
/// @param depth_enabled  The depth test is on for the draw.
/// @param index_count    Index count of the draw.
/// @param vertex_count   Vertex count of the draw.
/// @param discard_native Let native draws follow the heuristic too (EDEN_NX2_ASYNC_DISCARD=1).
/// @return True when the draw may be skipped.
constexpr bool CanDiscardPendingDraw(bool native, bool depth_enabled, u32 index_count,
                                    u32 vertex_count, bool discard_native = false) {
    // Native draws can produce persistent resources, even with depth enabled. This heuristic only
    // skips them when the user opts in (EDEN_NX2_ASYNC_DISCARD=1 or the native_async_draws
    // setting), trading a possible missing effect for no stall while a new pipeline builds.
    // Heavy pipelines are judged first by DecideHeavyPending, with its own guards.
    if (native && !discard_native) {
        return false;
    }
    // Preserve the legacy async-shader heuristic.
    return depth_enabled || (index_count > 6 && vertex_count > 6);
}

/// How a native draw with an unbuilt pipeline is handled (the native_async_draws setting).
enum class NativePendingMode { Wait, Bounded, Skip };

/// What to do with a native draw whose pipeline is still building.
enum class NativeDrawAction {
    Wait,      ///< Use the pipeline; the draw waits for the build.
    WaitTimed, ///< Wait for the build up to the remaining budget, then skip if still unbuilt.
    Skip,      ///< Drop the draw.
};

/// @param mode           Wait, Bounded or Skip (EDEN_NX2_ASYNC_DISCARD=1 passes Skip).
/// @param can_discard    The legacy heuristic allows skipping this draw.
/// @param budget_left_us Microseconds left of this frame's wait budget.
constexpr NativeDrawAction DecideNativePending(NativePendingMode mode, bool can_discard,
                                               s64 budget_left_us) {
    // A draw the heuristic keeps must be waited for whatever the mode is.
    if (mode == NativePendingMode::Wait || !can_discard) {
        return NativeDrawAction::Wait;
    }
    if (mode == NativePendingMode::Skip || budget_left_us <= 0) {
        return NativeDrawAction::Skip;
    }
    return NativeDrawAction::WaitTimed;
}

/// Converted instructions (all stages) from which a native pipeline counts as heavy. In a
/// demanding Switch 2 test title's first visits every driver build over 500 ms was above it.
inline constexpr size_t DefaultHeavySkipWords = 4000;

/// Reads EDEN_NX2_HEAVY_SKIP: "0", "off", "false" or "no" turn the heavy skip off and a number
/// above 1 is the threshold; unset, "1" or anything else keeps the default.
inline size_t ParseHeavySkipWords(const char* value) {
    if (value == nullptr) {
        return DefaultHeavySkipWords;
    }
    const auto is_word = [value](const char* word) {
        size_t i = 0;
        for (; word[i] != '\0'; ++i) {
            const char c = value[i];
            if ((c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c) != word[i]) {
                return false;
            }
        }
        return value[i] == '\0';
    };
    if (is_word("off") || is_word("false") || is_word("no")) {
        return 0;
    }
    if (*value < '0' || *value > '9') {
        return DefaultHeavySkipWords;
    }
    const size_t parsed = static_cast<size_t>(std::strtoull(value, nullptr, 10));
    return parsed == 1 ? DefaultHeavySkipWords : parsed;
}

/// A draw whose graphics pipeline is still building, as the heavy skip sees it.
struct HeavyPendingDraw {
    bool native{};             ///< From a Switch 2 (native Turing macro) program
    size_t source_words{};     ///< Converted instructions of the pipeline, all stages
    bool keeps_data{};         ///< The pipeline writes storage or uses transform feedback
    bool target_redrawn{};     ///< The render target is rebuilt every frame
    bool target_not_cleared{}; ///< The render target was not cleared this frame
};

/// What the heavy skip does with a draw whose pipeline is still building.
enum class HeavyPendingAction {
    NotHeavy,       ///< Not a heavy native pipeline: the pending-draw policy above decides.
    HoldStorage,    ///< Wait: skipping would lose its storage or transform feedback output.
    HoldNotCleared, ///< Wait: the target may be a cache the game only patches.
    HoldNewTarget,  ///< Wait: a new small target is likely a texture baked once.
    Skip,           ///< Drop the draw while the pipeline builds.
};

/// @param threshold_words Heavy threshold from ParseHeavySkipWords; 0 turns the skip off.
constexpr HeavyPendingAction DecideHeavyPending(const HeavyPendingDraw& draw,
                                                size_t threshold_words) {
    if (!draw.native || threshold_words == 0 || draw.source_words < threshold_words) {
        return HeavyPendingAction::NotHeavy;
    }
    if (draw.keeps_data) {
        return HeavyPendingAction::HoldStorage;
    }
    if (!draw.target_redrawn) {
        return draw.target_not_cleared ? HeavyPendingAction::HoldNotCleared
                                       : HeavyPendingAction::HoldNewTarget;
    }
    return HeavyPendingAction::Skip;
}

/// Whether a native pipeline's SPIR-V emission is deferred to a worker: only while fewer than
/// `capacity` emissions are pending.
constexpr bool DefersShaderEmission(bool native, size_t pending, size_t capacity) {
    return native && pending < capacity;
}

} // namespace Vulkan

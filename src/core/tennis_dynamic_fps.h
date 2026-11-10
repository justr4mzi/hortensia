// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <bit>
#include <cmath>
#include <map>
#include <mutex>
#include <unordered_map>
#include "common/common_types.h"

namespace Core {
// Sample once per actual VI present, not once per clock reader.
// The game's control/snapshot field stays in the fixed-target representation.
class TennisDynamicFps {
public:
    void Advance(u64 generation, u32 frame, s64 now, u32 target, u32 speed_percent) {
        std::scoped_lock lock{mutex};
        if (generation != current_generation || target != fps || speed_percent != speed) {
            initialized = false;
            clocks.clear();
            waits.clear();
        }
        if (initialized && frame == last_frame) {
            return;
        }
        const s64 elapsed = now - last_ns;
        const bool contiguous = initialized && frame == last_frame + 1 && elapsed > 0 &&
                                elapsed <= std::max<s64>(250'000'000, 4'000'000'000LL / target);
        seconds = contiguous && speed_percent != 0
                      ? static_cast<double>(elapsed) * speed_percent / 100e9
                      : 1.0 / target;
        if (!contiguous) {
            clocks.clear();
            waits.clear();
        }
        initialized = true;
        current_generation = generation;
        fps = target;
        speed = speed_percent;
        last_frame = frame;
        last_ns = now;
        ++sequence;
    }

    float Scale(float canonical, u32 target, u64 generation) {
        std::scoped_lock lock{mutex};
        if (!initialized || target != fps || generation != current_generation ||
            !std::isfinite(canonical) || canonical <= 0) {
            return canonical;
        }
        const u32 key = std::bit_cast<u32>(canonical);
        auto it = clocks.find(key);
        if (it == clocks.end()) {
            if (clocks.size() >= 512) {
                // Bound transient replay values without evicting active clock phases.
                return canonical;
            }
            it = clocks.emplace(key, Clock{}).first;
        }
        auto& clock = it->second;
        if (clock.sequence == sequence) {
            return clock.step;
        }
        if (clock.sequence + 1 != sequence) {
            clock.remainder = 0;
        }
        const double nominal = canonical / (60.0f / target);
        // Both native character and ball consumers truncate to 1/64. Carry the
        // fraction here so all readers agree and 90/144 FPS do not lose game time.
        const double units = std::clamp(nominal * 60 * seconds * 64, 0.0, 3840.0) + clock.remainder;
        const double whole = std::floor(units);
        clock.remainder = units - whole;
        clock.step = static_cast<float>(whole / 64);
        clock.sequence = sequence;
        return clock.step;
    }

    u32 Consume(u64 thread, u32 reg, u32 remaining, bool inclusive, u32 target, u64 generation,
                s64 now) {
        std::scoped_lock lock{mutex};
        if (remaining >= 0x80000000U)
            return remaining - 1;
        auto& wait = waits[{thread, reg}];
        if (wait.initialized && (wait.finished || remaining > wait.last_remaining)) {
            wait = {};
        }
        double elapsed = initialized && fps == target && current_generation == generation
                             ? seconds
                             : 1.0 / target;
        if (wait.initialized && speed != 0) {
            const s64 delta = now - wait.last_ns;
            if (delta >= 0 && delta <= std::max<s64>(250'000'000, 4'000'000'000LL / target)) {
                elapsed = static_cast<double>(delta) * speed / 100e9;
            } else {
                wait.remainder = 0;
            }
        }
        wait.initialized = true;
        wait.last_ns = now;
        const double exact = elapsed * target + wait.remainder;
        const u32 ticks = static_cast<u32>(std::floor(exact));
        wait.remainder = exact - ticks;
        const u32 result =
            inclusive && ticks > remaining ? 0xffffffffU : remaining - std::min(remaining, ticks);
        wait.last_remaining = result;
        wait.finished = result == (inclusive ? 0xffffffffU : 0);
        return result;
    }

private:
    struct Clock {
        u64 sequence{};
        double remainder{};
        float step{};
    };
    struct Wait {
        bool initialized{}, finished{};
        u32 last_remaining{};
        s64 last_ns{};
        double remainder{};
    };
    std::mutex mutex;
    std::unordered_map<u32, Clock> clocks;
    std::map<std::pair<u64, u32>, Wait> waits;
    bool initialized{};
    u64 current_generation{}, sequence{};
    u32 last_frame{}, fps{}, speed{};
    s64 last_ns{};
    double seconds{};
};
} // namespace Core

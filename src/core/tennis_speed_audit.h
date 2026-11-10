// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include "common/common_types.h"

namespace Core {
// Consumer audit: step is the delta consumed by the observed animation/physics
// update. One observation per call; this does not establish every other clock.
class TennisSpeedAudit {
public:
    bool Observe(u32 frame, s64 now_ns, float step, u32 target) {
        if (initialized && frame == last_frame) {
            covered = false;
            last_elapsed = 0;
            sample_speed = std::numeric_limits<double>::quiet_NaN();
            return false;
        }
        const s64 elapsed = now_ns - last_ns;
        last_elapsed = elapsed;
        covered = initialized && frame == last_frame + 1 && elapsed > 0 &&
                  elapsed <= std::max<s64>(250'000'000, 4'000'000'000LL / target) &&
                  std::isfinite(step) && step >= 0 && step <= 60;
        sample_speed = std::numeric_limits<double>::quiet_NaN();
        if (covered) {
            const double seconds = elapsed / 1e9;
            const double game_step = step / 60.0;
            wall_seconds += seconds;
            game_seconds += game_step;
            sample_speed = game_step / seconds;
            ++frames;
            if (step > 0) {
                active_wall_seconds += seconds;
                active_game_seconds += game_step;
            } else {
                paused_seconds += seconds;
            }
        } else if (initialized && elapsed > 0) {
            uncovered_seconds += elapsed / 1e9;
        }
        if (!initialized) {
            report_start_ns = now_ns;
        }
        initialized = true;
        last_frame = frame;
        last_ns = now_ns;
        return now_ns - report_start_ns >= 5'000'000'000LL;
    }

    void ClearWindow() {
        wall_seconds = game_seconds = active_wall_seconds = active_game_seconds = 0;
        paused_seconds = uncovered_seconds = 0;
        frames = 0;
        report_start_ns = last_ns;
    }

    double GameSpeed() const { return wall_seconds > 0 ? game_seconds / wall_seconds : 0; }
    double ActiveGameSpeed() const {
        return active_wall_seconds > 0 ? active_game_seconds / active_wall_seconds : 0;
    }

    double wall_seconds{}, game_seconds{}, active_wall_seconds{}, active_game_seconds{};
    double paused_seconds{}, uncovered_seconds{}, sample_speed{};
    u64 frames{};
    s64 last_elapsed{};
    bool covered{};

private:
    bool initialized{};
    u32 last_frame{};
    s64 last_ns{}, report_start_ns{};
};
} // namespace Core

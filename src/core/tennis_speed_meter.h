// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <mutex>
#include <optional>
#include "core/tennis_speed_audit.h"

namespace Core {
// Read back real consumer deltas; never infer speed from the requested FPS.
class TennisSpeedMeter {
public:
    void Observe(u64 generation, bool character, u64 object, s64 now, float step, u32 fps) {
        std::scoped_lock lock{mutex};
        if (generation != current_generation) {
            streams = {};
            current_generation = generation;
        }
        auto& stream = streams[character ? 1 : 0];
        if (!stream.initialized || (stream.object != object && now - stream.last_seen > 500'000'000)) {
            stream = {};
            stream.initialized = true;
            stream.object = object;
            stream.window_start = now;
        }
        if (stream.object != object) return;
        stream.last_seen = now;
        stream.audit.Observe(stream.sequence++, now, step, fps);
        if (!stream.audit.covered) {
            stream.ready = false;
            stream.audit.ClearWindow();
            stream.window_start = now;
        } else if (now - stream.window_start >= 500'000'000) {
            stream.value = stream.audit.GameSpeed();
            stream.ready = true;
            stream.last_sample = now;
            stream.audit.ClearWindow();
            stream.window_start = now;
        }
    }

    std::optional<double> Read(u64 generation, s64 now) {
        std::scoped_lock lock{mutex};
        if (generation != current_generation) return {};
        // In gameplay prefer character integration, including its real zero-step pause.
        for (const auto index : {1, 0}) {
            const auto& stream = streams[index];
            if (stream.initialized && now >= stream.last_seen &&
                now - stream.last_seen <= 750'000'000) {
                if (stream.ready && now - stream.last_sample <= 1'500'000'000) return stream.value;
                return {};
            }
        }
        return {};
    }

private:
    struct Stream {
        TennisSpeedAudit audit;
        u64 object{};
        u32 sequence{};
        s64 last_seen{}, last_sample{}, window_start{};
        double value{};
        bool initialized{}, ready{};
    };
    std::mutex mutex;
    std::array<Stream, 2> streams;
    u64 current_generation{};
};
inline TennisSpeedMeter g_tennis_speed_meter;
} // namespace Core

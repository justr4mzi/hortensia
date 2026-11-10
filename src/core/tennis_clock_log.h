// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <fstream>
#include <mutex>
#include <unordered_map>
#include "common/fs/path_util.h"
#include "common/logging.h"
#include "core/tennis_speed_audit.h"

namespace Core {
// Separate audits for concrete consumers. An arbitrary global scale is not a
// whole-game clock: UI and character integration may select different deltas.
class TennisClockLog {
public:
    enum class Clock : u32 { UiAnimation, CharacterIntegration, BallFlight };

    void Observe(u64 generation, Clock clock, u64 object, u32 frame, s64 now, float step,
                 float global_step, u32 fps, const std::array<float, 3>& position) {
        std::scoped_lock lock{mutex};
        if (generation != current_generation) {
            records = {};
            csv.close();
            csv.clear();
            const auto path = Common::FS::GetEdenPath(Common::FS::EdenPath::LogDir) /
                              fmt::format("tennis-consumer-speed-{}.csv", now);
            csv.open(path);
            csv.precision(9);
            csv << "clock,object,native_frame,host_ns,input_step,global_scale,clock_speed,covered,"
                   "target_fps,world_x,world_y,world_z\n";
            if (csv) {
                LOG_INFO(Kernel_SVC, "Tennis consumer-speed CSV: {}", path.string());
            } else {
                LOG_WARNING(Kernel_SVC, "Cannot open Tennis consumer-speed CSV: {}", path.string());
            }
            current_generation = generation;
        }
        auto& clocks = records[static_cast<size_t>(clock)];
        if (clocks.size() >= 64 && !clocks.contains(object)) {
            return;
        }
        auto& record = clocks[object];
        const bool report = record.audit.Observe(record.sequence++, now, step, fps);
        const char* name = clock == Clock::UiAnimation            ? "ui_animation"
                           : clock == Clock::CharacterIntegration ? "character_integration"
                                                                  : "ball_flight_requested";
        if (csv) {
            csv << name << ',' << object << ',' << frame << ',' << now << ',' << step << ','
                << global_step << ',' << record.audit.sample_speed << ',' << record.audit.covered
                << ',' << fps << ',' << position[0] << ',' << position[1] << ',' << position[2] << '\n';
        }
        if (report) {
            const auto& audit = record.audit;
            LOG_INFO(Kernel_SVC,
                     "Tennis clock audit: clock={} object={:#x} clock_speed={:.6f} "
                     "active_clock_speed={:.6f} calls_per_s={:.3f} input_step={} global_scale={} "
                     "paused_s={:.3f} uncovered_s={:.3f} target_fps={}",
                     name, object, audit.GameSpeed(), audit.ActiveGameSpeed(),
                     audit.wall_seconds > 0 ? audit.frames / audit.wall_seconds : 0, step,
                     global_step, audit.paused_seconds, audit.uncovered_seconds, fps);
            record.audit.ClearWindow();
            csv.flush();
        }
    }

private:
    struct Record {
        TennisSpeedAudit audit;
        u32 sequence{};
    };
    std::mutex mutex;
    std::ofstream csv;
    std::array<std::unordered_map<u64, Record>, 3> records;
    u64 current_generation{};
};
} // namespace Core

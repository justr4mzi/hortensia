// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include "common/settings_enums.h"

namespace Settings {

inline constexpr u64 WonderSwitch2ProgramId = 0x040015100B514001ULL;
inline constexpr u64 TennisFpsProgramId = 0x0400D4201C6EA000ULL;
inline std::atomic<u32> tennis_fps_active{0};
inline std::atomic<u64> tennis_fps_nro_base{0};
inline std::atomic<u64> tennis_fps_generation{0};

inline constexpr bool ShowTennisFps(u64 program_id, bool global) {
    return !global && program_id == TennisFpsProgramId;
}

inline constexpr u64 BananzaFpsProgramId = 0x040074A01BF12000ULL;

// Convert the preset index stored by older configs to the numeric FPS target.
inline constexpr u32 WonderFpsValue(WonderFps target) {
    switch (target) {
    case WonderFps::Fps30:
        return 30;
    case WonderFps::Fps60:
        return 60;
    case WonderFps::Fps90:
        return 90;
    case WonderFps::Fps120:
        return 120;
    case WonderFps::Fps180:
        return 180;
    case WonderFps::Fps240:
        return 240;
    }
    return 60;
}

// Set only after the matching main module was successfully patched. A GUI change during a
// session must not change presentation cadence until the game is restarted and patched again.
inline std::atomic<u32> wonder_fps_active{0};
inline std::atomic<bool> wonder_dynamic_fps_active{false};
inline std::atomic<u64> wonder_dynamic_fps_generation{0};

inline u32 GetActiveGameFps(u64 program_id) {
    if (program_id == WonderSwitch2ProgramId) {
        return wonder_fps_active.load(std::memory_order_relaxed);
    }
    if (program_id == TennisFpsProgramId) {
        return tennis_fps_active.load(std::memory_order_relaxed);
    }
    return 0;
}

inline constexpr bool ShowWonderFps(u64 program_id, bool global) {
    return !global && program_id == WonderSwitch2ProgramId;
}
inline constexpr bool ShowBananzaFps(u64 program_id, bool global) {
    return !global && program_id == BananzaFpsProgramId;
}

} // namespace Settings

// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <span>
#include "common/common_types.h"

namespace FileSys {
// The unpacked game NRO (nxboot's main NSO contains only its bootstrap).
inline constexpr std::array<u8, 32> TennisFpsModuleId{0x25, 0x0a, 0x51, 0x0a, 0x93, 0x41, 0x29,
                                                      0xcb, 0x38, 0xe9, 0x30, 0xc9, 0xa2, 0x90,
                                                      0x82, 0xb6, 0x97, 0xfd, 0x76, 0xc4};
inline constexpr u32 TennisReadHook = 0xd4000001U | (0xf00cU << 5);
inline constexpr u32 TennisBroadcastOffset = 0xf179c0;
inline constexpr u32 TennisBroadcastHook = 0xd4000001U | (0xf00eU << 5);
inline constexpr u32 TennisCounterHook = 0xd4000001U | (0xf00fU << 5);
inline constexpr u32 TennisRawCounterOffset = 0x11fa35c;
inline constexpr u32 TennisRawCompareOffset = 0x11fa37c;
inline constexpr u32 TennisRawCounterHook = 0xd4000001U | (0xf010U << 5);
inline constexpr u32 TennisRawCompareHook = 0xd4000001U | (0xf011U << 5);
inline constexpr u32 TennisStepOffset = 0x17995d4;
inline constexpr u32 TennisDirectWaitOffset = 0x95d880;
inline constexpr u32 TennisDirectWaitHook = 0xd4000001U | (0xf007U << 5);
inline constexpr u32 TennisFrameWaitOffset = 0x9664f4;
inline constexpr u32 TennisFrameWaitHook = 0xd4000001U | (0xf006U << 5);
inline constexpr u32 TennisUiStepOffset = 0x11ce2c4;
inline constexpr u32 TennisUiAuditOffset = 0x11ce2c8;
inline constexpr u32 TennisCharacterStepOffset = 0x106ed28;
inline constexpr u32 TennisUiStepHook = 0xd4000001U | (0xf008U << 5);
inline constexpr u32 TennisUiAuditHook = 0xd4000001U | (0xf009U << 5);
inline constexpr u32 TennisCharacterStepHook = 0xd4000001U | (0xf00aU << 5);
inline constexpr u32 TennisBallStepOffset = 0x1081bd8;
inline constexpr u32 TennisBallStepHook = 0xd4000001U | (0xf00bU << 5);
inline constexpr u32 TennisTimelineOffset = 0x10588e0;
inline constexpr u32 TennisTimelineHook = 0xd4000001U | (0xf005U << 5);
inline constexpr u32 TennisFpsHook = 0xd4000001U | (0xf003U << 5);

struct TennisStepStore {
    u32 offset;
    u32 instruction;
};
struct TennisCounter {
    u32 offset;
    u32 instruction;
    bool inclusive{};
};
inline constexpr std::array TennisCounters{
    TennisCounter{0x947810, 0x71000673},       TennisCounter{0x95757c, 0x710006b5},
    TennisCounter{0x9575c0, 0x710006b5},       TennisCounter{0x957a58, 0x710006b5},
    TennisCounter{0x95a41c, 0x710006b5},       TennisCounter{0x95a454, 0x710006b5},
    TennisCounter{0x96457c, 0x710006d6},       TennisCounter{0x9646d4, 0x710006d6},
    TennisCounter{0x9655c4, 0x710006d6},       TennisCounter{0x95c530, 0x710006b5, true},
    TennisCounter{0x965bf8, 0x710006d6, true}, TennisCounter{0x9616c4, 0x710006d6},
    TennisCounter{0x961c98, 0x710006d6},       TennisCounter{0x962c9c, 0x710006d6},
    TennisCounter{0x962f38, 0x710006d6},       TennisCounter{0x9631c0, 0x710006d6},
    TennisCounter{0x964d64, 0x710006d6},       TennisCounter{0x965024, 0x710006d6},
    TennisCounter{0x965798, 0x710006d6},       TennisCounter{0x965a98, 0x710006d6},
    TennisCounter{0x966518, 0x71000718},
};
inline constexpr std::array TennisStepLoads{
#include "core/file_sys/tennis_step_loads.inc"
};

// Fresh nominal speed setters only. Pause/restore writes and relative changes keep
// working on the already scaled value; hooking them would scale the step twice.
inline constexpr std::array TennisStepStores{
    TennisStepStore{0x94687c, 0xb90002e8},  // str w8, [x23], nominal 1.0
    TennisStepStore{0x94781c, 0xb90002a8},  // str w8, [x21], nominal 1.0
    TennisStepStore{0x103a044, 0xb9000109}, // str w9, [x8], nominal 1.0
    TennisStepStore{0x103a2b0, 0xb900012a}, // str w10, [x9], nominal 1.0
    TennisStepStore{0x103a334, 0xb9000109}, // str w9, [x8], nominal 1.0
    TennisStepStore{0x103c7f0, 0xb9000128}, // str w8, [x9], nominal 1.0
    TennisStepStore{0x103ca64, 0xb9000109}, // str w9, [x8], nominal 1.0
    TennisStepStore{0x1018fd4, 0xbd000100}, // str s0, [x8], replay speed byte / 64
    TennisStepStore{0x10de3c0, 0xbd000100}, // str s0, [x8], direct-address replay speed
};

struct TennisWaitInit {
    u32 offset;
    u32 instruction;
    u32 frames;
    bool inclusive;
};
// Fixed script/action timeouts, measured in original60-Hz frames.
inline constexpr std::array TennisWaitInits{
    TennisWaitInit{0x9477f8, 0x52801693, 180, false},
    TennisWaitInit{0x957568, 0x52802595, 300, false},
    TennisWaitInit{0x957a40, 0x52802595, 300, false},
    TennisWaitInit{0x95a40c, 0x52802595, 300, false},
    TennisWaitInit{0x96453c, 0x52802596, 300, false},
    TennisWaitInit{0x9646a0, 0x52802596, 300, false},
    TennisWaitInit{0x965598, 0x52802596, 300, false},
    TennisWaitInit{0x95c4fc, 0x52802575, 300, true},
    TennisWaitInit{0x965bd4, 0x52802576, 300, true},
    TennisWaitInit{0x961690, 0x52804b09, 600, false},
    TennisWaitInit{0x9616a0, 0x5280e108, 1800, false},
    TennisWaitInit{0x961c5c, 0x52804b09, 600, false},
    TennisWaitInit{0x961c6c, 0x5280e108, 1800, false},
    TennisWaitInit{0x962c68, 0x52804b09, 600, false},
    TennisWaitInit{0x962c78, 0x5280e108, 1800, false},
    TennisWaitInit{0x962f00, 0x52804b09, 600, false},
    TennisWaitInit{0x962f10, 0x5280e108, 1800, false},
    TennisWaitInit{0x96318c, 0x52804b09, 600, false},
    TennisWaitInit{0x96319c, 0x5280e108, 1800, false},
    TennisWaitInit{0x964d30, 0x52804b09, 600, false},
    TennisWaitInit{0x964d40, 0x5280e108, 1800, false},
    TennisWaitInit{0x964ff0, 0x52804b09, 600, false},
    TennisWaitInit{0x965000, 0x5280e108, 1800, false},
    TennisWaitInit{0x96575c, 0x52804b09, 600, false},
    TennisWaitInit{0x96576c, 0x5280e108, 1800, false},
    TennisWaitInit{0x965a60, 0x52804b09, 600, false},
    TennisWaitInit{0x965a70, 0x5280e108, 1800, false},
};

// The game's own rate is 60 FPS: the patch stays off there, as it changes gameplay (CPU
// opponents missing the ball) even when it keeps that rate.
inline constexpr bool TennisFpsPatchWanted(u32 fps) {
    return fps != 60;
}

inline constexpr float TennisSimulationStep(float nominal, u32 fps) {
    return nominal * (60.0f / fps);
}

// Carry fractional frames across integer script waits (e.g.144 FPS needs2/3 ticks
// per original60-Hz frame). The caller preserves negative sentinel values.
inline u32 TennisFrameDelay(u32 frames, u32 fps, u32& remainder) {
    const u64 ticks = static_cast<u64>(frames) * fps + remainder;
    remainder = static_cast<u32>(ticks % 60);
    return static_cast<u32>(std::min<u64>(ticks / 60, 0x7fffffff));
}

inline constexpr u32 TennisSubtractFlags(u32 left, u32 right) {
    const u32 result = left - right;
    return (result & 0x80000000U) | (result == 0 ? 0x40000000U : 0) |
           (left >= right ? 0x20000000U : 0) |
           ((((left ^ right) & (left ^ result)) >> 3) & 0x10000000U);
}

inline constexpr u32 TennisCompareOneFlags(u32 value) {
    const u32 result = value - 1;
    return (result & 0x80000000U) | (result == 0 ? 0x40000000U : 0U) |
           (value >= 1 ? 0x20000000U : 0U) | (value == 0x80000000U ? 0x10000000U : 0U);
}

inline bool ApplyTennisFpsPatch(std::span<u8> nro, std::span<const u8> module_id, u32 fps) {
    if (fps < 1 || fps > 1000 || module_id.size() != TennisFpsModuleId.size() ||
        !std::equal(module_id.begin(), module_id.end(), TennisFpsModuleId.begin()) ||
        nro.size() < TennisStepOffset + sizeof(u32)) {
        return false;
    }
    const auto read = [&](u32 offset) {
        u32 word{};
        std::memcpy(&word, nro.data() + offset, sizeof(word));
        return word;
    };
    if (read(TennisRawCounterOffset) != 0x51000674 || read(TennisRawCompareOffset) != 0x7100067f ||
        read(TennisBroadcastOffset) != 0x4d40c903 || read(0x25524) != 0x2a0203f5 ||
        read(0x11f76b4) != 0x1a9f17e8 || read(TennisStepOffset) != 0x3f800000 ||
        read(TennisDirectWaitOffset) != 0x2a1503e0 || read(TennisFrameWaitOffset) != 0x7100071f ||
        read(TennisUiStepOffset) != 0x1e2e1008 || read(TennisUiAuditOffset) != 0xf9412a68 ||
        read(TennisBallStepOffset) != 0x91004268 || read(TennisCharacterStepOffset) != 0xbd439101 ||
        read(TennisTimelineOffset) != 0xbd000320) {
        return false;
    }
    for (const auto& store : TennisStepStores) {
        if (read(store.offset) != store.instruction) {
            return false;
        }
    }
    for (const auto& counter : TennisCounters) {
        if (read(counter.offset) != counter.instruction)
            return false;
    }
    for (const auto& load : TennisStepLoads) {
        if (read(load.offset) != load.instruction) {
            return false;
        }
    }
    for (const auto& wait : TennisWaitInits) {
        if (read(wait.offset) != wait.instruction) {
            return false;
        }
    }
    const auto write = [&](u32 offset, u32 word) {
        std::memcpy(nro.data() + offset, &word, sizeof(word));
    };
    write(TennisRawCounterOffset, TennisRawCounterHook);
    write(TennisRawCompareOffset, TennisRawCompareHook);
    for (const auto& counter : TennisCounters)
        write(counter.offset, TennisCounterHook);
    write(TennisBroadcastOffset, TennisBroadcastHook);
    for (const auto& load : TennisStepLoads) {
        write(load.offset, TennisReadHook);
    }
    // Disable NVN's native present interval and its focused half-rate render branch.
    // The emulator's explicit FPS target now owns pacing, paired with the time step.
    write(0x25524, 0x52800015);   // mov w21, #0
    write(0x11f76b4, 0x52800028); // mov w8, #1
    for (const auto& wait : TennisWaitInits) {
        const u32 ticks = wait.frames * fps / 60 - static_cast<u32>(wait.inclusive);
        write(wait.offset, (wait.instruction & ~0x001fffe0U) | (ticks << 5));
    }
    write(TennisDirectWaitOffset, TennisDirectWaitHook);
    write(TennisFrameWaitOffset, TennisFrameWaitHook);
    write(TennisUiStepOffset, TennisUiStepHook);
    write(TennisUiAuditOffset, TennisUiAuditHook);
    write(TennisCharacterStepOffset, TennisCharacterStepHook);
    write(TennisBallStepOffset, TennisBallStepHook);
    write(TennisTimelineOffset, TennisTimelineHook);
    write(TennisStepOffset, std::bit_cast<u32>(TennisSimulationStep(1.0f, fps)));
    for (const auto& store : TennisStepStores) {
        write(store.offset, TennisFpsHook);
    }
    return true;
}
} // namespace FileSys

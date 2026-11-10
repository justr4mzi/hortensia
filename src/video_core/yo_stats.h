// SPDX-FileCopyrightText: Copyright 2026 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <cstdlib>
#include <map>
#include <string>
#include <unordered_map>

#include <fmt/format.h>

#include "common/common_types.h"

// ponytail: EDEN_YO_STATS=1 diagnostics (round 9). Per present: what the GPU thread did since the
// previous composite, and which op last wrote the presented address (draw/clear/dispatch/fermi2d
// or a texture-cache modification of the image at that address). Printed from AccelerateDisplay.
// Addresses are the rasterizer's CPU/device addresses (GPU VAs translated).
struct YoStats {
    const bool enabled{std::getenv("EDEN_YO_STATS") != nullptr};
    const std::chrono::steady_clock::time_point start{std::chrono::steady_clock::now()};
    u64 seq{};
    const char* op{"none"};
    u64 op_seq{};
    u32 draws{}, clears{}, dispatches{}, copies{}, modifications{};
    std::map<std::string, u32> skips;
    std::string rts;
    std::unordered_map<u64, std::string> last_writer;
    std::string pick;
    std::string draw_log; // per draw in this window: rt0>ps (ok) or !rt0>ps(reason) (skipped)
    u64 pick_cpu{}; // cpu address of the single image the composite used (0 = none)
    std::unordered_map<u64, u32> window_mods; // GPU modifications per image since last present

    void Begin(const char* kind) {
        op = kind;
        op_seq = ++seq;
    }
    std::string Op() const {
        return fmt::format("{}#{}", op, op_seq);
    }
    void NoteRT(u64 addr, u32 format, u32 width, u32 height) {
        if (addr == 0) {
            return;
        }
        last_writer[addr] = Op();
        const auto entry = fmt::format("{:#x}/fmt{}/{}x{} ", addr, format, width, height);
        if (rts.size() < 400 && rts.find(entry) == std::string::npos) {
            rts += entry;
        }
    }
    void NoteModified(u64 cpu_addr) {
        ++modifications;
        ++window_mods[cpu_addr];
        last_writer[cpu_addr] = Op();
    }
};

inline YoStats g_yo;

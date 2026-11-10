// SPDX-License-Identifier: GPL-3.0-or-later
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include "common/game_fps.h"
#include "core/file_sys/tennis_fps_patch.h"
#include "core/tennis_dynamic_fps.h"
#include "core/tennis_speed_meter.h"
#include "core/tennis_speed_audit.h"

namespace {
// Captured from the unpacked local game; independent of the production patch table.
constexpr std::array<u8, 32> ModuleId{0x25, 0x0a, 0x51, 0x0a, 0x93, 0x41, 0x29, 0xcb, 0x38, 0xe9,
                                      0x30, 0xc9, 0xa2, 0x90, 0x82, 0xb6, 0x97, 0xfd, 0x76, 0xc4};
constexpr std::array<std::array<u32, 2>, 20> CapturedWords{{
    {0x25524, 0x2a0203f5},   {0x11f76b4, 0x1a9f17e8}, {0x17995d4, 0x3f800000},
    {0x94687c, 0xb90002e8},  {0x94781c, 0xb90002a8},  {0x103a044, 0xb9000109},
    {0x103a2b0, 0xb900012a}, {0x103a334, 0xb9000109}, {0x103c7f0, 0xb9000128},
    {0x103ca64, 0xb9000109}, {0x1018fd4, 0xbd000100}, {0x10de3c0, 0xbd000100},
    {0x10588e0, 0xbd000320}, {0x11ce2c4, 0x1e2e1008}, {0x1081bd8, 0x91004268},
    {0x11ce2c8, 0xf9412a68}, {0x106ed28, 0xbd439101}, {0x9477f8, 0x52801693},
    {0x9664f4, 0x7100071f},  {0x95d880, 0x2a1503e0},
}};
constexpr std::array<std::array<u32, 4>, 27> CapturedWaits{{
    {0x9477f8, 0x52801693, 180, 0},  {0x957568, 0x52802595, 300, 0},
    {0x957a40, 0x52802595, 300, 0},  {0x95a40c, 0x52802595, 300, 0},
    {0x96453c, 0x52802596, 300, 0},  {0x9646a0, 0x52802596, 300, 0},
    {0x965598, 0x52802596, 300, 0},  {0x95c4fc, 0x52802575, 300, 1},
    {0x965bd4, 0x52802576, 300, 1},  {0x961690, 0x52804b09, 600, 0},
    {0x9616a0, 0x5280e108, 1800, 0}, {0x961c5c, 0x52804b09, 600, 0},
    {0x961c6c, 0x5280e108, 1800, 0}, {0x962c68, 0x52804b09, 600, 0},
    {0x962c78, 0x5280e108, 1800, 0}, {0x962f00, 0x52804b09, 600, 0},
    {0x962f10, 0x5280e108, 1800, 0}, {0x96318c, 0x52804b09, 600, 0},
    {0x96319c, 0x5280e108, 1800, 0}, {0x964d30, 0x52804b09, 600, 0},
    {0x964d40, 0x5280e108, 1800, 0}, {0x964ff0, 0x52804b09, 600, 0},
    {0x965000, 0x5280e108, 1800, 0}, {0x96575c, 0x52804b09, 600, 0},
    {0x96576c, 0x5280e108, 1800, 0}, {0x965a60, 0x52804b09, 600, 0},
    {0x965a70, 0x5280e108, 1800, 0},
}};
std::vector<u8> CapturedModule() {
    std::vector<u8> module(0x179e000);
    for (const auto& word : CapturedWords) {
        std::memcpy(module.data() + word[0], &word[1], sizeof(u32));
    }
    for (const auto& wait : CapturedWaits) {
        std::memcpy(module.data() + wait[0], &wait[1], sizeof(u32));
    }
    for (const auto& load : FileSys::TennisStepLoads) {
        std::memcpy(module.data() + load.offset, &load.instruction, sizeof(u32));
    }
    for (const auto& counter : FileSys::TennisCounters) {
        std::memcpy(module.data() + counter.offset, &counter.instruction, sizeof(u32));
    }
    constexpr u32 raw_sub = 0x51000674, raw_cmp = 0x7100067f;
    std::memcpy(module.data() + 0x11fa35c, &raw_sub, sizeof(u32));
    std::memcpy(module.data() + 0x11fa37c, &raw_cmp, sizeof(u32));
    constexpr u32 broadcast = 0x4d40c903;
    std::memcpy(module.data() + 0xf179c0, &broadcast, sizeof(u32));
    // The timeline STR to the shared step remains native after S0 was scaled upstream.
    constexpr u32 native_store = 0xbd000360;
    std::memcpy(module.data() + 0x10588e8, &native_store, sizeof(u32));
    return module;
}
u32 ReadWord(const std::vector<u8>& module, u32 offset) {
    u32 word{};
    std::memcpy(&word, module.data() + offset, sizeof(word));
    return word;
}
} // namespace

TEST_CASE("Tennis high FPS pairs presentation and actual simulation step", "[tennis_fps]") {
    auto module = CapturedModule();
    REQUIRE(FileSys::ApplyTennisFpsPatch(module, ModuleId, 120));
    REQUIRE(ReadWord(module, 0x9477f8) == (0x52800013U | (360U << 5)));
    REQUIRE(ReadWord(module, 0x95d880) == 0xd41e00e1);
    REQUIRE(ReadWord(module, 0x9664f4) == 0xd41e00c1);
    REQUIRE(ReadWord(module, 0x25524) == 0x52800015);
    REQUIRE(ReadWord(module, 0x11f76b4) == 0x52800028);
    REQUIRE(std::bit_cast<float>(ReadWord(module, 0x17995d4)) == 0.5f);
    REQUIRE(ReadWord(module, 0x10588e0) == 0xd41e00a1);
    REQUIRE(ReadWord(module, 0x10588e8) == 0xbd000360);
    REQUIRE(ReadWord(module, 0x11ce2c4) == 0xd41e0101);
    REQUIRE(ReadWord(module, 0x11ce2c8) == 0xd41e0121);
    REQUIRE(ReadWord(module, 0x106ed28) == 0xd41e0141);
    REQUIRE(ReadWord(module, 0x1081bd8) == 0xd41e0161);
    for (const auto& word : CapturedWords) {
        if (word[0] != 0x25524 && word[0] != 0x11f76b4 && word[0] != 0x17995d4 &&
            word[0] != 0x10588e0 && word[0] != 0x11ce2c4 && word[0] != 0x1081bd8 &&
            word[0] != 0x11ce2c8 && word[0] != 0x106ed28 && word[0] != 0x9477f8 &&
            word[0] != 0x9664f4 && word[0] != 0x95d880) {
            REQUIRE(ReadWord(module, word[0]) == 0xd41e0061);
        }
    }
    REQUIRE_FALSE(FileSys::ApplyTennisFpsPatch(module, ModuleId, 120));
}

TEST_CASE("Tennis patch stays off at the game's native 60 FPS", "[tennis_fps]") {
    CHECK_FALSE(FileSys::TennisFpsPatchWanted(60));
    CHECK(FileSys::TennisFpsPatchWanted(59));
    CHECK(FileSys::TennisFpsPatchWanted(61));
    CHECK(FileSys::TennisFpsPatchWanted(120));
}

TEST_CASE("Tennis patch rejects unknown or changed code atomically", "[tennis_fps]") {
    const auto original = CapturedModule();
    auto module = original;
    auto id = ModuleId;
    id[0] ^= 1;
    REQUIRE_FALSE(FileSys::ApplyTennisFpsPatch(module, id, 120));
    REQUIRE(module == original);
    REQUIRE_FALSE(FileSys::ApplyTennisFpsPatch(module, ModuleId, 0));
    REQUIRE_FALSE(FileSys::ApplyTennisFpsPatch(module, ModuleId, 1001));
    REQUIRE(module == original);
    REQUIRE_FALSE(FileSys::ApplyTennisFpsPatch(std::span{module}.first(0x17995d7), ModuleId, 120));
    REQUIRE(module == original);
    for (const auto& word : CapturedWords) {
        module = original;
        module[word[0]] ^= 1;
        const auto changed = module;
        REQUIRE_FALSE(FileSys::ApplyTennisFpsPatch(module, ModuleId, 120));
        REQUIRE(module == changed);
    }
    for (const auto& wait : CapturedWaits) {
        module = original;
        module[wait[0]] ^= 1;
        const auto changed = module;
        REQUIRE_FALSE(FileSys::ApplyTennisFpsPatch(module, ModuleId, 120));
        REQUIRE(module == changed);
    }
}

TEST_CASE("Tennis fractional steps advance one second without destroying slow motion or pause",
          "[tennis_fps]") {
    for (const u32 fps : {1U, 30U, 60U, 90U, 120U, 144U, 240U, 1000U}) {
        const double step = FileSys::TennisSimulationStep(1.0f, fps);
        REQUIRE(std::abs(fps * step / 60.0 - 1.0) < 0.000001);
        REQUIRE(FileSys::TennisSimulationStep(0.0f, fps) == 0.0f);
        REQUIRE(FileSys::TennisSimulationStep(0.25f, fps) ==
                FileSys::TennisSimulationStep(1.0f, fps) * 0.25f);
        auto module = CapturedModule();
        REQUIRE(FileSys::ApplyTennisFpsPatch(module, ModuleId, fps));
        REQUIRE(std::bit_cast<float>(ReadWord(module, 0x17995d4)) == step);
        for (const auto& wait : CapturedWaits) {
            REQUIRE(ReadWord(module, wait[0]) ==
                    ((wait[1] & ~0x001fffe0U) | ((wait[2] * fps / 60 - wait[3]) << 5)));
        }
    }
}

TEST_CASE("Tennis speed audit measures readback against real elapsed time", "[tennis_fps]") {
    for (const u32 fps : {1U, 30U, 60U, 120U, 144U, 240U}) {
        Core::TennisSpeedAudit audit;
        const float step = FileSys::TennisSimulationStep(1.0f, fps);
        REQUIRE_FALSE(audit.Observe(0, 0, step, fps));
        bool report = false;
        for (u32 frame = 1; frame <= fps * 5; ++frame) {
            report = audit.Observe(frame, static_cast<s64>(frame) * 1'000'000'000 / fps, step, fps);
        }
        REQUIRE(report);
        REQUIRE(audit.frames == fps * 5);
        REQUIRE(std::abs(audit.GameSpeed() - 1.0) < 0.000001);
        REQUIRE(std::abs(audit.ActiveGameSpeed() - 1.0) < 0.000001);
        const u64 frames = audit.frames;
        REQUIRE_FALSE(audit.Observe(fps * 5, 5'000'000'000LL, step, fps));
        REQUIRE(audit.frames == frames);
        REQUIRE_FALSE(audit.covered);
        REQUIRE(audit.last_elapsed == 0);
        REQUIRE(std::isnan(audit.sample_speed));
        audit.ClearWindow();
        REQUIRE(audit.frames == 0);
        REQUIRE(audit.wall_seconds == 0);
    }
    Core::TennisSpeedAudit slow;
    slow.Observe(0, 0, 0.5f, 120);
    for (u32 frame = 1; frame <= 300; ++frame) {
        slow.Observe(frame, static_cast<s64>(frame) * 1'000'000'000 / 60, 0.5f, 120);
    }
    // A requested120 FPS with only60 actual ticks is slow; the audit must not report1.
    REQUIRE(std::abs(slow.GameSpeed() - 0.5) < 0.000001);
}

TEST_CASE("Tennis speed audit separates pauses and uncovered stalls", "[tennis_fps]") {
    Core::TennisSpeedAudit audit;
    audit.Observe(0, 0, 1.0f, 60);
    audit.Observe(1, 16'666'667, 1.0f, 60);
    audit.Observe(2, 33'333'334, 0.0f, 60);
    REQUIRE(std::abs(audit.GameSpeed() - 0.5) < 0.000001);
    REQUIRE(std::abs(audit.ActiveGameSpeed() - 1.0) < 0.000001);
    REQUIRE(audit.paused_seconds > 0);
    audit.Observe(3, 1'033'333'334, 1.0f, 60);
    REQUIRE_FALSE(audit.covered);
    REQUIRE(audit.uncovered_seconds == 1.0);
    audit.Observe(4, 1'050'000'001, std::numeric_limits<float>::quiet_NaN(), 60);
    REQUIRE_FALSE(audit.covered);
    REQUIRE(std::isnan(audit.sample_speed));
}

TEST_CASE("Game FPS pacing uses the installed target only", "[tennis_fps]") {
    Settings::tennis_fps_active.store(120);
    Settings::wonder_fps_active.store(144);
    REQUIRE(Settings::GetActiveGameFps(Settings::TennisFpsProgramId) == 120);
    REQUIRE(Settings::GetActiveGameFps(Settings::WonderSwitch2ProgramId) == 144);
    REQUIRE(Settings::GetActiveGameFps(0) == 0);
    Settings::tennis_fps_active.store(0);
    Settings::wonder_fps_active.store(0);
    REQUIRE(Settings::GetActiveGameFps(Settings::TennisFpsProgramId) == 0);
    REQUIRE(Settings::ShowTennisFps(Settings::TennisFpsProgramId, false));
    REQUIRE_FALSE(Settings::ShowTennisFps(Settings::WonderSwitch2ProgramId, false));
    REQUIRE_FALSE(Settings::ShowTennisFps(Settings::TennisFpsProgramId, true));
}

TEST_CASE("Integer script waits retain real duration at fractional and high FPS", "[tennis_fps]") {
    for (const u32 fps : {1U, 30U, 60U, 120U, 144U, 240U, 1000U}) {
        u32 remainder = 0;
        u32 ticks = 0;
        for (u32 frame = 0; frame < 60; ++frame) {
            ticks += FileSys::TennisFrameDelay(1, fps, remainder);
        }
        REQUIRE(ticks == fps);
        REQUIRE(remainder == 0);
        REQUIRE(FileSys::TennisFrameDelay(180, fps, remainder) == 3 * fps);
    }
    REQUIRE(FileSys::TennisCompareOneFlags(0) == 0x80000000U);
    REQUIRE(FileSys::TennisCompareOneFlags(1) == 0x60000000U);
    REQUIRE(FileSys::TennisCompareOneFlags(2) == 0x20000000U);
    REQUIRE(FileSys::TennisCompareOneFlags(0xffffffffU) == 0xa0000000U);
    REQUIRE(FileSys::TennisCompareOneFlags(0x80000000U) == 0x30000000U);
    u32 remainder = 0;
    REQUIRE(FileSys::TennisFrameDelay(0x7fffffff, 1000, remainder) == 0x7fffffffU);
}

TEST_CASE("Tennis dynamic input clocks keep real time when120 drops to90", "[tennis_fps]") {
    Core::TennisDynamicFps timing;
    timing.Advance(1, 0, 0, 120, 100);
    timing.Scale(0.5f, 120, 1);
    double game_frames{};
    for (u32 frame = 1; frame <= 900; ++frame) {
        timing.Advance(1, frame, s64(frame) * 1'000'000'000 / 90, 120, 100);
        const float step = timing.Scale(0.5f, 120, 1);
        REQUIRE(step == timing.Scale(0.5f, 120, 1)); // many readers, one phase advance
        game_frames += std::trunc(step * 64) / 64;
    }
    REQUIRE(std::abs(game_frames / 60 - 10) <= 1.0 / 3840);
}

TEST_CASE("Tennis changing FPS preserves pause control and slow motion", "[tennis_fps]") {
    for (const u32 target : {30U, 120U, 144U, 1000U}) {
        Core::TennisDynamicFps timing;
        timing.Advance(7, 0, 0, target, 100);
        const float canonical = FileSys::TennisSimulationStep(1, target);
        timing.Scale(canonical, target, 7);
        timing.Scale(canonical * 0.5f, target, 7);
        double normal{}, slow{};
        s64 now{};
        for (u32 frame = 1; frame <= 900; ++frame) {
            const u32 actual = frame <= 300 ? target : frame <= 600 ? 90 : 48;
            now += 1'000'000'000 / actual;
            timing.Advance(7, frame, now, target, 100);
            normal += timing.Scale(canonical, target, 7) / 60;
            REQUIRE(timing.Scale(0, target, 7) == 0);
            slow += timing.Scale(canonical * 0.5f, target, 7) / 60;
        }
        REQUIRE(std::abs(normal - now / 1e9) < 1.0 / 3840);
        REQUIRE(std::abs(slow - now / 2e9) < 1.0 / 3840);
        // Long stop and process/target changes do not cause a catch-up jump.
        timing.Advance(7, 901, now + 10'000'000'000LL, target, 100);
        REQUIRE(timing.Scale(canonical, target, 7) <= canonical + 1.0f / 64);
        timing.Advance(8, 0, 0, 60, 100);
        REQUIRE(timing.Scale(1, 60, 8) == 1);
        REQUIRE(timing.Scale(canonical, target, 7) == canonical);
    }
}

TEST_CASE("Tennis dynamic patch guards its read sites and preserves canonical saves",
          "[tennis_fps]") {
    auto module = CapturedModule();
    const auto original = module;
    REQUIRE(FileSys::ApplyTennisFpsPatch(module, ModuleId, 120));
    REQUIRE(ReadWord(module, 0xf179c0) == FileSys::TennisBroadcastHook);
    for (const auto& load : FileSys::TennisStepLoads) {
        REQUIRE(ReadWord(module, load.offset) == FileSys::TennisReadHook);
        REQUIRE((load.instruction & 0xfffffc00) == 0xbd400000);
        REQUIRE(((load.instruction >> 5) & 31) < 29);
        for (const u32 saved : {0x9469b8U, 0x975198U, 0x9755bcU, 0x980c00U, 0x982118U, 0x9825fcU,
                                0x982838U, 0x987380U, 0x9876f4U, 0xf949b8U}) {
            REQUIRE(load.offset != saved);
        }
    }
    for (const u32 offset :
         {FileSys::TennisStepLoads.front().offset, FileSys::TennisStepLoads[348].offset,
          FileSys::TennisStepLoads.back().offset, FileSys::TennisBroadcastOffset}) {
        module = original;
        module[offset] ^= 1;
        const auto changed = module;
        REQUIRE_FALSE(FileSys::ApplyTennisFpsPatch(module, ModuleId, 120));
        REQUIRE(module == changed);
    }
}

TEST_CASE("Tennis script counters consume wall time across an FPS drop", "[tennis_fps]") {
    for (const bool inclusive : {false, true}) {
        Core::TennisDynamicFps timing;
        timing.Advance(1, 0, 0, 120, 100);
        u32 remaining = inclusive ? 599 : 600;
        s64 now{};
        u32 frame{};
        for (; frame < 1000; ++frame) {
            now += frame < 120 ? 1'000'000'000 / 120 : 1'000'000'000 / 90;
            timing.Advance(1, frame + 1, now, 120, 100);
            const u32 before = remaining;
            remaining = timing.Consume(17, 22, remaining, inclusive, 120, 1, now);
            const u32 flags = FileSys::TennisSubtractFlags(before, before - remaining);
            if (inclusive ? !(flags & 0x20000000U) : (flags & 0x40000000U))
                break;
        }
        REQUIRE(frame < 1000);
        REQUIRE(std::abs(now / 1e9 - 5) < 1.0 / 90);
        REQUIRE(remaining == (inclusive ? 0xffffffffU : 0));
    }
    REQUIRE(FileSys::TennisSubtractFlags(0x80000000U, 1) == 0x30000000U);
    REQUIRE(FileSys::TennisSubtractFlags(0, 1) == 0x80000000U);
}

TEST_CASE("Tennis counter polling cannot reuse one frame's elapsed time", "[tennis_fps]") {
    Core::TennisDynamicFps timing;
    timing.Advance(1, 0, 0, 120, 100);
    timing.Advance(1, 1, 11'111'111, 120, 100);
    u32 remaining = timing.Consume(7, 21, 600, false, 120, 1, 11'111'111);
    const u32 after_frame = remaining;
    for (int poll = 0; poll < 450; ++poll) {
        remaining = timing.Consume(7, 21, remaining, false, 120, 1, 11'111'111);
    }
    REQUIRE(remaining == after_frame);
    for (s64 tick = 1; tick <= 5000 && remaining != 0; ++tick) {
        // A callback may poll without submitting another present; real time still passes.
        remaining = timing.Consume(7, 21, remaining, false, 120, 1, 11'111'111 + tick * 1'000'000);
    }
    REQUIRE(remaining == 0);
    REQUIRE(timing.Consume(7, 21, 600, false, 120, 1, 100'000'000) >= 598);
}


TEST_CASE("Tennis status meter measures consumer speed rather than target speed", "[tennis_fps]") {
    Core::TennisSpeedMeter meter;
    for (u32 frame = 0; frame <= 90; ++frame) {
        const s64 now = s64(frame) * 1'000'000'000 / 90;
        meter.Observe(1, false, 100, now, 2.0f / 3, 120);
        meter.Observe(1, false, 200, now, 2.0f / 3, 120); // no double counting
    }
    REQUIRE(meter.Read(1, 1'000'000'000).has_value());
    REQUIRE(std::abs(*meter.Read(1, 1'000'000'000) - 1) < 0.00001);
    for (u32 frame = 0; frame <= 90; ++frame) {
        const s64 now = 1'000'000'000 + s64(frame) * 1'000'000'000 / 90;
        meter.Observe(1, false, 100, now, 2.0f / 3, 120);
        meter.Observe(1, true, 300, now, 0.5f, 120);
    }
    REQUIRE(std::abs(*meter.Read(1, 2'000'000'000) - 0.75) < 0.00001);
    REQUIRE_FALSE(meter.Read(2, 2'000'000'000));
    REQUIRE_FALSE(meter.Read(1, 4'000'000'000));
    for (u32 frame = 1; frame <= 90; ++frame) {
        meter.Observe(1, true, 300, 2'000'000'000 + s64(frame)*1'000'000'000/90, 0, 120);
    }
    REQUIRE(*meter.Read(1, 3'000'000'000) == 0);
    meter.Observe(2, true, 300, 3'000'000'001, 1, 60);
    REQUIRE_FALSE(meter.Read(1, 3'000'000'001));
    REQUIRE_FALSE(meter.Read(2, 3'000'000'001));
}

TEST_CASE("Tennis status keeps an active object and replaces a stale scene object", "[tennis_fps]") {
    Core::TennisSpeedMeter meter;
    for (u32 frame = 0; frame <= 60; ++frame) {
        meter.Observe(1, false, 10, s64(frame)*10'000'000, 0.6f, 120);
    }
    REQUIRE(meter.Read(1, 600'000'000));
    meter.Observe(1, false, 20, 610'000'000, 0, 120);
    REQUIRE(meter.Read(1, 620'000'000));
    REQUIRE(std::abs(*meter.Read(1, 620'000'000) - 1) < 0.00001);
    meter.Observe(1, false, 20, 1'200'000'000, 0.3f, 120);
    REQUIRE_FALSE(meter.Read(1, 1'200'000'000));
    for (u32 frame = 1; frame <= 60; ++frame) {
        meter.Observe(1, false, 20, 1'200'000'000+s64(frame)*10'000'000, 0.3f, 120);
    }
    REQUIRE(meter.Read(1, 1'800'000'000));
    REQUIRE(std::abs(*meter.Read(1, 1'800'000'000) - 0.5) < 0.00001);
}

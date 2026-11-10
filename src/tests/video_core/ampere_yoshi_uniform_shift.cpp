// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include "shader_recompiler/frontend/ampere/lower_to_maxwell.h"

namespace {
using namespace Shader::Ampere;
constexpr Instruction Captured{{0x3f0b0299, 0x00000005, 0x08011609, 0x000fe200}};
constexpr Instruction Exit{{0x0000794d, 0, 0x03800000, 0}};

TEST_CASE("Yoshi UP0 uniform right shift preserves guard and operands",
          "[video_core][yoshi_ushf]") {
    for (const u32 dest : {11U, 5U, 9U}) {
        auto insn = Captured;
        insn.words[0] = (insn.words[0] & ~0xff0000U) | (dest << 16);
        const auto lowered = LowerToMaxwell(std::array{insn, Exit});
        INFO(lowered.error);
        REQUIRE(lowered);
        REQUIRE(lowered.instructions.size() == 5);
        constexpr u64 mask = 64;
        CHECK(lowered.instructions[0] ==
              (0x38e8000000070000ULL | 253ULL | (255ULL << 8) | (mask << 20)));
        CHECK(lowered.instructions[1] == (0x38f0000000070000ULL | (239ULL << 8) | (mask << 20)));
        CHECK(lowered.instructions[2] == (0x5c28000000000000ULL | (6ULL << 16) | (216ULL + dest) |
                                          (225ULL << 8) | (221ULL << 20)));
        CHECK(lowered.instructions[3] == (0x38f0000000070000ULL | (253ULL << 8) | (mask << 20)));
        for (const u32 value : {0U, 1U, 0x80000001U, 0xffffffffU}) {
            for (const u32 amount : {0U, 1U, 31U, 32U, 63U, 255U}) {
                for (const bool guard : {false, true}) {
                    for (const bool guest_p6 : {false, true}) {
                        std::array<u32, 256> regs{};
                        regs[221] = amount;
                        regs[225] = value;
                        regs[216 + dest] = 0xabcdef01;
                        // Aliased destinations begin with their source's original value.
                        if (dest == 5)
                            regs[221] = amount;
                        if (dest == 9)
                            regs[225] = value;
                        const u32 before = regs[216 + dest], source = regs[225], count = regs[221];
                        regs[239] = guard ? mask : 0;
                        bool p6 = guest_p6;
                        for (u64 w : lowered.instructions) {
                            if ((w >> 48) == 0x38e8)
                                regs[253] = p6 ? mask : 0;
                            else if ((w >> 48) == 0x38f0)
                                p6 = (regs[(w >> 8) & 255] & mask) != 0;
                            else if ((w >> 48) == 0x5c28 && p6)
                                regs[w & 255] = count < 32 ? source >> count : 0;
                        }
                        const u64 wide = u64{source} << 32;
                        const u32 expected =
                            guard ? static_cast<u32>((wide >> std::min(count, 32U)) >> 32) : before;
                        CHECK(regs[216 + dest] == expected);
                        CHECK(p6 == guest_p6);
                        CHECK(regs[239] == (guard ? mask : 0));
                    }
                }
            }
        }
    }
}

TEST_CASE("Yoshi uniform shift retains unsupported mode guards", "[video_core][yoshi_ushf]") {
    for (const u32 guard : {1U, 8U}) {
        auto i = Captured;
        i.words[0] = (i.words[0] & ~0xf000U) | (guard << 12);
        CHECK_FALSE(LowerToMaxwell(std::array{i, Exit}));
    }
    for (const auto [word, bit] :
         std::array{std::pair{2U, 8U}, std::pair{2U, 10U}, std::pair{2U, 9U}}) {
        auto i = Captured;
        i.words[word] ^= 1U << bit;
        CHECK_FALSE(LowerToMaxwell(std::array{i, Exit}));
    }
    for (const u32 bit : {0U, 8U, 30U, 31U}) {
        auto i = Captured;
        i.words[3] ^= 1U << bit;
        CHECK_FALSE(LowerToMaxwell(std::array{i, Exit}));
    }
    for (const u32 bit : {11U, 12U, 16U}) {
        auto i = Captured;
        i.words[2] ^= 1U << bit;
        CHECK_FALSE(LowerToMaxwell(std::array{i, Exit}));
    }
}
} // namespace

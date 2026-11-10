// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>
#include <vector>
#include <catch2/catch_test_macros.hpp>

#include "shader_recompiler/frontend/ampere/lower_to_maxwell.h"

namespace {
using Shader::Ampere::Instruction;
using Shader::Ampere::LowerToMaxwell;

constexpr Instruction Exit{{0x0000794d, 0, 0x03800000, 0}};

TEST_CASE("Ampere constant LEA without a carry out lowers to ISCADD alone",
          "[video_core][ampere]") {
    // LEA R0, R0, c[0][0x2e0]-style constant, 0x8 (Pokemon Pokopia's album compute shaders).
    std::array<Instruction, 2> code{{{{0x00007a11, 0x01002e00, 0x078e40ff, 0x000fe200}}, Exit}};
    const auto lowered = LowerToMaxwell(code);
    REQUIRE(static_cast<bool>(lowered));
    // R251 = the constant, then ISCADD R0, R0, R251, 8 and no ISETP for a carry.
    REQUIRE(lowered.instructions.size() == 3);
    REQUIRE(lowered.instructions[1] == 0x5c1804000fb70000ULL);
    // A carry out to P1 keeps its compare.
    code[0].words[2] = (code[0].words[2] & ~(7U << 17)) | (1U << 17);
    const auto carry = LowerToMaxwell(code);
    REQUIRE(static_cast<bool>(carry));
    REQUIRE(std::ranges::any_of(carry.instructions, [](u64 i) { return (i >> 52) == 0x5b6; }));
}

TEST_CASE("Ampere DEPBAR.LE with a wait list is a no-op", "[video_core][ampere]") {
    // DEPBAR.LE SB5, 0x1 and DEPBAR.LE SB0, 0x0, {3,2,1} (Pokemon Pokopia's photo compute shaders).
    for (const u32 word : {0x0000d040U, 0x0000800eU}) {
        std::array<Instruction, 2> code{{{{0x0000791a, word, 0, 0x000fce00}}, Exit}};
        const auto lowered = LowerToMaxwell(code);
        REQUIRE(static_cast<bool>(lowered));
        REQUIRE(lowered.instructions.size() == 1);
        code[0].words[2] = 1;
        REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(code)));
    }
}
TEST_CASE("Ampere dual-destination PLOP3.LUT writes both predicates from the old inputs",
          "[video_core][ampere]") {
    // PLOP3.LUT P0, P1, P0, PT, P1, 0x80, 0xc8 (Star Fox's composite fragment shader).
    std::array<Instruction, 2> code{{{{0x00c8781c, 0, 0x0010f010, 0x000fc400}}, Exit}};
    const auto lowered = LowerToMaxwell(code);
    REQUIRE(static_cast<bool>(lowered));
    // Three input SELs, then one LOP3 per destination, both after every SEL.
    std::vector<u64> lop3;
    for (const u64 i : lowered.instructions)
        if ((i >> 52) == 0x5be) lop3.push_back(i);
    REQUIRE(lop3.size() == 2);
    REQUIRE(((lop3[0] >> 28) & 0xff) == 0x80);
    REQUIRE(((lop3[0] >> 48) & 7) == 0);
    REQUIRE(((lop3[1] >> 28) & 0xff) == 0xc8);
    REQUIRE(((lop3[1] >> 48) & 7) == 1);
    REQUIRE((lowered.instructions[3] >> 52) == 0x5be);
    // A second table without a second destination stays rejected.
    code[0].words[2] |= 7U << 20;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(code)));
}
} // namespace

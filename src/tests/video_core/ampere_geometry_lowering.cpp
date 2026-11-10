// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>

#include "shader_recompiler/frontend/ampere/lower_to_maxwell.h"

namespace {
using Shader::Ampere::Instruction;
using Shader::Ampere::LowerToMaxwell;

constexpr Instruction Exit{{0x0000794d, 0, 0x03800000, 0x001fea00}};
constexpr Instruction Final{{0x00ff7324, 0xff, 0, 0x0001e200}};
}

TEST_CASE("Ampere geometry instructions require an explicit geometry stage",
          "[video_core][ampere][geometry]") {
    for (const Instruction instruction : std::array{
             Final,
             Instruction{{0x030c7923, 0, 0, 0x000e2800}},
             Instruction{{0x0000734e, 0, 0, 0x000fe400}}}) {
        REQUIRE_FALSE(LowerToMaxwell(std::array{instruction, Exit}));
    }
    // Only a geometry stage takes the LEPC exit envelope; elsewhere LEPC lowers as the zero
    // address it reads in the lowered program.
    const auto lepc = LowerToMaxwell(std::array{Instruction{{0x0000734e, 0, 0, 0x000fe400}}, Exit});
    REQUIRE(lepc);
    REQUIRE(lepc.instructions == std::vector<u64>{0x010000000007f000ULL, 0x010000000007f001ULL,
                                                  0xe30000000007000fULL});
}

TEST_CASE("Ampere geometry finalization neither emits nor cuts",
          "[video_core][ampere][geometry]") {
    // OUT into RZ with neither emit nor cut has no effect: it lowers to nothing.
    const auto lowered = LowerToMaxwell(std::array{Final, Exit}, true);
    INFO(lowered.error);
    REQUIRE(lowered);
    REQUIRE(lowered.instructions.size() == 1);
    REQUIRE(lowered.instructions.back() == 0xe30000000007000fULL);
}

TEST_CASE("Ampere geometry emits and cuts with the original predicate and stream",
          "[video_core][ampere][geometry]") {
    for (const u32 opcode : {0x324U, 0x924U}) {
        for (u32 type = 1; type <= 3; ++type) {
            for (u32 predicate = 0; predicate < 15; ++predicate) {
                Instruction out{{0x03ff0000 | (predicate << 12) | opcode, 2, type << 14, 0}};
                const auto lowered = LowerToMaxwell(std::array{out, Exit}, true);
                INFO(lowered.error);
                REQUIRE(lowered);
                REQUIRE(lowered.instructions.size() == 2);
                const u64 word = lowered.instructions.front();
                REQUIRE((word & 0xff) == 255);
                REQUIRE(((word >> 8) & 0xff) == 3);
                REQUIRE(((word >> 16) & 15) == predicate);
                REQUIRE(((word >> 20) & 0x7ffff) == 2);
                REQUIRE(((word >> 39) & 3) == type);
            }
        }
    }
}

TEST_CASE("Ampere geometry rejects unrepresented finalization and stream modes",
          "[video_core][ampere][geometry]") {
    for (const Instruction out : std::array{
             Instruction{{0x00007324, 255, 0, 0}},
             Instruction{{0x00ff7324, 255, 1, 0}},
             Instruction{{0x00ff7324, 0x100, 0x4000, 0}},
             Instruction{{0x00ff7924, 0x80000, 0x4000, 0}}}) {
        REQUIRE_FALSE(LowerToMaxwell(std::array{out, Exit}, true));
    }
    // Any other stream with neither emit nor cut into RZ is the same no-op as the finalization.
    for (const Instruction out : std::array{
             Instruction{{0x00ff7924, 0, 0, 0}},
             Instruction{{0x00ff7324, 0, 0, 0}}}) {
        const auto lowered = LowerToMaxwell(std::array{out, Exit}, true);
        REQUIRE(lowered);
        REQUIRE(lowered.instructions == std::vector<u64>{0xe30000000007000fULL});
    }
}

TEST_CASE("Ampere geometry stage buffer selectors preserve vertex ordinals",
          "[video_core][ampere][geometry]") {
    const Instruction read{{0x030c7923, 0, 0, 0x000e2800}};
    const auto lowered = LowerToMaxwell(std::array{read, Exit}, true);
    INFO(lowered.error);
    REQUIRE(lowered);
    REQUIRE(lowered.instructions.front() == 0x5c9807800037000cULL);
    for (const Instruction invalid : std::array{
             Instruction{{0xff0c7923, 0, 0, 0}},
             Instruction{{0x030c7923, 1, 0, 0}},
             Instruction{{0x030c7923, 0, 1, 0}}}) {
        REQUIRE_FALSE(LowerToMaxwell(std::array{invalid, Exit}, true));
    }
}

TEST_CASE("Ampere geometry invocation registers use the existing host geometry ABI",
          "[video_core][ampere][geometry]") {
    for (const u32 special : {17U, 29U}) {
        const Instruction read{{0x00027919, 0, special << 8, 0x000fe400}};
        const auto lowered = LowerToMaxwell(std::array{read, Exit}, true);
        INFO(lowered.error);
        REQUIRE(lowered);
        REQUIRE(lowered.instructions.front() == (0xf0c8000000070002ULL | (u64{special} << 20)));
    }
}

TEST_CASE("Ampere geometry index conversion retains U16 truncation and FTZ",
          "[video_core][ampere][geometry]") {
    const Instruction convert{{0x00027305, 2, 0x0021e800, 0x002fe200}};
    const auto lowered = LowerToMaxwell(std::array{convert, Exit}, true);
    INFO(lowered.error);
    REQUIRE(lowered);
    REQUIRE(lowered.instructions.size() == 4);
    const u64 word = lowered.instructions[2];
    REQUIRE((word & 0xff) == 2);
    REQUIRE(((word >> 8) & 3) == 1);
    REQUIRE(((word >> 10) & 3) == 2);
    REQUIRE(((word >> 12) & 1) == 0);
    REQUIRE(((word >> 39) & 3) == 3);
    REQUIRE(((word >> 44) & 1) == 1);
    for (const u32 bit : {0U, 8U, 13U, 18U, 22U, 27U}) {
        CAPTURE(bit);
        auto invalid = convert;
        invalid.words[2] ^= 1U << bit;
        const auto modified = LowerToMaxwell(std::array{invalid, Exit}, true);
        if (bit == 8) {
            REQUIRE(modified);
            REQUIRE(modified.instructions.size() == 2);
            CHECK(modified.instructions.front() ==
                  (0x5cb0000000070002ULL | (1ULL << 8) | (2ULL << 10) |
                   (1ULL << 12) | (2ULL << 20) | (3ULL << 39) | (1ULL << 44)));
            CHECK(modified.instructions.back() == 0xe30000000007000fULL);
            invalid.words[2] |= 1;
            REQUIRE_FALSE(LowerToMaxwell(std::array{invalid, Exit}, true));
        } else {
            REQUIRE_FALSE(modified);
        }
    }
    // Bit 8 (72) signs it: F2I.S16.TRUNC lowers too.
    auto signed_convert = convert;
    signed_convert.words[2] |= 1U << 8;
    const auto signed_lowered = LowerToMaxwell(std::array{signed_convert, Exit}, true);
    REQUIRE(signed_lowered);
    REQUIRE(std::ranges::any_of(signed_lowered.instructions, [](u64 value) {
        return (value >> 52) == 0x5cb && ((value >> 8) & 3) == 1 && ((value >> 12) & 1) == 1;
    }));
}

namespace {
using Shader::Ampere::TessellationStage;

// Mario Tennis Fever's court grass: a control program (passes its control point through, writes
// the tessellation levels and a generic patch it reads back) and its evaluation program.
constexpr Instruction InvocationId{{0x00007919, 0, 0x1100, 0x000e2800}};
constexpr Instruction InvocationInfo{{0x00017919, 0, 0x1d00, 0x000e6200}};
constexpr Instruction StageBufferRead{{0x06017923, 0, 0, 0x000e2e00}};
constexpr Instruction LoadPosition{{0xff007321, 0x7001, 0xc00, 0x000e2800}};
constexpr Instruction StorePosition{{0xff007322, 0x7000, 0xcff, 0x001fe800}};
constexpr Instruction StorePatch{{0xff007322, 0x1803, 0x10ff, 0x002fe800}};
constexpr Instruction LoadOwnPatch{{0xff057321, 0x18ff, 0x9000, 0x000e6200}};
constexpr Instruction StoreLevel{{0xff007322, 0x0004, 0x10ff, 0x000fe800}};
constexpr Instruction LoadPatch{{0xff0c7321, 0x18ff, 0x1000, 0x000e6200}};
constexpr Instruction LaneId{{0x00007919, 0, 0, 0x000f6800}};
constexpr Instruction LoadTessCoord{{0xff007321, 0x2f000, 0x8400, 0x020e2200}};

u64 LoweredAt(const Shader::Ampere::LoweredProgram& lowered, size_t native) {
    return lowered.instructions.at(lowered.source_instruction_offsets.at(native));
}
} // namespace

TEST_CASE("Ampere tessellation control programs keep control points and patches",
          "[video_core][ampere][tessellation]") {
    const std::array control{InvocationId, InvocationInfo, StageBufferRead, LoadPosition, StorePosition,
                             StorePatch, LoadOwnPatch, StoreLevel, Exit};
    REQUIRE_FALSE(LowerToMaxwell(control));
    REQUIRE_FALSE(LowerToMaxwell(control, false, false, TessellationStage::Evaluation));
    const auto lowered = LowerToMaxwell(control, false, false, TessellationStage::Control);
    INFO(lowered.error);
    REQUIRE(lowered);
    REQUIRE(LoweredAt(lowered, 0) == 0xf0c8000001170000ULL);                 // S2R SR_INVOCATION_ID
    REQUIRE(LoweredAt(lowered, 2) == 0x5c98078000670001ULL);                 // ISBERD: MOV R1, R6
    const u64 load = LoweredAt(lowered, 3), store = LoweredAt(lowered, 4);
    REQUIRE(load >> 52 == 0xefd);
    REQUIRE(((load >> 31) & 1) == 0);
    REQUIRE(((load >> 39) & 0xff) == 1); // vertex handle R1
    REQUIRE(store >> 52 == 0xeff);
    REQUIRE(((store >> 31) & 1) == 0);
    for (const size_t index : {5U, 6U, 7U}) {
        const u64 patch = LoweredAt(lowered, index);
        REQUIRE(((patch >> 31) & 1) == 1);
        REQUIRE(((patch >> 32) & 1) == 0);
        REQUIRE(((patch >> 20) & 0x3ff) == (index == 7 ? 0U : 0x18U));
    }
    REQUIRE(LoweredAt(lowered, 6) >> 52 == 0xefd); // the patch read back is an ALD.P
}

TEST_CASE("Ampere tessellation evaluation programs read patches and their coordinate",
          "[video_core][ampere][tessellation]") {
    const std::array evaluation{InvocationInfo, LoadPatch, LaneId, LoadTessCoord, StorePosition, Exit};
    REQUIRE_FALSE(LowerToMaxwell(evaluation));
    REQUIRE_FALSE(LowerToMaxwell(evaluation, false, false, TessellationStage::Control));
    const auto lowered = LowerToMaxwell(evaluation, false, false, TessellationStage::Evaluation);
    INFO(lowered.error);
    REQUIRE(lowered);
    const u64 patch = LoweredAt(lowered, 1), coord = LoweredAt(lowered, 3);
    REQUIRE(((patch >> 31) & 1) == 1);
    REQUIRE(((patch >> 20) & 0x3ff) == 0x18);
    REQUIRE((coord & 0xff) == 0);
    REQUIRE(((coord >> 20) & 0x3ff) == 0x2f0);
    REQUIRE(((coord >> 31) & 3) == 0);
    REQUIRE(((coord >> 39) & 0xff) == 255);
    REQUIRE(((coord >> 47) & 3) == 1); // U and V
}

TEST_CASE("Ampere tessellation rejects attribute accesses the frontend cannot express",
          "[video_core][ampere][tessellation]") {
    auto output_vertex = LoadPosition;
    output_vertex.words[2] |= 0x8000; // ALD.O of a control point
    auto other_vertex = StorePosition;
    other_vertex.words[2] = 0xc01; // AST to vertex R1 instead of the own control point
    auto indexed = LoadPosition;
    indexed.words[2] |= 0x2000; // .PHYS
    auto level = LoadPatch;
    level.words[1] = 0x00ff; // ALD.P a[0x00]: a tessellation level
    for (const auto& code : {std::array{output_vertex, Exit}, std::array{other_vertex, Exit},
                             std::array{indexed, Exit}}) {
        REQUIRE_FALSE(LowerToMaxwell(code, false, false, TessellationStage::Control));
    }
    REQUIRE_FALSE(LowerToMaxwell(std::array{level, Exit}, false, false, TessellationStage::Evaluation));
    // The coordinate is indexed by the lane id; without one the index is unknown.
    REQUIRE_FALSE(LowerToMaxwell(std::array{LoadTessCoord, Exit}, false, false,
                                 TessellationStage::Evaluation));
}

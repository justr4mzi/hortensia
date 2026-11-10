// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>

#include "shader_recompiler/frontend/ampere/lower_to_maxwell.h"

namespace {
using Shader::Ampere::Instruction;
using Shader::Ampere::LowerToMaxwell;
constexpr Instruction Exit{{0x794d, 0, 0x03800000, 0}};
constexpr Instruction Captured{{0xff077a19, 0x01000300, 0x00010e04, 0x000fe200}};

// Interpret only the actual Maxwell instructions emitted for this form. The reference below
// instead evaluates the original 64-bit concatenated funnel shift from the documented semantics.
struct Machine {
    std::array<u32, 256> r{};
    std::array<bool, 7> p{};
    std::array<std::array<u32, 64>, 18> cb{};
    void Run(std::span<const u64> code) {
        for (u64 w : code) {
            const auto field = [w](u32 pos, u32 width) {
                return u32((w >> pos) & ((u64{1} << width) - 1));
            };
            const u32 guard = field(16, 3);
            if (!((guard == 7 || p[guard]) != bool(field(19, 1))))
                continue;
            const u32 dst = field(0, 8);
            if ((w & 0xfff8000000000000ULL) == 0x4c98000000000000ULL) {
                REQUIRE(field(39, 4) == 15);
                REQUIRE(field(34, 5) < cb.size());
                REQUIRE(field(20, 14) < cb[0].size());
                if (dst != 255)
                    r[dst] = cb[field(34, 5)][field(20, 14)];
            } else if ((w >> 48) == 0x5c48) {
                const u32 a = field(8, 8) == 255 ? 0 : r[field(8, 8)];
                const u32 b = field(20, 8) == 255 ? 0 : r[field(20, 8)];
                REQUIRE(field(39, 1) == 1);
                if (dst != 255)
                    r[dst] = a << (b & 31);
            } else {
                REQUIRE((w >> 56) == 0xe3); // EXIT
                break;
            }
        }
    }
};

TEST_CASE("Ampere captured constant-buffer high shift executes wrapped funnel semantics",
          "[video_core][ampere]") {
    for (u32 guard : {0U, 1U, 7U, 8U, 9U, 14U}) {
        for (u32 dest : {4U, 7U, 255U}) {
            for (u32 count : {0U, 1U, 31U, 32U, 63U, 255U, 0xffffffffU}) {
                for (u32 value : {0U, 1U, 0x80000001U, 0xffffffffU}) {
                    auto insn = Captured;
                    insn.words[0] = (insn.words[0] & ~0xfffff000U) | (guard << 12) | (dest << 16) |
                                    (255U << 24);
                    const auto lowered = LowerToMaxwell(std::array{insn, Exit});
                    INFO(lowered.error);
                    REQUIRE(lowered);
                    Machine m;
                    for (u32 i = 0; i < m.r.size(); ++i)
                        m.r[i] = 0x12340000U + i;
                    for (u32 bank = 0; bank < m.cb.size(); ++bank)
                        for (u32 offset = 0; offset < m.cb[bank].size(); ++offset)
                            m.cb[bank][offset] = 0x87650000U + bank * 64 + offset;
                    m.r[4] = value;
                    m.p[0] = false;
                    m.p[1] = true;
                    m.p[6] = true;
                    m.cb[4][3] = count;
                    const auto original = m;
                    m.Run(lowered.instructions);
                    const bool active =
                        ((guard & 7) == 7 || original.p[guard & 7]) != bool(guard & 8);
                    const u64 pair = u64{value} << 32;
                    const u32 expected = static_cast<u32>((pair << (count & 31)) >> 32);
                    for (u32 i = 0; i < 246; ++i)
                        REQUIRE(m.r[i] == (active && i == dest ? expected : original.r[i]));
                    REQUIRE(m.p == original.p);
                    REQUIRE(m.cb == original.cb);
                }
            }
        }
    }
}

TEST_CASE("Ampere captured constant-buffer high shift rejects unsupported encodings",
          "[video_core][ampere]") {
    for (auto [word, bit] : std::array{std::pair{0U, 24U}, std::pair{2U, 8U}, std::pair{2U, 9U},
                                       std::pair{2U, 27U}, std::pair{1U, 30U}, std::pair{1U, 31U},
                                       std::pair{3U, 0U}, std::pair{3U, 30U}}) {
        auto insn = Captured;
        insn.words[word] ^= 1U << bit;
        REQUIRE_FALSE(LowerToMaxwell(std::array{insn, Exit}));
    }
    for (u32 reg : {246U, 251U, 254U}) {
        auto insn = Captured;
        insn.words[2] = (insn.words[2] & ~255U) | reg;
        REQUIRE_FALSE(LowerToMaxwell(std::array{insn, Exit}));
        insn = Captured;
        insn.words[0] = (insn.words[0] & ~0xff0000U) | (reg << 16);
        REQUIRE_FALSE(LowerToMaxwell(std::array{insn, Exit}));
    }
    auto invalid_bank = Captured;
    invalid_bank.words[1] = (invalid_bank.words[1] & ~(31U << 22)) | (18U << 22);
    REQUIRE_FALSE(LowerToMaxwell(std::array{invalid_bank, Exit}));
    auto unaligned = Captured;
    unaligned.words[1] |= 1U << 6;
    REQUIRE_FALSE(LowerToMaxwell(std::array{unaligned, Exit}));
}
} // namespace

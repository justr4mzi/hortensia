// SPDX-License-Identifier: GPL-3.0-or-later
#include <array>
#include <unordered_map>
#include <catch2/catch_test_macros.hpp>
#include "shader_recompiler/environment.h"
#include "shader_recompiler/frontend/ampere/lower_to_maxwell.h"
#include "shader_recompiler/frontend/ir/basic_block.h"
#include "shader_recompiler/frontend/maxwell/translate/impl/impl.h"
namespace {
using namespace Shader::Ampere;
constexpr Instruction Exit{{0x794d, 0, 0x03800000, 0}};
constexpr Instruction Cases[] = {
    {{0x89937a10, 0x00008500, 0x027e6480, 0x000fe400}},
    {{0x59637a10, 0x00008500, 0x027e6454, 0x040fe400}},
};
TEST_CASE("Yoshi CBuf IADD3.X consumes two predicate carries", "[video_core][yoshi_cbuf_carry]") {
    for (auto word : Cases) {
        const auto result = LowerToMaxwell(std::array{word, Exit});
        INFO(result.error);
        REQUIRE(result);
    }
}

using namespace Shader;
class CarryEnvironment final : public Environment {
public:
    u64 ReadInstruction(u32) override {
        return 0;
    }
    u32 ReadCbufValue(u32, u32) override {
        return 0;
    }
    TextureType ReadTextureType(u32) override {
        return TextureType::Color2D;
    }
    TexturePixelFormat ReadTexturePixelFormat(u32) override {
        return TexturePixelFormat::A8B8G8R8_UNORM;
    }
    bool IsTexturePixelFormatInteger(u32) override {
        return false;
    }
    u32 ReadViewportTransformState() override {
        return 0;
    }
    u32 TextureBoundBuffer() const override {
        return 0;
    }
    u32 LocalMemorySize() const override {
        return 0;
    }
    u32 SharedMemorySize() const override {
        return 0;
    }
    std::array<u32, 3> WorkgroupSize() const override {
        return {1, 1, 1};
    }
    bool HasHLEMacroState() const override {
        return false;
    }
    std::optional<ReplaceConstant> GetReplaceConstBuffer(u32, u32) override {
        return {};
    }
    void Dump(u64, u64) override {}
};
void RunCarryIR(const IR::Block& block, std::array<u32, 256>& regs,
                const std::array<bool, 7>& preds, u32 cbuf, u32 expected_bank = 0,
                u32 expected_offset = 0x214) {
    std::unordered_map<const IR::Inst*, u32> values;
    auto arg = [&](IR::Value v) -> u32 {
        if (v.IsImmediate())
            return v.Type() == IR::Type::U1 ? u32{v.U1()} : v.U32();
        return values.at(v.Inst());
    };
    for (const IR::Inst& inst : block) {
        using O = IR::Opcode;
        u32 result{};
        switch (inst.GetOpcode()) {
        case O::GetRegister:
            result = regs[static_cast<size_t>(inst.Arg(0).Reg())];
            break;
        case O::SetRegister:
            regs[static_cast<size_t>(inst.Arg(0).Reg())] = arg(inst.Arg(1));
            continue;
        case O::GetPred:
            result = preds[static_cast<size_t>(inst.Arg(0).Pred())];
            break;
        case O::GetCbufU32:
            REQUIRE(arg(inst.Arg(0)) == expected_bank);
            REQUIRE(arg(inst.Arg(1)) == expected_offset);
            result = cbuf;
            break;
        case O::IAdd32:
            result = arg(inst.Arg(0)) + arg(inst.Arg(1));
            break;
        case O::SelectU32:
            result = arg(inst.Arg(0)) ? arg(inst.Arg(1)) : arg(inst.Arg(2));
            break;
        default:
            FAIL("Unexpected IR opcode " << static_cast<int>(inst.GetOpcode()));
        }
        values.emplace(&inst, result);
    }
}
TEST_CASE("Yoshi CBuf two-carry words execute through Maxwell IR", "[video_core][yoshi_cbuf_ir]") {
    CarryEnvironment env;
    for (const auto original : Cases)
        for (const u32 alias : {0U, 1U, 2U}) {
            auto native = original;
            const u32 a = (native.words[0] >> 24) & 255, c = native.words[2] & 255;
            const u32 dest = alias == 1 ? a : alias == 2 ? c : (native.words[0] >> 16) & 255;
            native.words[0] = (native.words[0] & ~0xff0000U) | (dest << 16);
            for (const u32 guard : {0U, 8U, 7U}) {
                native.words[0] = (native.words[0] & ~0xf000U) | (guard << 12);
                const auto lowered = LowerToMaxwell(std::array{native, Exit});
                INFO(lowered.error);
                REQUIRE(lowered);
                for (const u32 av : {0U, 1U, 0xffffffffU, 0x80000000U})
                    for (const u32 cv : {0U, 1U, 0xffffffffU})
                        for (const u32 cb : {0U, 1U, 0xffffffffU})
                            for (const bool p4 : {false, true})
                                for (const bool p3 : {false, true})
                                    for (const bool p0 : {false, true}) {
                                        std::array<u32, 256> regs{};
                                        regs[a] = av;
                                        regs[c] = cv;
                                        if (dest != a && dest != c)
                                            regs[dest] = 0x3456789a;
                                        const u32 old = regs[dest], aa = regs[a], cc = regs[c];
                                        std::array<bool, 7> preds{};
                                        preds[0] = p0;
                                        preds[4] = p4;
                                        preds[3] = p3;
                                        const auto saved = preds;
                                        const bool active = guard == 7 || (guard == 0 ? p0 : !p0);
                                        bool saw_cbuf = false;
                                        for (const u64 insn : lowered.instructions) {
                                            const u64 op = insn >> 48;
                                            if (op == 0xe300)
                                                break;
                                            const u32 condition = (insn >> 16) & 15;
                                            REQUIRE(condition == guard);
                                            if (!active)
                                                continue;
                                            ObjectPool<IR::Inst> pool;
                                            IR::Block block{pool};
                                            Maxwell::TranslatorVisitor v{env, block};
                                            if (op == 0x4c98) {
                                                v.MOV_cbuf(insn);
                                                saw_cbuf = true;
                                            } else if (op == 0x5c10)
                                                v.IADD_reg(insn);
                                            else if (op == 0x0100)
                                                v.MOV32I(insn);
                                            else if (op == 0x5ca0)
                                                v.SEL_reg(insn);
                                            else
                                                FAIL("Unexpected lowered word " << std::hex
                                                                                << insn);
                                            RunCarryIR(block, regs, preds, cb);
                                        }
                                        const u32 expected =
                                            static_cast<u32>(u64{aa} + cb + cc + u32{p4} + u32{p3});
                                        CHECK(regs[dest] == (active ? expected : old));
                                        CHECK(preds == saved);
                                        CHECK(saw_cbuf == active);
                                    }
            }
        }
}
TEST_CASE("Yoshi CBuf carry supports predicate inversion and true predicate",
          "[video_core][yoshi_cbuf_ir]") {
    CarryEnvironment env;
    for (const u32 first : {3U, 4U, 7U})
        for (const u32 second : {3U, 4U, 7U})
            for (const bool neg1 : {false, true})
                for (const bool neg2 : {false, true})
                    for (const bool p4 : {false, true})
                        for (const bool p3 : {false, true})
                            for (const u32 cb : {0U, 1U, 0xffffffffU}) {
                                auto x = Cases[0];
                                x.words[1] = (x.words[1] & ~((31U << 22) | (0xffffU << 6))) |
                                             (3U << 22) | (0x1f0U << 6);
                                x.words[2] = (x.words[2] & ~((7U << 23) | (1U << 26) | (7U << 13) |
                                                             (1U << 16))) |
                                             (first << 23) | (u32{neg1} << 26) | (second << 13) |
                                             (u32{neg2} << 16);
                                const auto lowered = LowerToMaxwell(std::array{x, Exit});
                                INFO(lowered.error);
                                REQUIRE(lowered);
                                std::array<u32, 256> regs{};
                                regs[137] = 0xffffffffU;
                                regs[128] = 2;
                                std::array<bool, 7> preds{};
                                preds[4] = p4;
                                preds[3] = p3;
                                const auto saved = preds;
                                for (u64 insn : lowered.instructions) {
                                    const u64 op = insn >> 48;
                                    if (op == 0xe300)
                                        break;
                                    ObjectPool<IR::Inst> pool;
                                    IR::Block block{pool};
                                    Maxwell::TranslatorVisitor v{env, block};
                                    if (op == 0x4c98)
                                        v.MOV_cbuf(insn);
                                    else if (op == 0x5c10)
                                        v.IADD_reg(insn);
                                    else if (op == 0x0100)
                                        v.MOV32I(insn);
                                    else if (op == 0x5ca0)
                                        v.SEL_reg(insn);
                                    else
                                        FAIL("unexpected word");
                                    RunCarryIR(block, regs, preds, cb, 3, 0x1f0);
                                }
                                const bool c1 = (first == 7 ? true : preds[first]) != neg1;
                                const bool c2 = (second == 7 ? true : preds[second]) != neg2;
                                CHECK(regs[147] == static_cast<u32>(u64{0xffffffffU} + cb + 2 +
                                                                    u32{c1} + u32{c2}));
                                CHECK(preds == saved);
                            }
}
TEST_CASE("Yoshi CBuf two-carry rejects unsupported address and modifiers",
          "[video_core][yoshi_cbuf_ir]") {
    for (const auto original : Cases) {
        for (const auto [word, bit] :
             std::array{std::pair{1U, 6U}, std::pair{1U, 30U}, std::pair{1U, 31U},
                        std::pair{2U, 11U}, std::pair{2U, 27U}}) {
            auto x = original;
            x.words[word] ^= 1U << bit;
            CHECK_FALSE(LowerToMaxwell(std::array{x, Exit}));
        }
        for (const u32 bit : {17U, 20U}) {
            auto output = original;
            output.words[2] ^= 1U << bit;
            CHECK_FALSE(LowerToMaxwell(std::array{output, Exit}));
        }
        auto uniform = original;
        uniform.words[0] = (uniform.words[0] & ~0xfffU) | 0x290U;
        CHECK_FALSE(LowerToMaxwell(std::array{uniform, Exit}));
        auto x = original;
        x.words[1] = (x.words[1] & ~(31U << 22)) | (18U << 22);
        CHECK_FALSE(LowerToMaxwell(std::array{x, Exit}));
    }
}

} // namespace

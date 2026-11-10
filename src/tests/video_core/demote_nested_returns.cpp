// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <limits>
#include <catch2/catch_test_macros.hpp>

#include "shader_recompiler/environment.h"
#include "shader_recompiler/frontend/ir/basic_block.h"
#include "shader_recompiler/frontend/maxwell/control_flow.h"
#include "shader_recompiler/frontend/maxwell/structured_control_flow.h"
#include "shader_recompiler/host_translate_info.h"
#include "shader_recompiler/object_pool.h"

namespace {
namespace IR = Shader::IR;

class FragmentEnvironment final : public Shader::Environment {
public:
    explicit FragmentEnvironment(bool nested_return)
        : code{0xe33000000001000fULL,                 // @P1 KIL
               0xe24000000108000fULL,                 // @!P0 BRA to final EXIT
               nested_return ? 0xe30000000001000fULL  // @P1 EXIT
                             : 0xe24000000007000fULL, // BRA to final EXIT
               0xe30000000007000fULL} {               // EXIT
        stage = Shader::Stage::Fragment;
    }

    u64 ReadInstruction(u32 address) override {
        for (size_t i = 0; i < code.size(); ++i) {
            if (address == 0x50 + i * 8 + ((i + 1) / 3) * 8)
                return code[i];
        }
        FAIL("Read outside fragment fixture");
        return 0;
    }
    u32 ReadCbufValue(u32, u32) override {
        return 0;
    }
    Shader::TextureType ReadTextureType(u32) override {
        return Shader::TextureType::Color2D;
    }
    Shader::TexturePixelFormat ReadTexturePixelFormat(u32) override {
        return Shader::TexturePixelFormat::A8B8G8R8_UNORM;
    }
    bool IsTexturePixelFormatInteger(u32) override {
        return false;
    }
    u32 ReadViewportTransformState() override {
        return 1;
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
    std::optional<Shader::ReplaceConstant> GetReplaceConstBuffer(u32, u32) override {
        return std::nullopt;
    }
    void Dump(u64, u64) override {}

private:
    std::array<u64, 4> code;
};

struct Order {
    size_t demote{(std::numeric_limits<size_t>::max)()};
    size_t nested_epilogue{(std::numeric_limits<size_t>::max)()};
    size_t epilogues{};
};

Order BuildOrder(bool nested_return, bool reorder) {
    FragmentEnvironment env{nested_return};
    Shader::ObjectPool<IR::Inst> inst_pool;
    Shader::ObjectPool<IR::Block> block_pool;
    Shader::ObjectPool<Shader::Maxwell::Flow::Block> flow_pool;
    Shader::Maxwell::Flow::CFG cfg{env, flow_pool, 0x50};
    Shader::HostTranslateInfo host{};
    host.needs_demote_reorder = reorder;
    const auto nodes = Shader::Maxwell::BuildASL(inst_pool, block_pool, env, cfg, host);
    Order order;
    u32 depth{};
    for (size_t index = 0; index < nodes.size(); ++index) {
        const auto& node = nodes[index];
        using Type = IR::AbstractSyntaxNode::Type;
        if (node.type == Type::If)
            ++depth;
        if (node.type == Type::EndIf)
            --depth;
        if (node.type != Type::Block)
            continue;
        for (const auto& inst : node.data.block->Instructions()) {
            if (inst.GetOpcode() == IR::Opcode::DemoteToHelperInvocation)
                order.demote = index;
            if (inst.GetOpcode() == IR::Opcode::Epilogue) {
                ++order.epilogues;
                if (depth > 1)
                    order.nested_epilogue = index;
            }
        }
    }
    return order;
}
} // namespace

TEST_CASE("AMD demote reordering preserves a nested early return", "[video_core][shader][demote]") {
    const auto unchanged = BuildOrder(true, false);
    const auto reordered = BuildOrder(true, true);
    REQUIRE(unchanged.epilogues == size_t{2});
    REQUIRE(reordered.epilogues == size_t{2});
    REQUIRE(unchanged.nested_epilogue != (std::numeric_limits<size_t>::max)());
    REQUIRE(reordered.nested_epilogue != (std::numeric_limits<size_t>::max)());
    CHECK(unchanged.demote < unchanged.nested_epilogue);
    CHECK(reordered.demote < reordered.nested_epilogue);
}

TEST_CASE("AMD demote reordering still combines a single-return shader",
          "[video_core][shader][demote]") {
    const auto unchanged = BuildOrder(false, false);
    const auto reordered = BuildOrder(false, true);
    REQUIRE(unchanged.epilogues == size_t{1});
    REQUIRE(reordered.epilogues == size_t{1});
    CHECK(reordered.demote > unchanged.demote);
}

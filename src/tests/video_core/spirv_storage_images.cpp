// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <vector>
#include <catch2/catch_test_macros.hpp>

#include "shader_recompiler/backend/spirv/emit_spirv.h"
#include "shader_recompiler/environment.h"
#include "shader_recompiler/frontend/ir/ir_emitter.h"
#include "shader_recompiler/host_translate_info.h"
#include "shader_recompiler/ir_opt/passes.h"

TEST_CASE("SPIR-V storage image formats determine texel scalar types", "[video_core][spirv]") {
    using namespace Shader;
    for (bool buffer : {false, true}) {
        for (bool integer_descriptor : {false, true}) {
            for (const ImageFormat format :
                 {ImageFormat::Typeless, ImageFormat::R8_UINT, ImageFormat::R8_SINT,
                  ImageFormat::R16_UINT, ImageFormat::R16_SINT, ImageFormat::R32_UINT,
                  ImageFormat::R32G32_UINT, ImageFormat::R32G32B32A32_UINT}) {
                ObjectPool<IR::Inst> pool;
                IR::Block block{pool};
                IR::Program program;
                program.stage = Stage::Compute;
                program.workgroup_size = {1, 1, 1};
                program.blocks = {&block};
                program.post_order_blocks = {&block};
                program.syntax_list = {{{.block = &block}, IR::AbstractSyntaxNode::Type::Block},
                                       {{}, IR::AbstractSyntaxNode::Type::Return}};
                const auto type = buffer ? TextureType::Buffer : TextureType::Color2D;
                if (buffer) {
                    program.info.image_buffer_descriptors.push_back(
                        {format, true, true, integer_descriptor, 0, 0, 1, 0});
                } else {
                    program.info.image_descriptors.push_back(
                        {type, format, true, true, integer_descriptor, 0, 0, 1, 0});
                }
                program.info.uses_image_buffers = buffer;
                program.info.uses_typeless_image_reads = format == ImageFormat::Typeless;
                program.info.uses_typeless_image_writes = format == ImageFormat::Typeless;
                IR::IREmitter ir{block};
                IR::TextureInstInfo info{};
                info.type.Assign(type);
                info.image_format.Assign(format);
                const IR::Value coords = buffer ? IR::Value{ir.Imm32(0)}
                                                : ir.CompositeConstruct(ir.Imm32(0), ir.Imm32(0));
                const auto color = ir.ImageRead(ir.Imm32(0), coords, info);
                ir.ImageWrite(ir.Imm32(0), coords, color, info);
                for (auto& instruction : block.Instructions()) {
                    if (instruction.GetOpcode() == IR::Opcode::BoundImageRead) {
                        instruction.ReplaceOpcode(IR::Opcode::ImageRead);
                    } else if (instruction.GetOpcode() == IR::Opcode::BoundImageWrite) {
                        instruction.ReplaceOpcode(IR::Opcode::ImageWrite);
                    }
                }
                Profile profile;
                profile.supported_spirv = 0x00010500;
                profile.support_typeless_image_loads = true;
                const auto code = Backend::SPIRV::EmitSPIRV(profile, program);
                std::map<u32, std::vector<u32>> definitions;
                std::map<u32, u32> value_types;
                u32 sampled_type{}, image_result_type{}, texel_type{};
                for (size_t pos = 5; pos < code.size();) {
                    const u32 size = code[pos] >> 16;
                    REQUIRE(size != 0);
                    const u32 op = code[pos] & 65535;
                    if (op == 21 || op == 22 || op == 23) {
                        definitions[code[pos + 1]] = {code.begin() + pos,
                                                      code.begin() + pos + size};
                    }
                    if (op == 98 || op == 124)
                        value_types[code[pos + 2]] = code[pos + 1];
                    if (op == 25)
                        sampled_type = code[pos + 2]; // OpTypeImage
                    if (op == 98)
                        image_result_type = code[pos + 1]; // OpImageRead
                    if (op == 99)
                        texel_type = code[pos + 3]; // OpImageWrite color ID
                    pos += size;
                }
                const auto scalar = definitions.at(sampled_type);
                const bool signed_format =
                    format == ImageFormat::R8_SINT || format == ImageFormat::R16_SINT;
                const bool floating = format == ImageFormat::Typeless && !integer_descriptor;
                CHECK((scalar[0] & 65535) == (floating ? 22U : 21U));
                if (!floating)
                    CHECK(scalar[3] == (signed_format ? 1U : 0U));
                CHECK(definitions.at(image_result_type)[2] == sampled_type);
                CHECK(value_types.at(texel_type) == image_result_type);
                if (const char* dir = std::getenv("EDEN_IMAGE_TEST_OUTPUT")) {
                    std::filesystem::create_directories(dir);
                    const auto name = std::to_string(buffer) + "-" +
                                      std::to_string(integer_descriptor) + "-" +
                                      std::to_string(static_cast<u32>(format));
                    std::ofstream file{std::filesystem::path{dir} / (name + ".spv"),
                                       std::ios::binary};
                    file.write(reinterpret_cast<const char*>(code.data()),
                               code.size() * sizeof(u32));
                    REQUIRE(file.good());
                }
            }
        }
    }
}

namespace {
class PackedSurfaceEnvironment final : public Shader::Environment {
public:
    Shader::TexturePixelFormat format{Shader::TexturePixelFormat::R16_FLOAT};
    Shader::TextureType type{Shader::TextureType::Color2D};
    u64 ReadInstruction(u32) override {
        return 0;
    }
    u32 ReadCbufValue(u32, u32) override {
        return 0x1234;
    }
    Shader::TextureType ReadTextureType(u32) override {
        return type;
    }
    Shader::TexturePixelFormat ReadTexturePixelFormat(u32) override {
        return format;
    }
    bool IsTexturePixelFormatInteger(u32) override {
        return false;
    }
    u32 ReadViewportTransformState() override {
        return 1;
    }
    u32 TextureBoundBuffer() const override {
        return 2;
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
};
u32 EvaluateU32(Shader::IR::Value value) {
    value = value.Resolve();
    if (value.IsImmediate()) {
        return value.U32();
    }
    const auto& inst = *value.Inst();
    switch (inst.GetOpcode()) {
    case Shader::IR::Opcode::ShiftLeftLogical32:
        return EvaluateU32(inst.Arg(0)) << EvaluateU32(inst.Arg(1));
    case Shader::IR::Opcode::ShiftRightLogical32:
        return EvaluateU32(inst.Arg(0)) >> EvaluateU32(inst.Arg(1));
    case Shader::IR::Opcode::IAdd32:
        return EvaluateU32(inst.Arg(0)) + EvaluateU32(inst.Arg(1));
    default:
        FAIL("Unexpected scalar expression in packed surface write");
        return 0;
    }
}
} // namespace

TEST_CASE("Raw surface words split across compatible single-channel 16-bit texels",
          "[video_core][texture]") {
    using namespace Shader;
    for (const auto format : {TexturePixelFormat::R16_FLOAT, TexturePixelFormat::R16_UINT,
                              TexturePixelFormat::R16_SINT, TexturePixelFormat::R16_UNORM,
                              TexturePixelFormat::R16_SNORM, TexturePixelFormat::R32_FLOAT}) {
        for (const auto type : {TextureType::Buffer, TextureType::Color1D, TextureType::Color2D,
                                TextureType::ColorArray2D}) {
            for (const bool raw : {false, true}) {
                PackedSurfaceEnvironment env;
                env.format = format;
                env.type = type;
                const bool scalar_coords =
                    type == TextureType::Buffer || type == TextureType::Color1D;
                ObjectPool<IR::Inst> pool;
                IR::Block block{pool};
                IR::Program program;
                program.blocks = {&block};
                program.post_order_blocks = {&block};
                IR::IREmitter ir{block};
                IR::TextureInstInfo flags{};
                flags.type.Assign(type);
                flags.image_format.Assign(raw ? ImageFormat::R32_UINT : ImageFormat::Typeless);
                const IR::Value coords =
                    scalar_coords ? IR::Value{ir.Imm32(7)}
                    : type == TextureType::Color2D
                        ? ir.CompositeConstruct(ir.Imm32(7), ir.Imm32(9))
                        : ir.CompositeConstruct(ir.Imm32(7), ir.Imm32(9), ir.Imm32(3));
                const auto color = ir.CompositeConstruct(ir.Imm32(0xbadc1234U), ir.Imm32(0),
                                                         ir.Imm32(0), ir.Imm32(0));
                ir.ImageWrite(ir.Imm32(0x520U), coords, color, flags);
                HostTranslateInfo host;
                host.ApplyDescriptorLimitPolicy();
                Optimization::TexturePass(env, program, host);
                const bool split = raw && format != TexturePixelFormat::R32_FLOAT;
                REQUIRE((type == TextureType::Buffer ? program.info.image_buffer_descriptors.size()
                                                     : program.info.image_descriptors.size()) == 1);
                const auto image_format = type == TextureType::Buffer
                                              ? program.info.image_buffer_descriptors[0].format
                                              : program.info.image_descriptors[0].format;
                CHECK(image_format == (split ? ImageFormat::R16_UINT : flags.image_format.Value()));
                std::vector<std::array<IR::Value, 4>> observed;
                for (auto& instruction : block.Instructions()) {
                    if (instruction.GetOpcode() != IR::Opcode::ImageWrite)
                        continue;
                    const auto coordinate = instruction.Arg(1);
                    const auto texel = instruction.Arg(2);
                    // Fold composites, then evaluate residual shifts explicitly: the optimizer
                    // intentionally does not constant-fold every integer opcode.
                    observed.push_back(
                        {scalar_coords ? coordinate : ir.CompositeExtract(coordinate, 0),
                         scalar_coords ? IR::Value{ir.Imm32(9)}
                                       : ir.CompositeExtract(coordinate, 1),
                         type == TextureType::ColorArray2D ? ir.CompositeExtract(coordinate, 2)
                                                           : IR::Value{ir.Imm32(3)},
                         ir.CompositeExtract(texel, 0)});
                }
                Optimization::ConstantPropagationPass(env, program);
                REQUIRE(observed.size() == (split ? 2U : 1U));
                for (size_t i = 0; i < observed.size(); ++i) {
                    CHECK(EvaluateU32(observed[i][0]) == (split ? 14U + i : 7U));
                    CHECK(EvaluateU32(observed[i][1]) == 9U);
                    CHECK(EvaluateU32(observed[i][2]) == 3U);
                    CHECK(EvaluateU32(observed[i][3]) ==
                          (split ? (i == 0 ? 0x1234U : 0xbadcU) : 0xbadc1234U));
                }
            }
        }
    }
}

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>
#include "shader_recompiler/frontend/ampere/program_capture.h"

namespace {
using Shader::Ampere::Instruction;
std::vector<Instruction> GeometryProgram() {
    return {{{{0x0000734e, 0, 0, 0}}},          {{{0x00037802, 0x70, 0xf00, 0}}},
            {{{0x00007210, 3, 0x07f1e0ff, 0}}}, {{{0x01017210, 0xff, 0x007fe4ff, 0}}},
            {{{0x00007357, 0, 0, 0}}},          {{{0x0000794d, 0, 0x03800000, 0}}},
            {{{0x00007918, 0, 0, 0}}},          {{{0x00ff7324, 255, 0, 0}}},
            {{{0x00007957, 0, 0, 0}}},          {{{0x0000794d, 0, 0x03800000, 0}}}};
}
} // namespace

TEST_CASE("Native capture retains ATEXIT finalization", "[ampere-geometry]") {
    const auto code = GeometryProgram();
    Shader::Ampere::ProgramCaptureState capture;
    for (u32 i = 0; i < code.size(); ++i) {
        const auto result = capture.Observe(code[i], i);
        REQUIRE(result == (i + 1 == code.size()
                               ? Shader::Ampere::ProgramCaptureState::Result::Complete
                               : Shader::Ampere::ProgramCaptureState::Result::Continue));
    }
}

TEST_CASE("Native geometry finalization needs a geometry stage and a verified exit envelope",
          "[ampere-geometry]") {
    // The single-invocation lowering this program was written for (LEPC materializing
    // code_address, OUT.FINAL as EndPrimitive) was dropped; those options are unused. Outside a
    // geometry stage the static ATEXIT path routes the EXIT into the handler and stops at its OUT.
    // A geometry stage takes the exit envelope, which needs an EXIT right before the handler.
    const auto code = GeometryProgram();
    const auto lowered = Shader::Ampere::LowerToMaxwell(
        code, {.code_address = 0x11fffffff0ULL, .single_invocation_geometry = true});
    REQUIRE_FALSE(lowered);
    REQUIRE(lowered.error_offset == 7 * 16);
    REQUIRE(lowered.error == "native geometry output outside geometry stage");
    const auto geometry = Shader::Ampere::LowerToMaxwell(code, {.geometry_stage = true});
    REQUIRE_FALSE(geometry);
    REQUIRE(geometry.error_offset == 6 * 16);
    REQUIRE(geometry.error == "native exit handler reached by fallthrough");
}

TEST_CASE("Native ATEXIT requires a complete verified epilogue", "[ampere-geometry]") {
    auto code = GeometryProgram();
    code.pop_back();
    REQUIRE_FALSE(Shader::Ampere::LowerToMaxwell(code, {.single_invocation_geometry = true}));
    code = GeometryProgram();
    code[8].words[1] = 1;
    REQUIRE_FALSE(Shader::Ampere::LowerToMaxwell(code, {.single_invocation_geometry = true}));
    code = GeometryProgram();
    code[1].words[1] = 0x71;
    // An unaligned handler is no static ATEXIT registration: outside a geometry stage the prologue
    // lowers as ordinary code and the program ends at its first EXIT. The geometry exit envelope
    // rejects the handler location.
    const auto unaligned = Shader::Ampere::LowerToMaxwell(code, {.single_invocation_geometry = true});
    REQUIRE(unaligned);
    REQUIRE(unaligned.source_size == 6 * 16);
    const auto geometry = Shader::Ampere::LowerToMaxwell(code, {.geometry_stage = true});
    REQUIRE_FALSE(geometry);
    REQUIRE(geometry.error_offset == 0x10);
    REQUIRE(geometry.error == "native exit handler location");
}

TEST_CASE("Native input entry reads lower as moves only in a geometry stage", "[ampere-geometry]") {
    std::vector<Instruction> code{{{0x00007919, 0, 0x1d00, 0}},
                                  {{0x00007816, 0x7770, 255, 0}},
                                  {{0x01007224, 0, 0x078e02ff, 0}},
                                  {{0x00077923, 0, 0, 0}},
                                  {{0x0000794d, 0, 0x03800000, 0}}};
    // The single-invocation proof was dropped (the option is unused): a geometry stage lowers
    // ISBERD as a move of its index, for any input entry; outside one the invocation read rejects.
    const auto lowered = Shader::Ampere::LowerToMaxwell(code, {.geometry_stage = true});
    REQUIRE(lowered);
    REQUIRE(lowered.instructions[lowered.source_instruction_offsets[3]] ==
            (0x5c98000000070000ULL | 7ULL | (15ULL << 39)));
    REQUIRE_FALSE(Shader::Ampere::LowerToMaxwell(code));
    REQUIRE_FALSE(Shader::Ampere::LowerToMaxwell(code, {.single_invocation_geometry = true}));
    auto with_zero_pair = code;
    with_zero_pair.insert(with_zero_pair.begin() + 1, Instruction{{0x00287805, 0, 0x0001ff00, 0}});
    REQUIRE(Shader::Ampere::LowerToMaxwell(with_zero_pair, {.geometry_stage = true}));
    code[1].words[1] = 0x7772;
    REQUIRE(Shader::Ampere::LowerToMaxwell(code, {.geometry_stage = true}));
}

TEST_CASE("Geometry local-memory uniforms retain real addresses", "[ampere-geometry]") {
    const std::array store{Instruction{{0xff007987, 0x4004, 0x08100c08, 0}},
                           Instruction{{0x794d, 0, 0x03800000, 0}}};
    const auto written = Shader::Ampere::LowerToMaxwell(store);
    REQUIRE(written);
    for (u32 i = 0; i < 4; ++i) {
        REQUIRE((written.instructions[i] & 255) == 4 + i);
        REQUIRE(((written.instructions[i] >> 8) & 255) == 224);
        REQUIRE(((written.instructions[i] >> 20) & 0xffffff) == 0x40 + i * 4);
    }
    const std::array load{Instruction{{0xff337983, 0x6c08, 0x08100800, 0}},
                          Instruction{{0x794d, 0, 0x03800000, 0}}};
    const auto read = Shader::Ampere::LowerToMaxwell(load);
    REQUIRE(read);
    REQUIRE((read.instructions.front() & 255) == 51);
    REQUIRE(((read.instructions.front() >> 8) & 255) == 224);
    REQUIRE(((read.instructions.front() >> 20) & 0xffffff) == 0x6c);
}

TEST_CASE("Geometry wide funnel shift retains both input words", "[ampere-geometry]") {
    for (const u32 shift : {0U, 1U, 16U, 31U, 32U}) {
        const std::array code{Instruction{{0x100f7819, shift, 0x0001020f, 0}},
                              Instruction{{0x794d, 0, 0x03800000, 0}}};
        const auto lowered = Shader::Ampere::LowerToMaxwell(code);
        REQUIRE(lowered);
        REQUIRE(static_cast<u32>(lowered.instructions.front() >> 20) == shift);
        const u64 word = lowered.instructions[1];
        REQUIRE(((word >> 8) & 255) == 16);
        REQUIRE(((word >> 39) & 255) == 15);
        REQUIRE(((word >> 37) & 3) == 2);
        REQUIRE(((word >> 20) & 255) == 251);
        for (const u32 low : {0U, 0x12345678U, 0xffffffffU})
            for (const u32 high : {0U, 0x87654321U, 0xffffffffU}) {
                const u32 result = static_cast<u32>(((u64{high} << 32 | low) << shift) >> 32);
                const u32 separate = shift == 0    ? high
                                     : shift == 32 ? low
                                                   : (high << shift) | (low >> (32 - shift));
                REQUIRE(result == separate);
            }
    }
}

TEST_CASE("Geometry half-word local stores preserve adjacent bytes", "[ampere-geometry]") {
    const std::array code{Instruction{{0xff007987, 0x9488, 0x08100408, 0}},
                          Instruction{{0x794d, 0, 0x03800000, 0}}};
    const auto lowered = Shader::Ampere::LowerToMaxwell(code);
    REQUIRE(lowered);
    const u64 word = lowered.instructions.front();
    REQUIRE((word & 255) == 136);
    REQUIRE(((word >> 8) & 255) == 224);
    REQUIRE(((word >> 20) & 0xffffff) == 0x94);
    REQUIRE(((word >> 48) & 7) == 2);
}

TEST_CASE("Geometry indexed vector loads preserve aliased addresses", "[ampere-geometry]") {
    for (u32 address : {26U, 43U}) {
        const std::array code{Instruction{{0x001a7983 | (address << 24), 0xc800, 0x00100a00, 0}},
                              Instruction{{0x794d, 0, 0x03800000, 0}}};
        const auto lowered = Shader::Ampere::LowerToMaxwell(code);
        REQUIRE(lowered);
        const u32 start = address == 26 ? 1 : 0;
        if (start) {
            REQUIRE((lowered.instructions[0] & 255) == 251);
            REQUIRE(((lowered.instructions[0] >> 20) & 255) == address);
        }
        for (u32 i = 0; i < 2; ++i) {
            const auto word = lowered.instructions[start + i];
            REQUIRE((word & 255) == 26 + i);
            REQUIRE(((word >> 8) & 255) == (start ? 251 : address));
            REQUIRE(((word >> 20) & 0xffffff) == 0xc8 + i * 4);
        }
    }
}

#include <algorithm>
#include <array>

#include <catch2/catch_test_macros.hpp>

#include "shader_recompiler/backend/spirv/emit_spirv.h"
#include "shader_recompiler/frontend/ampere/lower_to_maxwell.h"
#include "shader_recompiler/frontend/ir/ir_emitter.h"
#include "shader_recompiler/object_pool.h"

namespace {
using Shader::Ampere::Instruction;
using Shader::Ampere::LowerToMaxwell;
constexpr Instruction EXIT{{0x0000794d, 0, 0x03800000, 0}};

bool Contains(const Shader::Ampere::LoweredProgram& lowered, u64 instruction) {
    return std::find(lowered.instructions.begin(), lowered.instructions.end(), instruction) !=
           lowered.instructions.end();
}

constexpr std::array<Instruction, 16> GEOMETRY_PROGRAM{{
    {{0x0000734e, 0, 0, 0}},
    {{0x00037802, 0xd0, 0xf00, 0}},
    {{0x00007210, 3, 0x07f1e0ff, 0}},
    {{0xff017224, 0xff, 0x000e0401, 0}},
    {{0x00007357, 0, 0, 0}},
    {{0x00007802, 0, 0xf00, 0}},
    {{0x00017802, 0, 0xf00, 0}},
    {{0x00037802, 0, 0xf00, 0}},
    {{0x0100720c, 0xff, 0x03f03070, 0}},
    {{0x00017923, 0, 0, 0}},          // ISBERD R1, [R0]
    {{0x00027324, 0xff, 0x4000, 0}},  // OUT.EMIT R2, R0, RZ
    {{0x02037924, 0, 0x8000, 0}},     // OUT.CUT R3, R2, 0
    EXIT,
    {{0x00ff7324, 0xff, 0, 0}},
    {{0x00007957, 0, 0, 0}},
    EXIT,
}};
} // namespace

TEST_CASE("Ampere geometry program counter uses the native address", "[video_core][ampere]") {
    const Instruction nop{{0x00007918, 0, 0, 0}};
    const Instruction lepc{{0x0014734e, 0, 0, 0}};
    const std::array code{nop, lepc, EXIT};
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(code)));
    REQUIRE_FALSE(LowerToMaxwell(code, true));
    const auto result = LowerToMaxwell(code, {.geometry = true,
                                            .native_code_address = 0x00000012fffffff0ULL});
    REQUIRE(static_cast<bool>(result));
    REQUIRE(result.instructions.size() == 4);
    REQUIRE((result.instructions[1] & 0xff) == 20);
    REQUIRE(((result.instructions[1] >> 20) & 0xffffffffULL) == 0);
    REQUIRE((result.instructions[2] & 0xff) == 21);
    REQUIRE(((result.instructions[2] >> 20) & 0xffffffffULL) == 0x13);

    Instruction unsupported = lepc;
    unsupported.words[1] = 1;
    REQUIRE_FALSE(LowerToMaxwell(std::array{nop, unsupported, EXIT},
                                 {.geometry = true, .native_code_address = 0x00000012fffffff0ULL}));
    unsupported = lepc;
    unsupported.words[0] = (unsupported.words[0] & ~0xff0000U) | (245U << 16);
    REQUIRE_FALSE(LowerToMaxwell(std::array{nop, unsupported, EXIT},
                                 {.geometry = true, .native_code_address = 0x00000012fffffff0ULL}));
}

TEST_CASE("Ampere geometry stage map and exit setup are stage specific", "[video_core][ampere]") {
    const Instruction exit{{0x0000794d, 0, 0x03800000, 0}};
    const Instruction map_read{{0x05127923, 0, 0, 0}}; // ISBERD R18, [R5]
    const Instruction exit_setup{{0x03007357, 0, 0, 0}};
    auto code = GEOMETRY_PROGRAM;
    code[9] = map_read;
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(code)));
    const auto result = LowerToMaxwell(code, true);
    REQUIRE(static_cast<bool>(result));
    const u64 map = result.instructions[result.source_instruction_offsets[9]];
    REQUIRE((map & 0xff) == 18);
    REQUIRE(((map >> 20) & 0xff) == 5);
    REQUIRE_FALSE(LowerToMaxwell(std::array{map_read, exit_setup, EXIT}, true));

    Instruction unsupported = exit_setup;
    unsupported.words[1] = 1;
    REQUIRE_FALSE(LowerToMaxwell(std::array{unsupported, EXIT}, true));
}

TEST_CASE("Ampere F2I supports 16-bit truncation", "[video_core][ampere]") {
    for (const bool is_signed : {false, true}) {
        const Instruction convert{{0x00067305, 9, 0x0021e800U | (is_signed ? 0x100U : 0), 0}};
        const auto result = LowerToMaxwell(std::array{convert, EXIT});
        REQUIRE(static_cast<bool>(result));
        const auto word = std::ranges::find_if(result.instructions, [](u64 value) {
            return (value & 0xfff0000000000000ULL) == 0x5cb0000000000000ULL;
        });
        REQUIRE(word != result.instructions.end());
        REQUIRE(((*word >> 8) & 3) == 1);
        REQUIRE(((*word >> 12) & 1) == static_cast<u64>(is_signed));
        REQUIRE(((*word >> 39) & 3) == 3);
        REQUIRE(((*word >> 44) & 1) == 1);
    }
    const Instruction unsupported{{0x00067305, 9, 0x0021e801, 0}};
    REQUIRE_FALSE(static_cast<bool>(LowerToMaxwell(std::array{unsupported, EXIT})));
}

TEST_CASE("Ampere geometry programs lower their output instructions", "[video_core][ampere]") {
    const auto lowered = LowerToMaxwell(GEOMETRY_PROGRAM, true);
    INFO(lowered.error << " at " << lowered.error_offset);
    REQUIRE(static_cast<bool>(lowered));
    REQUIRE(lowered.source_size == sizeof(GEOMETRY_PROGRAM));
    for (size_t index = 0; index <= Shader::Ampere::GeometryExitSetupInstructions; ++index) {
        REQUIRE(lowered.source_instruction_offsets[index] == 0);
    }
    REQUIRE(Contains(lowered, 0x5c98078000070001ULL)); // MOV R1, R0
    REQUIRE(Contains(lowered, 0xfbe000800ff70002ULL)); // OUT.EMIT R2, R0, RZ
    REQUIRE(Contains(lowered, 0xf6e0010000070203ULL)); // OUT.CUT R3, R2, 0
    REQUIRE(Contains(lowered, 0xfbe000000ff700ffULL));
    REQUIRE(lowered.instructions.back() == 0xe30000000007000fULL);
}

TEST_CASE("Ampere output instructions stay rejected outside geometry programs",
          "[video_core][ampere]") {
    const auto lowered = LowerToMaxwell(GEOMETRY_PROGRAM);
    REQUIRE(!static_cast<bool>(lowered));
    REQUIRE(lowered.error == "native program counter load outside geometry stage");
    for (const auto& output : {GEOMETRY_PROGRAM[10], GEOMETRY_PROGRAM[11]}) {
        const auto rejected = LowerToMaxwell(std::array{output, EXIT});
        REQUIRE_FALSE(rejected);
        REQUIRE(rejected.error == "native geometry output outside geometry stage");
    }
}

TEST_CASE("Ampere geometry rejects an unvalidated program counter load", "[video_core][ampere]") {
    const Instruction lepc{{0x0004734e, 0, 0, 0}};
    const auto truncated = LowerToMaxwell(std::array{lepc, EXIT}, true);
    REQUIRE_FALSE(truncated);
    REQUIRE(truncated.error == "native exit envelope truncated");
    auto invalid = GEOMETRY_PROGRAM;
    invalid[0] = lepc;
    const auto rejected = LowerToMaxwell(invalid, true);
    REQUIRE_FALSE(rejected);
    REQUIRE(rejected.error == "native exit handler installation");
}

TEST_CASE("SPIR-V geometry stream zero needs no geometry streams capability",
          "[video_core][ampere][spirv]") {
    namespace IR = Shader::IR;
    Shader::ObjectPool<IR::Inst> inst_pool;
    IR::Block block{inst_pool};
    IR::IREmitter ir{block};
    ir.EmitVertex(ir.Imm32(0));
    ir.EndPrimitive(ir.Imm32(0));
    IR::Program program;
    program.stage = Shader::Stage::Geometry;
    program.output_topology = Shader::OutputTopology::PointList;
    program.output_vertices = 1;
    program.invocations = 1;
    program.blocks.push_back(&block);
    program.post_order_blocks.push_back(&block);
    IR::AbstractSyntaxNode block_node{};
    block_node.type = IR::AbstractSyntaxNode::Type::Block;
    block_node.data.block = &block;
    IR::AbstractSyntaxNode return_node{};
    return_node.type = IR::AbstractSyntaxNode::Type::Return;
    program.syntax_list = {block_node, return_node};
    Shader::Profile profile{};
    profile.supported_spirv = 0x00010300;
    profile.support_geometry_streams = false;
    const auto code = Shader::Backend::SPIRV::EmitSPIRV(profile, program);
    REQUIRE(code.size() > 5);
    constexpr u32 OpCapability = 17;
    constexpr u32 OpEmitVertex = 218;
    constexpr u32 OpEndPrimitive = 219;
    constexpr u32 OpEmitStreamVertex = 220;
    constexpr u32 OpEndStreamPrimitive = 221;
    constexpr u32 CapabilityGeometry = 2;
    constexpr u32 CapabilityGeometryStreams = 54;
    u32 emits = 0, ends = 0, geometry = 0;
    for (size_t offset = 5; offset < code.size();) {
        const u32 words = code[offset] >> 16;
        const u32 opcode = code[offset] & 0xffff;
        REQUIRE(words > 0);
        REQUIRE(offset + words <= code.size());
        if (opcode == OpCapability) {
            REQUIRE(words == 2);
            REQUIRE(code[offset + 1] != CapabilityGeometryStreams);
            geometry += code[offset + 1] == CapabilityGeometry;
        }
        emits += opcode == OpEmitVertex;
        ends += opcode == OpEndPrimitive;
        REQUIRE(opcode != OpEmitStreamVertex);
        REQUIRE(opcode != OpEndStreamPrimitive);
        offset += words;
    }
    REQUIRE(emits == 1);
    REQUIRE(ends == 1);
    REQUIRE(geometry == 1);
}

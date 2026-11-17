// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <span>
#include <vector>

#include "common/logging.h"
#include "shader_recompiler/backend/spirv/emit_spirv.h"

namespace Vulkan {

/// Whether a shader routes global memory with OpSelect over every bound buffer on NVIDIA (#1848).
/// Compute keeps it: the two Ocarina programs of #619 hang the Linux driver (Xid 109) with
/// candidate-route chains, and NVIDIA compiles other compute programs ~1.6x faster with it, which
/// shortens first-use freezes. Graphics shaders keep the candidate routes of #1807: the select form
/// costs ~10 fps at the Market fountain.
inline bool SelectsGlobalMemoryBuffer(const Shader::IR::Program& program, u64 shader_hash) {
    return program.stage == Shader::Stage::Compute || shader_hash == 0xf98012dc9a4465ffULL ||
           shader_hash == 0xe78efceda31f97a8ULL;
}

inline std::vector<u32> EmitPipelineShader(const Shader::Profile& profile,
                                           const Shader::RuntimeInfo& runtime_info,
                                           Shader::IR::Program& program,
                                           Shader::Backend::Bindings& binding, u64 shader_hash,
                                           std::span<const u32> reference = {}) {
    Shader::Profile candidate_routes;
    const Shader::Profile* used{&profile};
    if (profile.select_global_memory_buffer && !SelectsGlobalMemoryBuffer(program, shader_hash)) {
        candidate_routes = profile;
        candidate_routes.select_global_memory_buffer = false;
        used = &candidate_routes;
    }
    std::vector<u32> code{
        Shader::Backend::SPIRV::EmitSPIRV(*used, runtime_info, program, binding)};
    if (!reference.empty() && !std::ranges::equal(code, reference)) {
        LOG_ERROR(Render_Vulkan,
                  "Nx2Perf async_pipelines: shader {:016x} worker SPIR-V differs; "
                  "using GPU-thread SPIR-V",
                  shader_hash);
        code.assign(reference.begin(), reference.end());
    }
    return code;
}

/// A compute program's SPIR-V, as EmitPipelineShader emits a graphics stage's
inline std::vector<u32> EmitComputeShader(const Shader::Profile& profile,
                                          Shader::IR::Program& program, u64 shader_hash,
                                          std::span<const u32> reference = {}) {
    Shader::Backend::Bindings binding;
    return EmitPipelineShader(profile, {}, program, binding, shader_hash, reference);
}

} // namespace Vulkan

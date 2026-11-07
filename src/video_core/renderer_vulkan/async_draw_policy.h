// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include "common/common_types.h"

namespace Vulkan {

/// Whether a draw whose graphics pipeline is still building may be skipped (asynchronous
/// shaders) instead of waiting for the build.
constexpr bool CanDiscardPendingDraw(bool native, bool depth_enabled, u32 index_count,
                                    u32 vertex_count, bool discard_native = false) {
    if (native && !discard_native) {
        return false;
    }
    return depth_enabled || (index_count > 6 && vertex_count > 6);
}

/// Whether a native pipeline's SPIR-V emission is deferred to a worker: only while fewer than
/// `capacity` emissions are pending.
constexpr bool DefersShaderEmission(bool native, size_t pending, size_t capacity) {
    return native && pending < capacity;
}

} // namespace Vulkan

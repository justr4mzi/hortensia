// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <limits>

namespace Kernel::Detail {

// IsValidVirtualAddressRange does not reject an overflowing end address.
template <typename Memory>
bool ReadArbiterValue64(Memory& memory, std::uint64_t address, std::int64_t& value) {
    if (address > std::numeric_limits<std::uint64_t>::max() - sizeof(value) ||
        !memory.IsValidVirtualAddressRange(address, sizeof(value))) {
        return false;
    }
    value = static_cast<std::int64_t>(memory.Read64(address));
    return true;
}

} // namespace Kernel::Detail

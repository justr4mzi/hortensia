// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <cmath>
#include <cstring>
#include <span>
#include <vector>

#include "audio_core/common/common.h"

namespace Service::Audio::Sysmix {

inline bool IsPlayable(u32 type, u32 channels, u32 rate) {
    return (channels == 2 || channels == 6) && rate == 48000 &&
           (type == static_cast<u32>(AudioCore::SampleFormat::PcmInt16) ||
            type == static_cast<u32>(AudioCore::SampleFormat::PcmFloat));
}

// Keep interleaved FL, FR, C, LFE, BL, BR order for SinkStream's existing downmix.
inline std::vector<s16> ConvertPcm(std::span<const u8> raw, bool is_float, f32 gain) {
    const auto bytes = is_float ? sizeof(f32) : sizeof(s16);
    std::vector<s16> samples(raw.size() / bytes);
    for (std::size_t i = 0; i < samples.size(); ++i) {
        f32 value;
        if (is_float) {
            std::memcpy(&value, raw.data() + i * bytes, bytes);
        } else {
            s16 sample;
            std::memcpy(&sample, raw.data() + i * bytes, bytes);
            value = static_cast<f32>(sample) / 32768.0f;
        }
        value = std::isfinite(value) ? std::clamp(value * gain, -1.0f, 1.0f) : 0.0f;
        samples[i] =
            static_cast<s16>(std::clamp(static_cast<s32>(value * 32768.0f), -32768, 32767));
    }
    return samples;
}

// The reader takes a payload-relative byte offset. Failed copies must not consume the ring.
template <typename Reader>
bool ReadRing(std::span<u8> raw, u32 read, u32 count, u32 capacity, u32 frame_bytes,
              Reader&& reader) {
    if (capacity == 0 || read >= capacity || count >= capacity ||
        raw.size() != u64{count} * frame_bytes) {
        return false;
    }
    const auto first_bytes = std::size_t{(std::min)(count, capacity - read)} * frame_bytes;
    return reader(u64{read} * frame_bytes, raw.first(first_bytes)) &&
           (first_bytes == raw.size() || reader(0, raw.subspan(first_bytes)));
}

} // namespace Service::Audio::Sysmix

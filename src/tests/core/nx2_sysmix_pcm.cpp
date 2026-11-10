// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <limits>
#include <catch2/catch_test_macros.hpp>
#include "audio_core/sink/sink_stream.h"
#include "common/scope_exit.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/hle/service/audio/nx2_sysmix_pcm.h"

using namespace Service::Audio::Sysmix;

TEST_CASE("Sysmix PCM supported formats and channel counts", "[sysmix]") {
    for (u32 channels : {2U, 6U}) {
        REQUIRE(IsPlayable(2, channels, 48000));
        REQUIRE(IsPlayable(5, channels, 48000));
        REQUIRE_FALSE(IsPlayable(5, channels, 44100));
        for (u32 type : {0U, 1U, 3U, 4U, 6U, 99U}) {
            REQUIRE_FALSE(IsPlayable(type, channels, 48000));
        }
    }
    for (u32 channels : {0U, 1U, 3U, 4U, 8U, 0xffffffffU}) {
        REQUIRE_FALSE(IsPlayable(5, channels, 48000));
    }
}

TEST_CASE("Sysmix PCM preserves channels and wrapped frames", "[sysmix]") {
    for (u32 channels : {2U, 6U}) {
        for (bool floating : {false, true}) {
            const u32 bytes = floating ? sizeof(f32) : sizeof(s16);
            const u32 stride = channels * bytes;
            std::vector<u8> ring(4 * stride);
            std::vector<s16> expected;
            for (u32 frame = 0; frame < 4; ++frame) {
                for (u32 channel = 0; channel < channels; ++channel) {
                    const s16 value = static_cast<s16>((frame * channels + channel + 1) * 512);
                    const f32 f = value / 32768.0f;
                    std::memcpy(ring.data() + frame * stride + channel * bytes,
                                floating ? static_cast<const void*>(&f) : &value, bytes);
                }
            }
            std::vector<u8> raw(3 * stride);
            const auto reader = [&](u64 offset, std::span<u8> out) {
                REQUIRE(offset + out.size() <= ring.size());
                std::memcpy(out.data(), ring.data() + offset, out.size());
                return true;
            };
            REQUIRE(ReadRing(raw, 3, 3, 4, stride, reader));
            for (u32 frame : {3U, 0U, 1U}) {
                for (u32 channel = 0; channel < channels; ++channel) {
                    expected.push_back(static_cast<s16>((frame * channels + channel + 1) * 512));
                }
            }
            REQUIRE(ConvertPcm(raw, floating, 1.0f) == expected);
            REQUIRE_FALSE(ReadRing(raw, 4, 3, 4, stride, reader));
            REQUIRE_FALSE(ReadRing(raw, 0, 4, 4, stride, reader));
            REQUIRE_FALSE(ReadRing(raw, 0, 3, 4, stride, [](u64, std::span<u8>) { return false; }));
        }
    }
}

TEST_CASE("Sysmix PCM gain clips and silences nonfinite input", "[sysmix]") {
    const std::array<f32, 6> input{1.0f,
                                   -1.0f,
                                   0.25f,
                                   -0.25f,
                                   std::numeric_limits<f32>::infinity(),
                                   std::numeric_limits<f32>::quiet_NaN()};
    const auto raw = std::span{reinterpret_cast<const u8*>(input.data()), sizeof(input)};
    REQUIRE(ConvertPcm(raw, true, 2.0f) == std::vector<s16>{32767, -32768, 16384, -16384, 0, 0});
    REQUIRE(ConvertPcm(raw, true, 0.0f) == std::vector<s16>(6, 0));
    const std::array<s16, 2> pcm{-32768, 32767};
    const auto bytes = std::span{reinterpret_cast<const u8*>(pcm.data()), sizeof(pcm)};
    REQUIRE(ConvertPcm(bytes, false, 1.0f) == std::vector<s16>{-32768, 32767});
    REQUIRE(ConvertPcm(bytes, false, 2.0f) == std::vector<s16>{-32768, 32767});
}

namespace {
class TestSinkStream final : public AudioCore::Sink::SinkStream {
public:
    TestSinkStream(Core::System& system_, u32 channels, u32 output_channels)
        : SinkStream{system_, AudioCore::Sink::StreamType::Out} {
        system_channels = channels;
        device_channels = output_channels;
    }

    auto TakeSamples() {
        // ReleaseBuffer applies the microphone boost; cancel it for this sample-queue check.
        SetSystemVolume(0.125f);
        auto samples = ReleaseBuffer(device_channels);
        SetSystemVolume(1.0f);
        return samples;
    }
};
} // namespace

TEST_CASE("Sysmix six-channel PCM reaches the sink and stereo downmix", "[sysmix]") {
    Core::System system;
    const auto old_volume = Settings::values.volume.GetValue();
    const auto old_muted = Settings::values.audio_muted.GetValue();
    SCOPE_EXIT {
        Settings::values.volume.SetValue(old_volume);
        Settings::values.audio_muted.SetValue(old_muted);
    };
    Settings::values.volume.SetValue(100);
    Settings::values.audio_muted.SetValue(false);
    for (u32 output_channels : {2U, 6U}) {
        TestSinkStream sink{system, 6, output_channels};
        const std::array<s16, 6> source{1000, 2000, 3000, 4000, 5000, 6000};
        const auto raw = std::span{reinterpret_cast<const u8*>(source.data()), sizeof(source)};
        auto samples = ConvertPcm(raw, false, 1.0f);
        AudioCore::Sink::SinkBuffer buffer{1, 0, 0, false};
        sink.AppendBuffer(buffer, samples);
        REQUIRE(sink.GetQueueSize() == 1);
        const auto result = sink.TakeSamples();
        if (output_channels == 6) {
            REQUIRE(result == std::vector<s16>(source.begin(), source.end()));
        } else {
            // SinkStream's established coefficients: front + .596 center + .354 LFE + .707 rear.
            REQUIRE(result == std::vector<s16>{7739, 9446});
        }
        sink.ClearQueue();
        REQUIRE(sink.GetQueueSize() == 0);
        for (s16 full_scale : {s16{32767}, s16{-32768}}) {
            std::vector<s16> loud(6, full_scale);
            sink.AppendBuffer(buffer, loud);
            REQUIRE(sink.TakeSamples() == std::vector<s16>(output_channels, full_scale));
            sink.ClearQueue();
        }
    }
}

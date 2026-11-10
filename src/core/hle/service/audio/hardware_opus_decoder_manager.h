// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <memory>
#include <opus_multistream.h>

#include "audio_core/opus/decoder_manager.h"
#include "core/hle/service/cmif_types.h"
#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/service.h"

namespace Service::Audio {

class IHardwareOpusDecoder;
class IClientSession;
class HardwareAudioCodecTags;

using AudioCore::OpusDecoder::OpusMultiStreamParameters;
using AudioCore::OpusDecoder::OpusMultiStreamParametersEx;
using AudioCore::OpusDecoder::OpusParameters;
using AudioCore::OpusDecoder::OpusParametersEx;

// Switch 2 hwacod (hardware audio codec) service, reached through hwopus cmd 100. Layouts come from
// the sdk shim (nn::codec::hwacod::shim). Results go into a ring in the client's work buffer.
// Tasks are decoded synchronously in Execute with libopus.
class IHardwareAudioCodecManagerSession final
    : public ServiceFramework<IHardwareAudioCodecManagerSession> {
public:
    explicit IHardwareAudioCodecManagerSession(Core::System& system_, VAddr ring_, u32 ring_count_,
                                               VAddr io_base_, u32 io_input_size_,
                                               u32 io_output_size_);
    ~IHardwareAudioCodecManagerSession() override;

    Kernel::KReadableEvent& GetEvent();

private:
    Result InitializeInstance(u32 index, InBuffer<BufferAttr_HipcPointer> parameter);
    Result FinalizeInstance(u32 index);
    void Execute(HLERequestContext& ctx);

    struct Instance {
        u64 kind{}; // 0 stub (copy), 1 opus, 2 opus multistream
        s32 channels{};
        std::unique_ptr<OpusMSDecoder, void (*)(OpusMSDecoder*)> decoder{nullptr, nullptr};
    };

    KernelHelpers::ServiceContext service_context;
    Kernel::KEvent* event;
    VAddr ring;     // shared header (server half 0x00-0x3f, client read index at +0x44), entries at +0x80
    u32 ring_count; // entries of 0x40 bytes
    VAddr io_base;  // io buffer i: input at io_base + i * (in + out), output right after its input
    u32 io_input_size;
    u32 io_output_size;
    std::map<u32, Instance> instances;
};

class IHardwareAudioCodecClientSession final : public ServiceFramework<IHardwareAudioCodecClientSession> {
public:
    explicit IHardwareAudioCodecClientSession(Core::System& system_);

    // shim::WorkBufferSizeCalculator::GetSharedMemoryParameter() plus the raw words after it
    struct InitializeParameter {
        std::array<u8, 0x60> shared_memory;
        u64 shared_memory_size;
        u64 work_buffer;
        u64 work_buffer_size;
        u64 properties;
    };
    static_assert(sizeof(InitializeParameter) == 0x80);

private:
    Result GetRequiredResourceCount(Out<u32> out_count, InBuffer<BufferAttr_HipcPointer> parameter);
    Result GetServerWorkSize(Out<u32> out_size, u32 io_buffer_count);
    Result Initialize(Out<u32> out_value,
                      Out<SharedPointer<IHardwareAudioCodecManagerSession>> out_session,
                      OutCopyHandle<Kernel::KReadableEvent> out_event, InitializeParameter parameter,
                      InCopyHandle<Kernel::KTransferMemory> tmem,
                      InCopyHandle<Kernel::KProcess> process);
};

class IHardwareOpusDecoderManager final : public ServiceFramework<IHardwareOpusDecoderManager> {
public:
    explicit IHardwareOpusDecoderManager(Core::System& system_);
    ~IHardwareOpusDecoderManager() override;
    Result GetClientSession(Out<SharedPointer<IClientSession>> out_client);

private:
    void GetClientSessionRequest(HLERequestContext& ctx);
    Result OpenHardwareOpusDecoder(Out<SharedPointer<IHardwareOpusDecoder>> out_decoder,
                                   OpusParameters params, u32 tmem_size,
                                   InCopyHandle<Kernel::KTransferMemory> tmem_handle);
    Result GetWorkBufferSize(Out<u32> out_size, OpusParameters params);
    Result OpenHardwareOpusDecoderForMultiStream(
        Out<SharedPointer<IHardwareOpusDecoder>> out_decoder,
        InLargeData<OpusMultiStreamParameters, BufferAttr_HipcPointer> params, u32 tmem_size,
        InCopyHandle<Kernel::KTransferMemory> tmem_handle);
    Result GetWorkBufferSizeForMultiStream(
        Out<u32> out_size, InLargeData<OpusMultiStreamParameters, BufferAttr_HipcPointer> params);
    Result OpenHardwareOpusDecoderEx(Out<SharedPointer<IHardwareOpusDecoder>> out_decoder,
                                     OpusParametersEx params, u32 tmem_size,
                                     InCopyHandle<Kernel::KTransferMemory> tmem_handle);
    Result GetWorkBufferSizeEx(Out<u32> out_size, OpusParametersEx params);
    Result OpenHardwareOpusDecoderForMultiStreamEx(
        Out<SharedPointer<IHardwareOpusDecoder>> out_decoder,
        InLargeData<OpusMultiStreamParametersEx, BufferAttr_HipcPointer> params, u32 tmem_size,
        InCopyHandle<Kernel::KTransferMemory> tmem_handle);
    Result GetWorkBufferSizeForMultiStreamEx(
        Out<u32> out_size, InLargeData<OpusMultiStreamParametersEx, BufferAttr_HipcPointer> params);
    Result GetWorkBufferSizeExEx(Out<u32> out_size, OpusParametersEx params);
    Result GetWorkBufferSizeForMultiStreamExEx(
        Out<u32> out_size, InLargeData<OpusMultiStreamParametersEx, BufferAttr_HipcPointer> params);
    // Switch 2 only: cmd 100 takes no input and returns hwacod::IClientSession (vtable confirmed in sdk NSO).
    Result Unknown100(Out<SharedPointer<IHardwareAudioCodecClientSession>> out_session);

    Core::System& system;
    AudioCore::OpusDecoder::OpusDecoderManager impl;
    std::shared_ptr<HardwareAudioCodecTags> codec_tags;
};

} // namespace Service::Audio

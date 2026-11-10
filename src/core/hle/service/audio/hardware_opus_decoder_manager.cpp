// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/swap.h"
#include "core/core.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_readable_event.h"
#include <fmt/ranges.h>

#include "core/hle/service/audio/hardware_opus_decoder.h"
#include "core/hle/service/audio/hardware_opus_decoder_manager.h"
#include "core/hle/service/audio/hardware_audio_codec.h"
#include "core/hle/service/audio/errors.h"
#include "core/hle/service/cmif_serialization.h"
#include "core/hle/service/ipc_helpers.h"
#include "core/memory.h"

namespace Service::Audio {

using namespace AudioCore::OpusDecoder;

IHardwareOpusDecoderManager::IHardwareOpusDecoderManager(Core::System& system_)
    : ServiceFramework{system_, "hwopus"}, system{system_}, impl{system},
      codec_tags{std::make_shared<HardwareAudioCodecTags>()} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, D<&IHardwareOpusDecoderManager::OpenHardwareOpusDecoder>, "OpenHardwareOpusDecoder"},
        {1, D<&IHardwareOpusDecoderManager::GetWorkBufferSize>, "GetWorkBufferSize"},
        {2, D<&IHardwareOpusDecoderManager::OpenHardwareOpusDecoderForMultiStream>, "OpenOpusDecoderForMultiStream"},
        {3, D<&IHardwareOpusDecoderManager::GetWorkBufferSizeForMultiStream>, "GetWorkBufferSizeForMultiStream"},
        {4, D<&IHardwareOpusDecoderManager::OpenHardwareOpusDecoderEx>, "OpenHardwareOpusDecoderEx"},
        {5, D<&IHardwareOpusDecoderManager::GetWorkBufferSizeEx>, "GetWorkBufferSizeEx"},
        {6, D<&IHardwareOpusDecoderManager::OpenHardwareOpusDecoderForMultiStreamEx>, "OpenHardwareOpusDecoderForMultiStreamEx"},
        {7, D<&IHardwareOpusDecoderManager::GetWorkBufferSizeForMultiStreamEx>, "GetWorkBufferSizeForMultiStreamEx"},
        {8, D<&IHardwareOpusDecoderManager::GetWorkBufferSizeExEx>, "GetWorkBufferSizeExEx"},
        {9, D<&IHardwareOpusDecoderManager::GetWorkBufferSizeForMultiStreamExEx>, "GetWorkBufferSizeForMultiStreamExEx"},
        {100, &IHardwareOpusDecoderManager::GetClientSessionRequest, "GetClientSession"},
    };
    // clang-format on
    RegisterHandlers(functions);
}

IHardwareOpusDecoderManager::~IHardwareOpusDecoderManager() = default;

void IHardwareOpusDecoderManager::GetClientSessionRequest(HLERequestContext& ctx) {
    if (!GuardHardwareAudioCodecRequest(ctx, {0, 0, Hwacod::InputBuffer::None})) {
        return;
    }
    try {
        CmifReplyWrap<true, &IHardwareOpusDecoderManager::GetClientSession>(ctx);
    } catch (const std::bad_alloc&) {
        IPC::ResponseBuilder reply{ctx, 2};
        reply.Push(ResultLibOpusAllocFail);
    }
}

Result IHardwareOpusDecoderManager::GetClientSession(Out<SharedPointer<IClientSession>> out_client) {
    *out_client = std::make_shared<IClientSession>(system, codec_tags);
    R_SUCCEED();
}

Result IHardwareOpusDecoderManager::OpenHardwareOpusDecoder(
    Out<SharedPointer<IHardwareOpusDecoder>> out_decoder, OpusParameters params, u32 tmem_size,
    InCopyHandle<Kernel::KTransferMemory> tmem_handle) {
    LOG_DEBUG(Service_Audio, "sample_rate {} channel_count {} transfer_memory_size {:#x}",
              params.sample_rate, params.channel_count, tmem_size);

    auto decoder{std::make_shared<IHardwareOpusDecoder>(system, impl.GetHardwareOpus())};
    OpusParametersEx ex{
        .sample_rate = params.sample_rate,
        .channel_count = params.channel_count,
        .use_large_frame_size = false,
    };
    R_TRY(decoder->Initialize(ex, tmem_handle.Get(), tmem_size));

    *out_decoder = decoder;
    R_SUCCEED();
}

Result IHardwareOpusDecoderManager::GetWorkBufferSize(Out<u32> out_size, OpusParameters params) {
    R_TRY(impl.GetWorkBufferSize(params, *out_size));
    LOG_DEBUG(Service_Audio, "sample_rate {} channel_count {} -- returned size {:#x}",
              params.sample_rate, params.channel_count, *out_size);
    R_SUCCEED();
}

Result IHardwareOpusDecoderManager::OpenHardwareOpusDecoderForMultiStream(
    Out<SharedPointer<IHardwareOpusDecoder>> out_decoder,
    InLargeData<OpusMultiStreamParameters, BufferAttr_HipcPointer> params, u32 tmem_size,
    InCopyHandle<Kernel::KTransferMemory> tmem_handle) {
    LOG_DEBUG(Service_Audio,
              "sample_rate {} channel_count {} total_stream_count {} stereo_stream_count {} "
              "transfer_memory_size {:#x}",
              params->sample_rate, params->channel_count, params->total_stream_count,
              params->stereo_stream_count, tmem_size);

    auto decoder{std::make_shared<IHardwareOpusDecoder>(system, impl.GetHardwareOpus())};

    OpusMultiStreamParametersEx ex{
        .sample_rate = params->sample_rate,
        .channel_count = params->channel_count,
        .total_stream_count = params->total_stream_count,
        .stereo_stream_count = params->stereo_stream_count,
        .use_large_frame_size = false,
        .mappings{},
    };
    std::memcpy(ex.mappings.data(), params->mappings.data(), sizeof(params->mappings));
    R_TRY(decoder->Initialize(ex, tmem_handle.Get(), tmem_size));

    *out_decoder = decoder;
    R_SUCCEED();
}

Result IHardwareOpusDecoderManager::GetWorkBufferSizeForMultiStream(
    Out<u32> out_size, InLargeData<OpusMultiStreamParameters, BufferAttr_HipcPointer> params) {
    R_TRY(impl.GetWorkBufferSizeForMultiStream(*params, *out_size));
    LOG_DEBUG(Service_Audio, "size {:#x}", *out_size);
    R_SUCCEED();
}

Result IHardwareOpusDecoderManager::OpenHardwareOpusDecoderEx(
    Out<SharedPointer<IHardwareOpusDecoder>> out_decoder, OpusParametersEx params, u32 tmem_size,
    InCopyHandle<Kernel::KTransferMemory> tmem_handle) {
    LOG_DEBUG(Service_Audio, "sample_rate {} channel_count {} transfer_memory_size {:#x}",
              params.sample_rate, params.channel_count, tmem_size);

    auto decoder{std::make_shared<IHardwareOpusDecoder>(system, impl.GetHardwareOpus())};
    R_TRY(decoder->Initialize(params, tmem_handle.Get(), tmem_size));

    *out_decoder = decoder;
    R_SUCCEED();
}

Result IHardwareOpusDecoderManager::GetWorkBufferSizeEx(Out<u32> out_size,
                                                        OpusParametersEx params) {
    R_TRY(impl.GetWorkBufferSizeEx(params, *out_size));
    LOG_DEBUG(Service_Audio, "size {:#x}", *out_size);
    R_SUCCEED();
}

Result IHardwareOpusDecoderManager::OpenHardwareOpusDecoderForMultiStreamEx(
    Out<SharedPointer<IHardwareOpusDecoder>> out_decoder,
    InLargeData<OpusMultiStreamParametersEx, BufferAttr_HipcPointer> params, u32 tmem_size,
    InCopyHandle<Kernel::KTransferMemory> tmem_handle) {
    LOG_DEBUG(Service_Audio,
              "sample_rate {} channel_count {} total_stream_count {} stereo_stream_count {} "
              "use_large_frame_size {}"
              "transfer_memory_size {:#x}",
              params->sample_rate, params->channel_count, params->total_stream_count,
              params->stereo_stream_count, params->use_large_frame_size, tmem_size);

    auto decoder{std::make_shared<IHardwareOpusDecoder>(system, impl.GetHardwareOpus())};

    R_TRY(decoder->Initialize(*params, tmem_handle.Get(), tmem_size));

    *out_decoder = decoder;
    R_SUCCEED();
}

Result IHardwareOpusDecoderManager::GetWorkBufferSizeForMultiStreamEx(
    Out<u32> out_size, InLargeData<OpusMultiStreamParametersEx, BufferAttr_HipcPointer> params) {
    R_TRY(impl.GetWorkBufferSizeForMultiStreamEx(*params, *out_size));
    LOG_DEBUG(Service_Audio,
              "sample_rate {} channel_count {} total_stream_count {} stereo_stream_count {} "
              "use_large_frame_size {} -- returned size {:#x}",
              params->sample_rate, params->channel_count, params->total_stream_count,
              params->stereo_stream_count, params->use_large_frame_size, *out_size);
    R_SUCCEED();
}

Result IHardwareOpusDecoderManager::GetWorkBufferSizeExEx(Out<u32> out_size,
                                                          OpusParametersEx params) {
    R_TRY(impl.GetWorkBufferSizeExEx(params, *out_size));
    LOG_DEBUG(Service_Audio, "size {:#x}", *out_size);
    R_SUCCEED();
}

Result IHardwareOpusDecoderManager::GetWorkBufferSizeForMultiStreamExEx(
    Out<u32> out_size, InLargeData<OpusMultiStreamParametersEx, BufferAttr_HipcPointer> params) {
    R_TRY(impl.GetWorkBufferSizeForMultiStreamExEx(*params, *out_size));
    LOG_DEBUG(Service_Audio, "size {:#x}", *out_size);
    R_SUCCEED();
}

IHardwareAudioCodecManagerSession::IHardwareAudioCodecManagerSession(
    Core::System& system_, VAddr ring_, u32 ring_count_, VAddr io_base_, u32 io_input_size_,
    u32 io_output_size_)
    : ServiceFramework{system_, "hwacod::IHardwareAudioCodecManagerSession"},
      service_context{system_, "hwacod"}, event{service_context.CreateEvent("hwacod:Result")},
      ring{ring_}, ring_count{ring_count_}, io_base{io_base_}, io_input_size{io_input_size_},
      io_output_size{io_output_size_} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, D<&IHardwareAudioCodecManagerSession::InitializeInstance>, "InitializeInstance"},
        {1, D<&IHardwareAudioCodecManagerSession::FinalizeInstance>, "FinalizeInstance"},
        {10, &IHardwareAudioCodecManagerSession::Execute, "Execute"},
    };
    // clang-format on
    RegisterHandlers(functions);
}

IHardwareAudioCodecManagerSession::~IHardwareAudioCodecManagerSession() {
    service_context.CloseEvent(event);
}

Kernel::KReadableEvent& IHardwareAudioCodecManagerSession::GetEvent() {
    return event->GetReadableEvent();
}

Result IHardwareAudioCodecManagerSession::InitializeInstance(
    u32 index, InBuffer<BufferAttr_HipcPointer> parameter) {
    // shim parameter: u64 kind, s32 sample rate +8, s32 channels +0xc, TimeSpan frame +0x10;
    // multistream adds s32 total streams +0x18, s32 stereo streams +0x1c, u8 mapping[255] +0x20
    std::array<u8, 0x120> p{};
    std::memcpy(p.data(), parameter.data(), std::min(parameter.size(), p.size()));
    Instance inst{};
    s32 sample_rate{}, streams{1}, stereo{};
    std::memcpy(&inst.kind, p.data(), 8);
    std::memcpy(&sample_rate, p.data() + 8, 4);
    std::memcpy(&inst.channels, p.data() + 0xc, 4);
    std::array<u8, 255> mapping{0, 1};
    if (inst.kind == 2) {
        std::memcpy(&streams, p.data() + 0x18, 4);
        std::memcpy(&stereo, p.data() + 0x1c, 4);
        std::memcpy(mapping.data(), p.data() + 0x20, mapping.size());
    } else {
        stereo = inst.channels == 2 ? 1 : 0;
    }
    if (inst.kind == 1 || inst.kind == 2) {
        int error{};
        OpusMSDecoder* decoder = opus_multistream_decoder_create(
            sample_rate, inst.channels, streams, stereo, mapping.data(), &error);
        if (decoder == nullptr) {
            LOG_ERROR(Service_Audio, "hwacod instance {}: opus init failed ({}), {} Hz {} ch", index,
                      error, sample_rate, inst.channels);
        }
        inst.decoder = {decoder, opus_multistream_decoder_destroy};
    }
    LOG_INFO(Service_Audio, "hwacod instance {} kind {} {} Hz {} ch streams {}/{}", index, inst.kind,
             sample_rate, inst.channels, streams, stereo);
    instances.insert_or_assign(index, std::move(inst));
    R_SUCCEED();
}

Result IHardwareAudioCodecManagerSession::FinalizeInstance(u32 index) {
    instances.erase(index);
    R_SUCCEED();
}

void IHardwareAudioCodecManagerSession::Execute(HLERequestContext& ctx) {
    // 16-byte tasks: u16 instance +2, u16 io buffer +4, u32 input size +8. Each gets a 0x40-byte
    // result entry: u16 instance +2, u16 io buffer +4, TimeSpan +8, u32 output bytes +0x10, Result +0x14.
    const auto tasks = ctx.ReadBuffer();
    auto& memory = system.ApplicationMemory();
    std::vector<u8> input;
    std::vector<s16> output(io_output_size / sizeof(s16));
    u32 write = memory.Read32(ring + 4);
    for (size_t off = 0; off + 16 <= tasks.size(); off += 16) {
        u16 instance{}, io{};
        u32 input_size{};
        std::memcpy(&instance, tasks.data() + off + 2, 2);
        std::memcpy(&io, tasks.data() + off + 4, 2);
        std::memcpy(&input_size, tasks.data() + off + 8, 4);
        const VAddr in_addr = io_base + u64{io} * (io_input_size + io_output_size);
        input_size = std::min(input_size, io_input_size);
        input.resize(input_size);
        memory.ReadBlock(in_addr, input.data(), input.size());

        u32 out_bytes{};
        u32 result = ResultSuccess.raw;
        const auto it = instances.find(instance);
        if (it == instances.end()) {
            result = ResultUnknown.raw;
        } else if (it->second.kind == 0) {
            out_bytes = std::min(input_size, io_output_size);
            memory.WriteBlock(in_addr + io_input_size, input.data(), out_bytes);
        } else if (it->second.decoder && it->second.channels > 0) {
            // some streams carry the Switch 1 hwopus packet header (u32 BE size, u32 final range)
            size_t skip = 0;
            if (input.size() >= 8 && Common::swap32(*reinterpret_cast<const u32*>(input.data())) ==
                                         input.size() - 8) {
                skip = 8;
            }
            const int frames = opus_multistream_decode(
                it->second.decoder.get(), input.data() + skip,
                static_cast<opus_int32>(input.size() - skip),
                output.data(), static_cast<int>(output.size() / it->second.channels), 0);
            if (frames < 0) {
                LOG_WARNING(Service_Audio,
                        "hwacod instance {}: opus decode of {} bytes failed ({}), starts {:02x}",
                        instance, input_size, frames,
                        fmt::join(input.begin(), input.begin() + std::min<size_t>(16, input.size()), " "));
            } else {
                out_bytes = static_cast<u32>(frames * it->second.channels * sizeof(s16));
                memory.WriteBlock(in_addr + io_input_size, output.data(), out_bytes);
            }
        }

        std::array<u8, 0x40> entry{};
        std::memcpy(entry.data(), tasks.data() + off, 8);
        const u64 time_ns = 100'000;
        std::memcpy(entry.data() + 8, &time_ns, sizeof(time_ns));
        std::memcpy(entry.data() + 0x10, &out_bytes, sizeof(out_bytes));
        std::memcpy(entry.data() + 0x14, &result, sizeof(result));
        memory.WriteBlock(ring + 0x80 + u64{write} * 0x40, entry.data(), entry.size());
        write = write + 1 >= ring_count ? 0 : write + 1;
    }
    memory.Write32(ring + 4, write);
    event->Signal(system.Kernel());

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

IHardwareAudioCodecClientSession::IHardwareAudioCodecClientSession(Core::System& system_)
    : ServiceFramework{system_, "hwacod::IClientSession"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, D<&IHardwareAudioCodecClientSession::GetRequiredResourceCount>, "GetRequiredResourceCount"},
        {1, D<&IHardwareAudioCodecClientSession::GetServerWorkSize>, "GetServerWorkSize"},
        {10, D<&IHardwareAudioCodecClientSession::Initialize>, "Initialize"},
    };
    // clang-format on
    RegisterHandlers(functions);
}

Result IHardwareAudioCodecClientSession::GetRequiredResourceCount(
    Out<u32> out_count, InBuffer<BufferAttr_HipcPointer> parameter) {
    // CanInstantiate compares this against the manager's resource budget; nothing is reserved here.
    *out_count = 0;
    R_SUCCEED();
}

Result IHardwareAudioCodecClientSession::GetServerWorkSize(Out<u32> out_size, u32 io_buffer_count) {
    // Extra work buffer space the server keeps per manager; HLE needs none.
    *out_size = 0;
    R_SUCCEED();
}

Result IHardwareAudioCodecClientSession::Initialize(
    Out<u32> out_value, Out<SharedPointer<IHardwareAudioCodecManagerSession>> out_session,
    OutCopyHandle<Kernel::KReadableEvent> out_event, InitializeParameter parameter,
    InCopyHandle<Kernel::KTransferMemory> tmem, InCopyHandle<Kernel::KProcess> process) {
    // GetSharedMemoryParameter(): io buffer offset +0x24, result ring offset +0x2c,
    // io buffer input/output sizes +0x3c/+0x40, io buffer count +0x44 (the ring holds one more)
    const auto field = [&](size_t offset) {
        u32 value{};
        std::memcpy(&value, parameter.shared_memory.data() + offset, sizeof(value));
        return value;
    };
    const u32 io_offset = field(0x24), ring_offset = field(0x2c), io_buffer_count = field(0x44);
    const u32 in_size = field(0x3c), out_size = field(0x40);
    LOG_INFO(Service_Audio,
             "hwacod init: work buffer {:#x} size {:#x}, ring {:#x}, io {:#x} x{} ({:#x} in, {:#x} out)",
             parameter.work_buffer, parameter.work_buffer_size, ring_offset, io_offset,
             io_buffer_count, in_size, out_size);
    auto session = std::make_shared<IHardwareAudioCodecManagerSession>(
        system, parameter.work_buffer + ring_offset, io_buffer_count + 1,
        parameter.work_buffer + io_offset, in_size, out_size);
    *out_event = &session->GetEvent();
    *out_session = session;
    *out_value = 0;
    R_SUCCEED();
}

Result IHardwareOpusDecoderManager::Unknown100(
    Out<SharedPointer<IHardwareAudioCodecClientSession>> out_session) {
    LOG_WARNING(Service_Audio, "Switch 2 hwopus cmd 100 called, returning hwacod IClientSession");
    *out_session = std::make_shared<IHardwareAudioCodecClientSession>(system);
    R_SUCCEED();
}

} // namespace Service::Audio

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <cstdlib>
#include <mutex>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "common/settings.h"
#include "core/hle/kernel/k_page_table.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_transfer_memory.h"
#include "core/hle/service/am/applet_data_broker.h"
#include "core/hle/service/am/applet_manager.h"
#include "core/hle/service/am/frontend/applets.h"
#include "core/hle/service/am/library_applet_storage.h"
#include "core/hle/service/am/process_creation.h"
#include "core/hle/service/am/service/library_applet_accessor.h"
#include "core/hle/service/am/service/library_applet_creator.h"

#include "core/hle/api_version.h"
#include "core/hle/service/am/service/storage.h"
#include "core/hle/service/am/window_system.h"
#include "core/hle/service/cmif_serialization.h"
#include "core/hle/service/sm/sm.h"

namespace Service::AM {

namespace {

// Switch 2 titles ask for the Switch 1 applets by new ids.
AppletId FromNx2AppletId(AppletId applet_id) {
    switch (applet_id) {
    case AppletId::SoftwareKeyboardNx2:
        return AppletId::SoftwareKeyboard;
    case AppletId::ControllerNx2:
        return AppletId::Controller;
    default:
        return applet_id;
    }
}

bool ShouldCreateGuestApplet(AppletId applet_id) {
#define X(Name, name)                                                                              \
    if (applet_id == AppletId::Name &&                                                             \
        Settings::values.name##_applet_mode.GetValue() != Settings::AppletMode::LLE) {             \
        return false;                                                                              \
    }

    X(Cabinet, cabinet)
    X(Controller, controller)
    X(DataErase, data_erase)
    X(Error, error)
    X(NetConnect, net_connect)
    X(ProfileSelect, player_select)
    X(SoftwareKeyboard, swkbd)
    X(MiiEdit, mii_edit)
    X(Web, web)
    X(Shop, shop)
    X(PhotoViewer, photo_viewer)
    X(OfflineWeb, offline_web)
    X(LoginShare, login_share)
    X(WebAuth, wifi_web_auth)
    X(MyPage, my_page)

#undef X

    return true;
}

AppletProgramId AppletIdToProgramId(AppletId applet_id) {
    switch (applet_id) {
    case AppletId::OverlayDisplay:
        return AppletProgramId::OverlayDisplay;
    case AppletId::QLaunch:
        return AppletProgramId::QLaunch;
    case AppletId::Starter:
        return AppletProgramId::Starter;
    case AppletId::Auth:
        return AppletProgramId::Auth;
    case AppletId::Cabinet:
        return AppletProgramId::Cabinet;
    case AppletId::Controller:
        return AppletProgramId::Controller;
    case AppletId::DataErase:
        return AppletProgramId::DataErase;
    case AppletId::Error:
        return AppletProgramId::Error;
    case AppletId::NetConnect:
        return AppletProgramId::NetConnect;
    case AppletId::ProfileSelect:
        return AppletProgramId::ProfileSelect;
    case AppletId::SoftwareKeyboard:
        return AppletProgramId::SoftwareKeyboard;
    case AppletId::MiiEdit:
        return AppletProgramId::MiiEdit;
    case AppletId::Web:
        return AppletProgramId::Web;
    case AppletId::Shop:
        return AppletProgramId::Shop;
    case AppletId::PhotoViewer:
        return AppletProgramId::PhotoViewer;
    case AppletId::Settings:
        return AppletProgramId::Settings;
    case AppletId::OfflineWeb:
        return AppletProgramId::OfflineWeb;
    case AppletId::LoginShare:
        return AppletProgramId::LoginShare;
    case AppletId::WebAuth:
        return AppletProgramId::WebAuth;
    case AppletId::MyPage:
        return AppletProgramId::MyPage;
    default:
        return static_cast<AppletProgramId>(0);
    }
}

AppletId NormalizeAppletId(AppletId id) {
    const u32 raw_id = static_cast<u32>(id);
    static std::mutex mutex;
    static std::unordered_set<u32> logged_raw_ids;
    static std::unordered_set<u32> warned_raw_ids;

    bool log_info = false;
    {
        std::scoped_lock lk{mutex};
        log_info = logged_raw_ids.insert(raw_id).second;
    }
    if (log_info) {
        LOG_INFO(Service_AM, "CreateLibraryApplet called with raw applet_id={:#x}", raw_id);
    }

    if (AppletIdToProgramId(id) == static_cast<AppletProgramId>(0)) {
        const auto low_byte = static_cast<AppletId>(raw_id & 0xFF);
        if (AppletIdToProgramId(low_byte) != static_cast<AppletProgramId>(0)) {
            bool log_warning = false;
            {
                std::scoped_lock lk{mutex};
                log_warning = warned_raw_ids.insert(raw_id).second;
            }
            if (log_warning) {
                LOG_WARNING(Service_AM,
                            "Normalizing unknown raw applet_id={:#x} to low byte {:#x}",
                            raw_id, static_cast<u32>(low_byte));
            }
            return low_byte;
        }
    }

    return id;
}

std::shared_ptr<ILibraryAppletAccessor> CreateGuestApplet(Core::System& system,
                                                          WindowSystem& window_system,
                                                          std::shared_ptr<Applet> caller_applet,
                                                          AppletId applet_id,
                                                          LibraryAppletMode mode) {
    const auto program_id = static_cast<u64>(AppletIdToProgramId(applet_id));
    if (program_id == 0) {
        // Unknown applet
        return {};
    }

    auto process = CreateProcess(system, program_id, 1, HLE::ApiVersion::HOS_VERSION_MAJOR);
    if (process) {
        const auto applet = std::make_shared<Applet>(system, std::move(process), false);
        applet->program_id = program_id;
        applet->applet_id = applet_id;
        applet->type = AppletType::LibraryApplet;
        applet->library_applet_mode = mode;
        applet->window_visible = mode != LibraryAppletMode::AllForegroundInitiallyHidden;

        auto broker = std::make_shared<AppletDataBroker>(system);
        applet->caller_applet = caller_applet;
        applet->caller_applet_broker = broker;
        {
            std::scoped_lock lk{caller_applet->lock};
            caller_applet->child_applets.push_back(applet);
        }
        window_system.TrackApplet(applet, false);
        return std::make_shared<ILibraryAppletAccessor>(system, broker, applet);
    }
    // Couldn't initialize the guest process
    return {};
}

std::shared_ptr<ILibraryAppletAccessor> CreateFrontendApplet(Core::System& system,
                                                             WindowSystem& window_system,
                                                             std::shared_ptr<Applet> caller_applet,
                                                             AppletId applet_id,
                                                             LibraryAppletMode mode) {
    const auto program_id = static_cast<u64>(AppletIdToProgramId(applet_id));

    auto process = std::make_unique<Process>(system);
    auto applet = std::make_shared<Applet>(system, std::move(process), false);
    applet->program_id = program_id;
    applet->applet_id = applet_id;
    applet->type = AppletType::LibraryApplet;
    applet->library_applet_mode = mode;

    auto storage = std::make_shared<AppletDataBroker>(system);
    applet->caller_applet = caller_applet;
    applet->caller_applet_broker = storage;
    applet->frontend = system.GetFrontendAppletHolder().GetApplet(applet, applet_id, mode);
    {
        std::scoped_lock lk{caller_applet->lock};
        caller_applet->child_applets.push_back(applet);
    }
    return std::make_shared<ILibraryAppletAccessor>(system, storage, applet);
}

} // namespace

ILibraryAppletCreator::ILibraryAppletCreator(Core::System& system_, std::shared_ptr<Applet> applet,
                                             WindowSystem& window_system)
    : ServiceFramework{system_, "ILibraryAppletCreator"},
      m_window_system{window_system}, m_applet{std::move(applet)} {
    static const FunctionInfo functions[] = {
        {0, D<&ILibraryAppletCreator::CreateLibraryApplet>, "CreateLibraryApplet"},
        {1, &ILibraryAppletCreator::StubReplyZeroed, "TerminateAllLibraryApplets"},
        {2, &ILibraryAppletCreator::StubReplyZeroed, "AreAnyLibraryAppletsLeft"},
        {3, D<&ILibraryAppletCreator::CreateLibraryAppletEx>, "CreateLibraryAppletEx"},
        {10, D<&ILibraryAppletCreator::CreateStorage>, "CreateStorage"},
        {11, D<&ILibraryAppletCreator::CreateTransferMemoryStorage>, "CreateTransferMemoryStorage"},
        {12, D<&ILibraryAppletCreator::CreateHandleStorage>, "CreateHandleStorage"},
    };
    RegisterHandlers(functions);
}

ILibraryAppletCreator::~ILibraryAppletCreator() = default;

// ponytail: DxD round-9 debug, dumps the game's controller-manager object (EDEN_DXD_BT only)
static void DxdDumpControllerState(Core::System& system) {
    auto& mem = system.ApplicationMemory();
    // ponytail: FindModules() finds no named modules here, so walk the page table for code regions
    std::vector<std::pair<u64, std::string>> mods;
    auto& pt = system.ApplicationProcess()->GetPageTable();
    VAddr cur = 0;
    while (true) {
        Kernel::KMemoryInfo info{};
        Kernel::Svc::PageInfo pinfo{};
        if (pt.QueryInfo(std::addressof(info), std::addressof(pinfo), cur).IsError()) {
            break;
        }
        const auto svc = info.GetSvcMemoryInfo();
        if (svc.permission == Kernel::Svc::MemoryPermission::ReadExecute &&
            (svc.state == Kernel::Svc::MemoryState::Code ||
             svc.state == Kernel::Svc::MemoryState::AliasCode)) {
            mods.emplace_back(svc.base_address, std::string{});
        }
        const u64 next = svc.base_address + svc.size;
        if (next <= cur) {
            break;
        }
        cur = next;
    }
    // ponytail: code regions come in rtld, main, sdk order
    u64 base = 0;
    for (size_t idx = 0; idx < mods.size(); idx++) {
        LOG_ERROR(Service_AM, "DXD_R9 code region {} at {:#x} MOD0={:#x}", idx, mods[idx].first,
                  mem.Read32(mods[idx].first + 4));
        if (idx == 1) {
            base = mods[idx].first;
        }
    }
    if (base == 0) {
        LOG_ERROR(Service_AM, "DXD_R9 main module not found");
        return;
    }
    // ponytail: nearest module start below addr, ignores module end
    const auto where = [&](u64 a) {
        std::string r = "?";
        for (size_t idx = 0; idx < mods.size(); idx++) {
            if (a >= mods[idx].first) {
                r = fmt::format("reg{}+{:#x}", idx, a - mods[idx].first);
            }
        }
        return r;
    };
    const u64 g = mem.Read64(base + 0x227c5e8);
    const u64 obj = mem.Read64(g);
    LOG_ERROR(Service_AM, "DXD_R9 main={:#x} g={:#x} obj={:#x}", base, g, obj);
    if (obj == 0) {
        return;
    }
    LOG_ERROR(Service_AM, "DXD_R9 obj+0x178 n(s8)={} obj+0x17c flags={:#x}",
              static_cast<s8>(mem.Read8(obj + 0x178)), mem.Read32(obj + 0x17c));
    for (const u64 off : std::array<u64, 4>{0x30, 0x50, 0x70, 0x90}) {
        const u64 s = mem.Read64(obj + off);
        if (s == 0) {
            LOG_ERROR(Service_AM, "DXD_R9 obj+{:#x} null", off);
            continue;
        }
        const u64 vt = mem.Read64(s);
        const u64 fn = mem.Read64(vt + 0x20);
        LOG_ERROR(Service_AM,
                  "DXD_R9 obj+{:#x}: s={:#x} vt={:#x} ({}) fn+0x20={:#x} ({}) s+8={:#x} s+0x17c={:#x}",
                  off, s, vt, where(vt), fn, where(fn), mem.Read32(s + 8), mem.Read8(s + 0x17c));
    }
    // ponytail: DxD round-10 debug, player object s1=obj+0x30 holds +0x17c flags; log its
    // style index and the nn::hid-style record it last read (sampling, buttons, attributes).
    const u64 s1 = mem.Read64(obj + 0x30);
    if (s1 != 0) {
        const u64 rec = mem.Read64(s1 + 0x180);
        LOG_ERROR(Service_AM, "DXD_R10 s1={:#x} +0x178={:#x} +0x17c={:#x} rec(+0x180)={:#x}",
                  s1, mem.Read32(s1 + 0x178), mem.Read8(s1 + 0x17c), rec);
        if (rec != 0) {
            LOG_ERROR(Service_AM, "DXD_R10 rec sampling={} buttons={:#x} attr={:#x} ({})",
                      mem.Read64(rec), mem.Read64(rec + 8), mem.Read32(rec + 0x20), where(rec));
        }
    }
    const u64 x23 = mem.Read64(mem.Read64(base + 0x227c5e0));
    LOG_ERROR(Service_AM, "DXD_R9 list x23={:#x} head={:#x} sentinel={:#x}", x23,
              mem.Read64(x23 + 0x168), x23 + 0x160);
}

Result ILibraryAppletCreator::CreateLibraryApplet(
    Out<SharedPointer<ILibraryAppletAccessor>> out_library_applet_accessor, AppletId applet_id,
    LibraryAppletMode library_applet_mode) {
    LOG_DEBUG(Service_AM, "called with applet_id={} applet_mode={}", applet_id,
              library_applet_mode);

    applet_id = NormalizeAppletId(applet_id);
    // ponytail: DxD round-8 debug, EDEN_DXD_BT=1 logs guest backtrace on controller applet creation
    // ponytail: IPC runs on a host thread, so CurrentPhysicalCore() is unusable; dump every guest core.
    if (applet_id == AppletId::Controller && std::getenv("EDEN_DXD_BT")) {
        DxdDumpControllerState(system);
        auto* proc = system.ApplicationProcess();
        for (s32 core = 0; core < 4; core++) {
            if (auto* arm = proc->GetArmInterface(core)) {
                LOG_ERROR(Service_AM, "DXD_BT core={}", core);
                arm->LogBacktrace(proc);
            }
        }
    }

    std::shared_ptr<ILibraryAppletAccessor> library_applet;
    if (ShouldCreateGuestApplet(applet_id)) {
        library_applet =
            CreateGuestApplet(system, m_window_system, m_applet, applet_id, library_applet_mode);
    }
    if (!library_applet) {
        library_applet =
            CreateFrontendApplet(system, m_window_system, m_applet, applet_id, library_applet_mode);
    }
    if (!library_applet) {
        LOG_ERROR(Service_AM, "Applet doesn't exist! applet_id={}", applet_id);
        R_THROW(ResultUnknown);
    }

    // Applet is created, can now be launched.
    m_applet->library_applet_launchable_event.Signal(system.Kernel());
    *out_library_applet_accessor = library_applet;
    R_SUCCEED();
}

Result ILibraryAppletCreator::CreateLibraryAppletEx(
    Out<SharedPointer<ILibraryAppletAccessor>> out_library_applet_accessor, AppletId applet_id,
    LibraryAppletMode library_applet_mode, u64 thread_id) {
    LOG_DEBUG(Service_AM, "called with applet_id={} applet_mode={} thread_id={}", applet_id,
              library_applet_mode, thread_id);

    applet_id = NormalizeAppletId(applet_id);
    // ponytail: DxD round-8 debug, EDEN_DXD_BT=1 logs guest backtrace on controller applet creation
    if (applet_id == AppletId::Controller && std::getenv("EDEN_DXD_BT")) {
        DxdDumpControllerState(system);
        auto* proc = system.ApplicationProcess();
        for (s32 core = 0; core < 4; core++) {
            if (auto* arm = proc->GetArmInterface(core)) {
                LOG_ERROR(Service_AM, "DXD_BT core={}", core);
                arm->LogBacktrace(proc);
            }
        }
    }

    std::shared_ptr<ILibraryAppletAccessor> library_applet;
    if (ShouldCreateGuestApplet(applet_id)) {
        library_applet =
            CreateGuestApplet(system, m_window_system, m_applet, applet_id, library_applet_mode);
    }
    if (!library_applet) {
        library_applet =
            CreateFrontendApplet(system, m_window_system, m_applet, applet_id, library_applet_mode);
    }
    if (!library_applet) {
        LOG_ERROR(Service_AM, "Applet doesn't exist! applet_id={}", applet_id);
        R_THROW(ResultUnknown);
    }

    // Applet is created, can now be launched.
    m_applet->library_applet_launchable_event.Signal(system.Kernel());
    *out_library_applet_accessor = library_applet;
    R_SUCCEED();
}

Result ILibraryAppletCreator::CreateStorage(Out<SharedPointer<IStorage>> out_storage, s64 size) {
    LOG_DEBUG(Service_AM, "called, size={}", size);

    if (size <= 0) {
        LOG_ERROR(Service_AM, "size is less than or equal to 0");
        R_THROW(ResultUnknown);
    }

    *out_storage = std::make_shared<IStorage>(system, AM::CreateStorage(std::vector<u8>(size)));
    R_SUCCEED();
}

Result ILibraryAppletCreator::CreateTransferMemoryStorage(
    Out<SharedPointer<IStorage>> out_storage, bool is_writable, s64 size,
    InCopyHandle<Kernel::KTransferMemory> transfer_memory_handle) {
    LOG_DEBUG(Service_AM, "called, is_writable={} size={}", is_writable, size);

    if (size <= 0) {
        LOG_ERROR(Service_AM, "size is less than or equal to 0");
        R_THROW(ResultUnknown);
    }

    if (!transfer_memory_handle) {
        LOG_ERROR(Service_AM, "transfer_memory_handle is null");
        R_THROW(ResultUnknown);
    }

    *out_storage = std::make_shared<IStorage>(
        system, AM::CreateTransferMemoryStorage(system.Kernel(), transfer_memory_handle->GetOwner()->GetMemory(),
                                                transfer_memory_handle.Get(), is_writable, size));
    R_SUCCEED();
}

Result ILibraryAppletCreator::CreateHandleStorage(
    Out<SharedPointer<IStorage>> out_storage, s64 size,
    InCopyHandle<Kernel::KTransferMemory> transfer_memory_handle) {
    LOG_DEBUG(Service_AM, "called, size={}", size);

    if (size <= 0) {
        LOG_ERROR(Service_AM, "size is less than or equal to 0");
        R_THROW(ResultUnknown);
    }

    if (!transfer_memory_handle) {
        LOG_ERROR(Service_AM, "transfer_memory_handle is null");
        R_THROW(ResultUnknown);
    }

    *out_storage = std::make_shared<IStorage>(
        system, AM::CreateHandleStorage(system.Kernel(), transfer_memory_handle->GetOwner()->GetMemory(),
                                        transfer_memory_handle.Get(), size));
    R_SUCCEED();
}

} // namespace Service::AM

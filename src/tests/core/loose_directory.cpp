// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string_view>
#include <vector>
#include "common/fs/fs_util.h"
#include "core/core.h"
#include "core/file_sys/vfs/vfs_real.h"
#include "core/loader/loose_directory.h"
#include "core/loader/deconstructed_rom_directory.h"
#include "core/file_sys/romfs.h"
#include "core/file_sys/registered_cache.h"
#include "core/file_sys/romfs_factory.h"
#include "core/hle/service/filesystem/filesystem.h"
#include "core/loader/loader.h"
#include "qt_common/game_list/discovery.h"

namespace {
namespace fs = std::filesystem;
struct LooseTestDirectory {
    fs::path root = fs::temp_directory_path() /
                    ("loose-program-test-" +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ~LooseTestDirectory() {
        std::error_code ec;
        fs::remove_all(root, ec);
    }
    static void Put(const fs::path& file, std::string_view content = "main") {
        fs::create_directories(file.parent_path());
        std::ofstream(file, std::ios::binary) << content;
    }
    static void ExeFS(const fs::path& dir, std::string_view content = "main") {
        Put(dir / "main", content);
        Put(dir / "main.npdm", "npdm");
    }
};
} // namespace

TEST_CASE("Loose edition folders prefer a complete native program and retain legacy fallback",
          "[loose_directory]") {
    LooseTestDirectory t;
    const auto game = t.root / fs::path{u8"Kirby 日本語"};
    const auto programs = game / "upgrade/65536/program";
    LooseTestDirectory::ExeFS(game / "base/exefs", "base");
    LooseTestDirectory::ExeFS(game / "updates/65536/exefs", "update");
    LooseTestDirectory::ExeFS(game / "updates/999999_old/exefs", "invalid update directory");
    LooseTestDirectory::ExeFS(programs / "0/exefs", "program0");
    LooseTestDirectory::ExeFS(programs / "1/exefs", "program1");
    for (const auto& directory : {game, game / "upgrade", programs.parent_path(), programs,
                                  programs / "0", programs / "1"}) {
        REQUIRE(Loader::ResolveLooseProgramPath(directory) == programs / "1/exefs/main");
    }
    REQUIRE(Loader::LooseGameRootFromMain(programs / "1/exefs/main") == game);
    auto main = Core::GetGameFileFromPath(std::make_shared<FileSys::RealVfsFilesystem>(),
                                         Common::FS::PathToUTF8String(game));
    REQUIRE(main);
    CHECK(main->ReadAllBytes() == std::vector<u8>{'p', 'r', 'o', 'g', 'r', 'a', 'm', '1'});
    main.reset();
    fs::remove(programs / "1/exefs/main.npdm");
    CHECK(Loader::ResolveLooseProgramPath(game) == programs / "0/exefs/main");
    fs::remove(programs / "0/exefs/main");
    CHECK(Loader::ResolveLooseProgramPath(game) == game / "updates/65536/exefs/main");
    fs::remove_all(game / "updates");
    CHECK(Loader::ResolveLooseProgramPath(game) == game / "base/exefs/main");
    CHECK(Loader::ResolveLooseProgramPath(game / "base/exefs") == game / "base/exefs/main");
    LooseTestDirectory::Put(game / "main", "direct main");
    CHECK(Loader::ResolveLooseProgramPath(game) == game / "main");
}

TEST_CASE("Loose edition discovery chooses one program in shallow and deep scans",
          "[loose_directory]") {
    LooseTestDirectory t;
    const auto game = t.root / "Game";
    const auto programs = game / "upgrade/327680/program";
    LooseTestDirectory::ExeFS(game / "base/exefs");
    LooseTestDirectory::ExeFS(game / "updates/65536/exefs");
    const auto scan = [](const fs::path& root, bool deep) {
        return GameListDiscovery::Discover(root, deep,
                                           [](const fs::path& file) { return file.filename() == "main"; },
                                           [] { return false; });
    };
    for (const bool deep : {false, true}) {
        REQUIRE(scan(t.root, deep).files.size() == 2);
    }
    LooseTestDirectory::ExeFS(programs / "0/exefs");
    LooseTestDirectory::ExeFS(programs / "1/exefs");
    for (const auto& root : {t.root, game, programs, programs / "0", programs / "0/exefs"}) {
        for (const bool deep : {false, true}) {
            const auto result = scan(root, deep);
            REQUIRE(result.files.size() == 1);
            CHECK(result.files.front() == programs / "1/exefs/main");
        }
    }
    fs::remove(programs / "1/exefs/main.npdm");
    for (const bool deep : {false, true}) {
        const auto fallback = scan(t.root, deep);
        REQUIRE(fallback.files.size() == 1);
        CHECK(fallback.files.front() == programs / "0/exefs/main");
    }
}

TEST_CASE("Loose nested updates select the newest complete program and preserve the game root",
          "[loose_directory]") {
    LooseTestDirectory t;
    const auto game = t.root / fs::path{u8"Xenoblade 日本語"};
    const auto base = game / "base/program/0";
    const auto old = game / "updates/65536/program";
    const auto newest = game / "updates/262144/program";
    LooseTestDirectory::ExeFS(base / "exefs", "base");
    LooseTestDirectory::ExeFS(old / "1/exefs", "old native");
    LooseTestDirectory::ExeFS(newest / "0/exefs", "new legacy");
    LooseTestDirectory::ExeFS(newest / "1/exefs", "new native");
    LooseTestDirectory::ExeFS(game / "updates/999999_old/program/1/exefs", "invalid version");
    for (const auto& directory : {game, newest.parent_path(), newest, newest / "0", newest / "1"}) {
        CHECK(Loader::ResolveLooseProgramPath(directory) == newest / "1/exefs/main");
    }
    for (const auto& package : {base, old / "1", newest / "0", newest / "1"}) {
        CHECK(Loader::LooseGameRootFromMain(package / "exefs/main") == game);
    }
    auto main = Core::GetGameFileFromPath(std::make_shared<FileSys::RealVfsFilesystem>(),
                                         Common::FS::PathToUTF8String(game));
    REQUIRE(main);
    CHECK(main->ReadAllBytes() == std::vector<u8>{'n', 'e', 'w', ' ', 'n', 'a', 't', 'i', 'v', 'e'});
    main.reset();
    fs::remove(newest / "1/exefs/main.npdm");
    CHECK(Loader::ResolveLooseProgramPath(game) == newest / "0/exefs/main");
    fs::remove(newest / "0/exefs/main.npdm");
    CHECK(Loader::ResolveLooseProgramPath(game) == old / "1/exefs/main");
    fs::remove_all(game / "updates");
    CHECK(Loader::ResolveLooseProgramPath(game) == base / "exefs/main");
    CHECK(Loader::ResolveLooseProgramPath(game / "base") == base / "exefs/main");
}

TEST_CASE("Loose nested edition discovery omits sibling and DLC executables",
          "[loose_directory]") {
    LooseTestDirectory t;
    const auto game = t.root / "Game";
    const auto programs = game / "updates/262144/program";
    LooseTestDirectory::ExeFS(game / "base/program/0/exefs");
    LooseTestDirectory::ExeFS(programs / "0/exefs");
    LooseTestDirectory::ExeFS(programs / "1/exefs");
    LooseTestDirectory::ExeFS(game / "dlc/0100ff500e34b001/0/base/program/0/exefs");
    const auto scan = [](const fs::path& root, bool deep) {
        return GameListDiscovery::Discover(root, deep,
                                           [](const fs::path& file) { return file.filename() == "main"; },
                                           [] { return false; });
    };
    for (const auto& root : {t.root, game, programs, programs / "0/exefs"}) {
        for (const bool deep : {false, true}) {
            const auto result = scan(root, deep);
            REQUIRE(result.files.size() == 1);
            CHECK(result.files.front() == programs / "1/exefs/main");
        }
    }
    fs::remove_all(game / "updates");
    for (const bool deep : {false, true}) {
        const auto result = scan(t.root, deep);
        REQUIRE(result.files.size() == 1);
        CHECK(result.files.front() == game / "base/program/0/exefs/main");
    }
}

TEST_CASE("Loose add-on content resolves nested extracted program data", "[loose_directory]") {
    LooseTestDirectory t;
    const auto game = t.root / "Game";
    const auto exefs = game / "updates/262144/program/1/exefs";
    LooseTestDirectory::ExeFS(exefs);
    const auto addon = game / "dlc/0100ff500e34b001";
    SECTION("legacy flat RomFS") {
        LooseTestDirectory::Put(addon / "0/romfs/data.bin", "payload");
    }
    SECTION("nested base program RomFS") {
        LooseTestDirectory::Put(addon / "0/base/program/0/romfs/data.bin", "payload");
    }
    SECTION("nested program RomFS") {
        LooseTestDirectory::Put(addon / "0/program/0/romfs/data.bin", "payload");
    }
    SECTION("base RomFS") {
        LooseTestDirectory::Put(addon / "0/base/romfs/data.bin", "payload");
    }
    SECTION("newest valid version wins") {
        LooseTestDirectory::Put(addon / "0/romfs/data.bin", "old");
        LooseTestDirectory::Put(addon / "65536/base/program/0/romfs/data.bin", "payload");
        LooseTestDirectory::Put(addon / "999999_old/romfs/data.bin", "invalid");
    }
    SECTION("patched add-on replaces its base") {
        LooseTestDirectory::Put(addon / "0/base/program/0/romfs/data.bin", "old");
        LooseTestDirectory::Put(addon / "0/updates/65536/program/0/romfs/data.bin", "payload");
        LooseTestDirectory::Put(addon / "0/updates/x1/program/0/romfs/data.bin", "invalid");
    }
    FileSys::RealVfsFilesystem vfs;
    const auto dir = vfs.OpenDirectory(exefs.string());
    const auto entries = Loader::FindLooseAOC(dir, 0x0100FF500E34A000ULL);
    REQUIRE(entries.size() == 1);
    CHECK(entries.front().first == 0x0100FF500E34B001ULL);
    const auto data = entries.front().second->GetFile("data.bin");
    REQUIRE(data);
    const auto bytes = data->ReadAllBytes();
    CHECK(std::string(bytes.begin(), bytes.end()) == "payload");
    CHECK(Loader::FindLooseAOC(dir, 0x0100123400000000ULL).empty());
}

TEST_CASE("Loose sibling programs resolve by program index", "[loose_directory]") {
    LooseTestDirectory t;
    const auto programs = t.root / "Game/updates/196608/program";
    LooseTestDirectory::ExeFS(programs / "1/exefs");
    LooseTestDirectory::Put(programs / "1/romfs/own.bin", "own");
    LooseTestDirectory::Put(programs / "0/romfs/data.bin", "payload");
    LooseTestDirectory::Put(programs / "2/exefs/main", "no romfs");
    LooseTestDirectory::Put(programs / "old/romfs/data.bin", "invalid");
    FileSys::RealVfsFilesystem vfs;
    const auto entries =
        Loader::FindLooseSiblingPrograms(vfs.OpenDirectory((programs / "1/exefs").string()),
                                         0x0400453019AA8001ULL);
    REQUIRE(entries.size() == 1);
    CHECK(entries.front().first == 0x0400453019AA8000ULL);
    REQUIRE(entries.front().second->GetFile("data.bin"));

    FileSys::SetBootContent(
        {{entries.front().first, FileSys::ContentRecordType::Program, entries.front().second}});
    CHECK(FileSys::OpenBootContent(0x0400453019AA8002ULL, FileSys::ContentRecordType::Program) ==
          nullptr);
    CHECK(FileSys::OpenBootContent(0x0400453019AA8000ULL, FileSys::ContentRecordType::Data) ==
          nullptr);
    CHECK(FileSys::ListBootContent(FileSys::ContentRecordType::Program) ==
          std::vector<u64>{0x0400453019AA8000ULL});
    CHECK(FileSys::ListBootContent(FileSys::ContentRecordType::Data).empty());
    const auto romfs =
        FileSys::OpenBootContent(0x0400453019AA8000ULL, FileSys::ContentRecordType::Program);
    REQUIRE(romfs);
    CHECK(FileSys::OpenBootContent(0x0400453019AA8000ULL, FileSys::ContentRecordType::Program) ==
          romfs);
    const auto extracted = FileSys::ExtractRomFS(romfs);
    REQUIRE(extracted);
    const auto data = extracted->GetFile("data.bin");
    REQUIRE(data);
    const auto bytes = data->ReadAllBytes();
    CHECK(std::string(bytes.begin(), bytes.end()) == "payload");
    FileSys::SetBootContent({});
    CHECK(FileSys::OpenBootContent(0x0400453019AA8000ULL, FileSys::ContentRecordType::Program) ==
          nullptr);

    LooseTestDirectory::ExeFS(t.root / "Flat/exefs");
    CHECK(Loader::FindLooseSiblingPrograms(vfs.OpenDirectory((t.root / "Flat/exefs").string()),
                                           0x0400453019AA8001ULL)
              .empty());

    // No program 0 in the dump: nothing to register, fsp-srv stands the running program in.
    const auto lone = t.root / "Lone/updates/1114112/program";
    LooseTestDirectory::ExeFS(lone / "1/exefs");
    LooseTestDirectory::Put(lone / "1/romfs/dummy.bin", "own");
    CHECK(Loader::FindLooseSiblingPrograms(vfs.OpenDirectory((lone / "1/exefs").string()),
                                           0x0400E95004038001ULL)
              .empty());

    // A running program 0 never points at itself.
    const auto zero = t.root / "Zero/program";
    LooseTestDirectory::ExeFS(zero / "0/exefs");
    LooseTestDirectory::Put(zero / "0/romfs/data.bin", "own");
    CHECK(Loader::FindLooseSiblingPrograms(vfs.OpenDirectory((zero / "0/exefs").string()),
                                           0x0400E95004038000ULL)
              .empty());
}

namespace {
struct NoRomFSLoader final : Loader::AppLoader {
    NoRomFSLoader() : AppLoader(nullptr) {}
    Loader::FileType GetFileType() const override {
        return Loader::FileType::Unknown;
    }
    LoadResult Load(Kernel::KProcess&, Core::System&) override {
        return {Loader::ResultStatus::ErrorNotImplemented, {}};
    }
};
} // namespace

TEST_CASE("RomFS factory serves loose sibling programs by program index", "[loose_directory]") {
    LooseTestDirectory t;
    const auto programs = t.root / "Game/program";
    LooseTestDirectory::ExeFS(programs / "1/exefs");
    LooseTestDirectory::Put(programs / "0/romfs/data.bin", "payload");
    LooseTestDirectory::Put(t.root / "Game/dlc/0100453019AA9001/0/romfs/addon.bin", "addon");
    FileSys::RealVfsFilesystem vfs;
    const auto exefs = vfs.OpenDirectory((programs / "1/exefs").string());
    std::vector<FileSys::BootContent> content;
    for (auto& [id, romfs] : Loader::FindLooseSiblingPrograms(exefs, 0x0400453019AA8001ULL)) {
        content.push_back({id, FileSys::ContentRecordType::Program, std::move(romfs)});
    }
    for (auto& [id, romfs] : Loader::FindLooseAOC(exefs, 0x0400453019AA8001ULL)) {
        content.push_back({id, FileSys::ContentRecordType::Data, std::move(romfs)});
    }
    FileSys::SetBootContent(std::move(content));

    Core::System system;
    NoRomFSLoader loader;
    FileSys::ContentProviderUnion provider;
    for (std::size_t slot = 0; slot < std::size_t(FileSys::ContentProviderUnionSlot::Count); ++slot) {
        provider.SetSlot(FileSys::ContentProviderUnionSlot(slot), nullptr);
    }
    const FileSys::RomFSFactory factory{loader, provider, system.GetFileSystemController()};
    const auto romfs = factory.OpenPatchedRomFSWithProgramIndex(
        0x0400453019AA8001ULL, 0, FileSys::ContentRecordType::Program);
    REQUIRE(romfs);
    const auto extracted = FileSys::ExtractRomFS(romfs);
    REQUIRE(extracted);
    CHECK(extracted->GetFile("data.bin"));
    CHECK(factory.OpenPatchedRomFSWithProgramIndex(0x0400453019AA8001ULL, 0,
                                                   FileSys::ContentRecordType::Data) == nullptr);
    CHECK(factory.OpenPatchedRomFSWithProgramIndex(0x0400453019AA8001ULL, 2,
                                                   FileSys::ContentRecordType::Program) == nullptr);
    // Add-on content is served as Data under the running program's platform byte.
    CHECK(FileSys::ListBootContent(FileSys::ContentRecordType::Data) ==
          std::vector<u64>{0x0400453019AA9001ULL});
    const auto addon = factory.Open(0x0400453019AA9001ULL, FileSys::StorageId::None,
                                    FileSys::ContentRecordType::Data);
    REQUIRE(addon);
    const auto addon_files = FileSys::ExtractRomFS(addon);
    REQUIRE(addon_files);
    CHECK(addon_files->GetFile("addon.bin"));
    FileSys::SetBootContent({});
}

// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <charconv>
#include <cctype>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "common/common_types.h"

namespace Loader {
namespace LooseDirectoryDetail {
namespace fs = std::filesystem;

inline std::string Name(const fs::path& path) {
    const auto utf8 = path.filename().u8string();
    return {utf8.begin(), utf8.end()};
}

inline bool NameIs(const fs::path& path, std::string_view expected) {
    const auto name = Name(path);
    return name.size() == expected.size() &&
           std::equal(name.begin(), name.end(), expected.begin(), [](unsigned char a, char b) {
               return std::tolower(a) == b;
           });
}

inline fs::path Subdirectory(const fs::path& parent, std::string_view name) {
    std::error_code ec;
    if (const auto direct = parent / name; fs::is_directory(direct, ec)) {
        return direct;
    }
    for (fs::directory_iterator it(parent, ec), end; !ec && it != end; it.increment(ec)) {
        std::error_code entry_error;
        if (it->is_directory(entry_error) && NameIs(it->path(), name)) {
            return it->path();
        }
    }
    return {};
}

inline bool IsFile(const fs::path& path) {
    std::error_code ec;
    return fs::is_regular_file(path, ec);
}

inline std::optional<fs::path> PackageMain(const fs::path& package) {
    for (const auto& exefs : {Subdirectory(package, "exefs"), package}) {
        if (!exefs.empty() && IsFile(exefs / "main") && IsFile(exefs / "main.npdm")) {
            return exefs / "main";
        }
    }
    return std::nullopt;
}

inline std::optional<fs::path> ProgramMain(const fs::path& programs) {
    if (programs.empty()) {
        return std::nullopt;
    }
    for (const auto index : {"1", "0"}) {
        if (const auto main = PackageMain(programs / index)) {
            return main;
        }
    }
    return std::nullopt;
}

inline std::vector<fs::path> NewestDirectories(const fs::path& parent) {
    struct Entry {
        fs::path path;
        bool numeric{};
        u64 version{};
    };
    std::vector<Entry> entries;
    std::error_code ec;
    for (fs::directory_iterator it(parent, ec), end; !ec && it != end; it.increment(ec)) {
        std::error_code entry_error;
        if (!it->is_directory(entry_error)) {
            continue;
        }
        const auto name = Name(it->path());
        u64 version{};
        const auto parsed = std::from_chars(name.data(), name.data() + name.size(), version);
        entries.push_back({it->path(), !name.empty() && parsed.ec == std::errc{} &&
                                          parsed.ptr == name.data() + name.size(), version});
    }
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        if (a.numeric != b.numeric) {
            return a.numeric > b.numeric;
        }
        if (a.numeric && a.version != b.version) {
            return a.version > b.version;
        }
        return a.path.filename() > b.path.filename();
    });
    std::vector<fs::path> paths;
    for (auto& entry : entries) {
        paths.push_back(std::move(entry.path));
    }
    return paths;
}
} // namespace LooseDirectoryDetail

// Resolve a game root, upgrade/version folder, program folder or ordinary ExeFS folder.
// Prefer the newest usable edition upgrade, then program 1 over 0 inside that upgrade.
inline std::optional<std::filesystem::path> ResolveLooseProgramPath(
    const std::filesystem::path& directory) {
    using namespace LooseDirectoryDetail;
    if (const auto upgrades = NameIs(directory, "upgrade") ? directory
                                                           : Subdirectory(directory, "upgrade");
        !upgrades.empty()) {
        for (const auto& version : NewestDirectories(upgrades)) {
            if (const auto main = ProgramMain(Subdirectory(version, "program"))) {
                return main;
            }
        }
    }
    if (const auto main = ProgramMain(NameIs(directory, "program")
                                         ? directory
                                         : Subdirectory(directory, "program"))) {
        return main;
    }
    if ((NameIs(directory, "0") || NameIs(directory, "1")) &&
        NameIs(directory.parent_path(), "program")) {
        if (const auto main = ProgramMain(directory.parent_path())) {
            return main;
        }
    }
    // Existing direct main, update, base and ExeFS layouts remain valid.
    if (IsFile(directory / "main")) {
        return directory / "main";
    }
    if (const auto updates = Subdirectory(directory, "updates"); !updates.empty()) {
        for (const auto& version : NewestDirectories(updates)) {
            const auto name = Name(version);
            u64 number{};
            const auto parsed = std::from_chars(name.data(), name.data() + name.size(), number);
            if (name.empty() || parsed.ec != std::errc{} || parsed.ptr != name.data() + name.size()) {
                continue;
            }
            if (const auto main = ProgramMain(Subdirectory(version, "program"))) {
                return main;
            }
            if (const auto main = PackageMain(version)) {
                return main;
            }
        }
    }
    if (const auto base = Subdirectory(directory, "base"); !base.empty()) {
        if (const auto main = ProgramMain(Subdirectory(base, "program"))) {
            return main;
        }
        if (const auto main = PackageMain(base)) {
            return main;
        }
    }
    return PackageMain(directory);
}

// Keep DLC/sidecars associated with the game root when a selected main is nested deeply.
inline std::optional<std::filesystem::path> LooseGameRootFromMain(
    const std::filesystem::path& main) {
    using namespace LooseDirectoryDetail;
    if (!NameIs(main, "main")) {
        return std::nullopt;
    }
    auto package = main.parent_path();
    if (NameIs(package, "exefs")) {
        package = package.parent_path();
    }
    if ((NameIs(package, "0") || NameIs(package, "1")) &&
        NameIs(package.parent_path(), "program")) {
        auto root = package.parent_path().parent_path();
        if (NameIs(root.parent_path(), "upgrade") || NameIs(root.parent_path(), "updates")) {
            root = root.parent_path().parent_path();
        } else if (NameIs(root, "base")) {
            root = root.parent_path();
        }
        return root;
    }
    if (NameIs(package, "base")) {
        return package.parent_path();
    }
    if (NameIs(package.parent_path(), "updates")) {
        return package.parent_path().parent_path();
    }
    return package;
}

} // namespace Loader

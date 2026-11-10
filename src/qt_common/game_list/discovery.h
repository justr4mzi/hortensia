// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/loader/loose_directory.h"

namespace GameListDiscovery {

struct Result {
    std::vector<std::filesystem::path> files;
    std::vector<std::filesystem::path> directories;
    std::size_t skipped_romfs{};
    std::size_t errors{};
};

inline bool IsExeFS(const std::filesystem::path& path) {
    std::error_code ec;
    return std::filesystem::is_regular_file(path / "main", ec) &&
           std::filesystem::is_regular_file(path / "main.npdm", ec);
}

// Only edition layouts collapse sibling executables; legacy base/update entries stay distinct.
inline bool IsUpgradeProgramMain(const std::filesystem::path& main) {
    const auto equals = [](const std::filesystem::path& path, std::u8string_view expected) {
        auto name = path.filename().u8string();
        std::transform(name.begin(), name.end(), name.begin(), [](char8_t c) {
            return static_cast<char8_t>(std::tolower(static_cast<unsigned char>(c)));
        });
        return name == expected;
    };
    const auto package = main.parent_path().parent_path();
    if (!equals(main, u8"main") || !equals(main.parent_path(), u8"exefs") ||
        !(equals(package, u8"0") || equals(package, u8"1")) ||
        !equals(package.parent_path(), u8"program")) {
        return false;
    }
    const auto storage = package.parent_path().parent_path();
    if (equals(storage, u8"base") || equals(storage.parent_path(), u8"upgrade")) {
        return true;
    }
    const auto name = Loader::LooseDirectoryDetail::Name(storage);
    u64 version{};
    const auto parsed = std::from_chars(name.data(), name.data() + name.size(), version);
    return equals(storage.parent_path(), u8"updates") && !name.empty() &&
           parsed.ec == std::errc{} && parsed.ptr == name.data() + name.size();
}

template <typename Supported, typename Stopped>
Result Discover(const std::filesystem::path& root, bool deep, Supported supported,
                Stopped stopped) {
    namespace fs = std::filesystem;
    Result result;
    std::unordered_set<fs::path> files;
    std::unordered_set<fs::path> directories;
    const auto watch = [&](const fs::path& path) {
        if (directories.insert(path).second) {
            result.directories.push_back(path);
        }
    };
    const auto add_file = [&](const fs::path& path) {
        fs::path chosen = path;
        if (path.filename() == "main") {
            if (const auto game_root = Loader::LooseGameRootFromMain(path)) {
                if (const auto preferred = Loader::ResolveLooseProgramPath(*game_root);
                    preferred && IsUpgradeProgramMain(*preferred)) {
                    chosen = *preferred;
                    // Watch the storage levels too, so a newly added program is discovered.
                    for (auto directory = chosen.parent_path(); !directory.empty();
                         directory = directory.parent_path()) {
                        watch(directory);
                        if (directory == *game_root || directory == directory.parent_path()) {
                            break;
                        }
                    }
                }
            }
        }
        if (files.insert(chosen).second) {
            result.files.push_back(std::move(chosen));
        }
    };
    const auto probe = [&](const fs::path& path) {
        if (IsExeFS(path)) {
            watch(path);
            add_file(path / "main");
        }
    };
    const auto probe_layouts = [&](const fs::path& path) {
        if (const auto preferred = Loader::ResolveLooseProgramPath(path);
            preferred && IsUpgradeProgramMain(*preferred)) {
            probe(preferred->parent_path());
            return;
        }
        for (const auto& content : {path, path / "base"}) {
            probe(content / "exefs");
            const auto updates = content / "updates";
            std::error_code ec;
            if (!fs::is_directory(updates, ec)) {
                continue;
            }
            watch(updates);
            for (fs::directory_iterator it{updates, fs::directory_options::skip_permission_denied,
                                           ec};
                 !ec && it != fs::directory_iterator{} && !stopped(); it.increment(ec)) {
                probe(it->path() / "exefs");
            }
        }
    };

    std::vector<std::pair<fs::path, unsigned>> pending{{root, 0}};
    while (!pending.empty() && !stopped()) {
        auto [directory, depth] = std::move(pending.back());
        pending.pop_back();
        watch(directory);
        if (IsExeFS(directory)) {
            add_file(directory / "main");
            continue;
        }
        if (!deep) {
            probe_layouts(directory);
        }
        std::error_code ec;
        for (fs::directory_iterator it{directory, fs::directory_options::skip_permission_denied,
                                       ec};
             !ec && it != fs::directory_iterator{} && !stopped(); it.increment(ec)) {
            const auto& entry = *it;
            std::error_code entry_ec;
            if (entry.is_directory(entry_ec)) {
                if (entry.is_symlink(entry_ec)) {
                    continue;
                }
                auto name = entry.path().filename().u8string();
                std::transform(name.begin(), name.end(), name.begin(), [](char8_t c) {
                    return static_cast<char8_t>(std::tolower(static_cast<unsigned char>(c)));
                });
                if (name == u8"romfs" && IsExeFS(directory / "exefs")) {
                    ++result.skipped_romfs;
                    continue;
                }
                if (name == u8"dlc" && Loader::ResolveLooseProgramPath(directory)) {
                    continue;
                }
                if (deep || depth == 0) {
                    pending.emplace_back(entry.path(), depth + 1);
                }
            } else if (entry.is_regular_file(entry_ec) && depth == 0 && !deep) {
                if (supported(entry.path())) {
                    add_file(entry.path());
                }
            } else if (entry.is_regular_file(entry_ec) && deep && supported(entry.path())) {
                add_file(entry.path());
            }
        }
        result.errors += ec ? 1 : 0;
    }
    return result;
}

} // namespace GameListDiscovery

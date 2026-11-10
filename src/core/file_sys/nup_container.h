// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "common/common_types.h"
#include "core/file_sys/vfs/vfs_types.h"
#include "core/file_sys/vfs/vfs_vector.h"
#include "core/loader/loader.h"

namespace FileSys {

struct NupDecompressCache;

/**
 * NUP (NX Package) container: stores loose ExeFS/RomFS trees for a base program,
 * its updates and add-on content in one file. Layout:
 *
 *   Superblock (0x80 bytes, magic "NUP\x01")
 *   Table of Contents (packages, dir/file nodes, children array, string pool)
 *   Blob region (per-file extents, raw or zstd codec, 16-byte aligned)
 *
 * Per-package meta TLVs carry NPDM bytes, title names (default + localized),
 * icons and a display version. All integers little-endian.
 */
class NupContainer {
public:
    static constexpr std::array<u8, 4> HeaderMagic{'N', 'U', 'P', 0x01};
    /// Memory budget for decoded zstd members.
    static constexpr std::size_t DefaultCacheCap = 512 * 1024 * 1024;

    enum class PackageKind : u8 {
        Base = 0,
        Update = 1,
        Addon = 2,
    };


    struct PackageInfo {
        u64 title_id{};
        u32 version{};
        PackageKind kind{};
        bool standalone_update{};
        u32 root_dir{};
        u32 exefs_dir{};
        u32 romfs_dir{};
        u64 meta_offset{};
        u32 meta_size{};

        std::vector<u8> npdm;
        /// The dump's control.nacp as packed, when it had one.
        std::vector<u8> nacp;
        std::string title_name;
        std::string publisher;
        std::string source_dump;
        std::vector<std::pair<std::string, std::string>> localized_titles;
        std::vector<u8> icon;
        std::vector<std::pair<std::string, std::vector<u8>>> localized_icons;
        std::string display_version;
    };

    /// Quick magic check without a full parse.
    static bool IsNupFile(const VirtualFile& file);

    /// Recursively merges overlay into a copy of base; overlay paths win.
    static VirtualDir MergeShadowDirs(const VirtualDir& base, const VirtualDir& overlay);

    /// Zstd members larger than half of `memory_cache_cap` are decompressed to a temporary file
    /// in `spill_dir` (the cache directory by default) instead of being held in memory.
    explicit NupContainer(VirtualFile file_, std::size_t memory_cache_cap = DefaultCacheCap,
                          std::filesystem::path spill_dir_ = {});
    ~NupContainer();

    Loader::ResultStatus GetStatus() const {
        return status;
    }

    const std::vector<PackageInfo>& GetPackages() const {
        return packages;
    }

    std::optional<std::size_t> FindPackage(PackageKind kind,
                                           std::optional<u64> title_id = std::nullopt) const;
    std::optional<std::size_t> FindBasePackage() const;
    /// Highest version among Update packages, if any.
    std::optional<std::size_t> FindBestUpdatePackage() const;

    /// The package that carries the application's name, icon and control data: the base, else
    /// the newest update (the one a container without a base boots), else the first package.
    /// The packer writes application metadata there; the loader and the editor read it there.
    static std::size_t MetadataPackageIndex(
        std::span<const std::pair<PackageKind, u32>> kinds_and_versions);
    std::size_t FindMetadataPackage() const;

    /// Whether a member name is one plain file name: not empty, "." or "..", and without
    /// separators, drive or stream colons or NULs. Names are labels, never paths: Parse rejects
    /// any other, so nothing that joins them onto a host folder can leave it.
    static bool IsPlainEntryName(std::string_view name);

    /// Opens a member file by node index; raw extents are zero-copy views into
    /// the container, zstd extents are decompressed only as far as reads reach,
    /// cached in memory or, when large, in a temporary file.
    VirtualFile OpenFile(std::size_t file_index);

    enum class StreamResult { Success, Corrupt, Stopped };

    /// Hands a member file to `sink` in order, decompressing a zstd extent as it goes (nothing is
    /// cached or written to a temporary file), and checks the result against the stored size and
    /// SHA-256. On Corrupt, whatever `sink` received must be discarded. Stopped: `sink` returned
    /// false.
    StreamResult StreamFile(std::size_t file_index,
                            const std::function<bool(std::span<const u8>)>& sink) const;

    /// A directory's entries as stored, for copying a package out without building its tree:
    /// subdirectory indices and member file indices.
    struct DirEntries {
        std::vector<u32> subdirs;
        std::vector<std::size_t> file_indices;
    };
    DirEntries ListDir(u32 dir_index) const;
    const std::string& GetDirName(u32 dir_index) const;
    const std::string& GetFileName(std::size_t file_index) const;
    u64 GetFileSize(std::size_t file_index) const;

    VirtualDir GetExeFSDir(std::size_t package_index);
    VirtualDir GetRomFSDir(std::size_t package_index);
    VirtualDir GetPackageDir(std::size_t package_index);

    std::vector<std::size_t> FindAddonPackages(u64 program_id) const;

    /// Newest usable add-on RomFS per title belonging to this application.
    std::vector<std::pair<u64, VirtualDir>> GetAddonRomFS(u64 program_id);

    /// RomFS of the other programs of a multi-program application: the base packages after the
    /// first, keyed by their program-index title ID.
    std::vector<std::pair<u64, VirtualDir>> GetSiblingProgramRomFS();

    /// Stream-hashes every member file against its stored SHA-256.
    Loader::ResultStatus VerifyIntegrity(
        std::function<bool(std::size_t, std::size_t)> progress_callback) const;

    std::size_t GetFileCount() const {
        return files.size();
    }

    bool IsZstdMember(std::size_t file_index) const {
        return file_index < files.size() && files[file_index].size != 0 &&
               files[file_index].codec != 0;
    }

private:
    struct DirNode {
        std::string name;
        u32 first_child{};
        u32 child_count{};
    };

    struct FileNode {
        std::string name;
        u64 size{};
        std::array<u8, 0x20> sha256{};
        u64 extent_offset{};
        u64 extent_size{};
        u8 codec{};
    };

    bool Parse();
    bool ParseMeta(PackageInfo& package);
    bool ValidateDirTree() const;
    void BuildDirTree(u32 dir_index, VectorVfsDirectory& out);
    VirtualDir BuildPackageDir(u32 dir_index);

    VirtualFile file;
    Loader::ResultStatus status{Loader::ResultStatus::ErrorNullFile};

    u64 toc_offset{};
    u64 toc_size{};
    u64 blob_offset{};
    u64 file_size{};

    std::vector<DirNode> dirs;
    std::vector<FileNode> files;
    std::vector<u32> children;
    std::vector<PackageInfo> packages;

    std::vector<VirtualDir> exefs_dir_cache;
    std::vector<VirtualDir> romfs_dir_cache;

    std::shared_ptr<NupDecompressCache> cache;
};

} // namespace FileSys

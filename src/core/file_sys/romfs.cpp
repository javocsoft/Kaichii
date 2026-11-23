// SPDX-FileCopyrightText: Copyright 2025 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "common/common_types.h"
#include "common/logging.h"
#include "common/string_util.h"
#include "common/swap.h"
#include "core/file_sys/fsmitm_romfsbuild.h"
#include "core/file_sys/romfs.h"
#include "core/file_sys/vfs/vfs.h"
#include "core/file_sys/vfs/vfs_cached.h"
#include "core/file_sys/vfs/vfs_concat.h"
#include "core/file_sys/vfs/vfs_offset.h"
#include "core/file_sys/vfs/vfs_vector.h"

namespace FileSys {
namespace {
constexpr u32 ROMFS_ENTRY_EMPTY = 0xFFFFFFFF;

// Paths are limited to 0x301 bytes, and each level of a path takes at least two of them.
constexpr u32 ROMFS_MAX_DEPTH = 0x301 / 2;

struct TableLocation {
    u64_le offset;
    u64_le size;
};
static_assert(sizeof(TableLocation) == 0x10, "TableLocation has incorrect size.");

struct RomFSHeader {
    u64_le header_size;
    TableLocation directory_hash;
    TableLocation directory_meta;
    TableLocation file_hash;
    TableLocation file_meta;
    u64_le data_offset;
};
static_assert(sizeof(RomFSHeader) == 0x50, "RomFSHeader has incorrect size.");

struct DirectoryEntry {
    u32_le parent;
    u32_le sibling;
    u32_le child_dir;
    u32_le child_file;
    u32_le hash;
    u32_le name_length;
};
static_assert(sizeof(DirectoryEntry) == 0x18, "DirectoryEntry has incorrect size.");

struct FileEntry {
    u32_le parent;
    u32_le sibling;
    u64_le offset;
    u64_le size;
    u32_le hash;
    u32_le name_length;
};
static_assert(sizeof(FileEntry) == 0x20, "FileEntry has incorrect size.");

struct RomFSTraversalContext {
    RomFSHeader header;
    VirtualFile file;
    std::vector<u8> directory_meta;
    std::vector<u8> file_meta;
    // The most entries the tables can hold. Running out means the links form a cycle.
    size_t directories_left;
    size_t files_left;
    bool corrupt;
};

template <typename EntryType, auto Member>
std::optional<std::pair<EntryType, std::string>> GetEntry(const RomFSTraversalContext& ctx,
                                                          size_t offset) {
    const size_t entry_end = offset + sizeof(EntryType);
    const std::vector<u8>& vec = ctx.*Member;
    const size_t size = vec.size();
    const u8* data = vec.data();
    EntryType entry{};

    if (entry_end > size) {
        return std::nullopt;
    }
    std::memcpy(&entry, data + offset, sizeof(EntryType));

    const size_t name_length = (std::min)(entry_end + entry.name_length, size) - entry_end;
    std::string name(reinterpret_cast<const char*>(data + entry_end), name_length);

    return std::pair{entry, std::move(name)};
}

std::optional<std::pair<DirectoryEntry, std::string>> GetDirectoryEntry(
    const RomFSTraversalContext& ctx, size_t directory_offset) {
    return GetEntry<DirectoryEntry, &RomFSTraversalContext::directory_meta>(ctx, directory_offset);
}

std::optional<std::pair<FileEntry, std::string>> GetFileEntry(const RomFSTraversalContext& ctx,
                                                              size_t file_offset) {
    return GetEntry<FileEntry, &RomFSTraversalContext::file_meta>(ctx, file_offset);
}

void ProcessFile(RomFSTraversalContext& ctx, u32 this_file_offset,
                 std::shared_ptr<VectorVfsDirectory>& parent) {
    while (this_file_offset != ROMFS_ENTRY_EMPTY) {
        auto entry = GetFileEntry(ctx, this_file_offset);
        if (!entry || ctx.files_left == 0) {
            ctx.corrupt = true;
            return;
        }
        --ctx.files_left;

        parent->AddFile(std::make_shared<OffsetVfsFile>(
            ctx.file, entry->first.size, entry->first.offset + ctx.header.data_offset,
            std::move(entry->second)));

        this_file_offset = entry->first.sibling;
    }
}

void ProcessDirectory(RomFSTraversalContext& ctx, u32 this_dir_offset,
                      std::shared_ptr<VectorVfsDirectory>& parent, u32 depth) {
    while (this_dir_offset != ROMFS_ENTRY_EMPTY) {
        auto entry = GetDirectoryEntry(ctx, this_dir_offset);
        if (!entry || ctx.directories_left == 0 || depth > ROMFS_MAX_DEPTH) {
            ctx.corrupt = true;
            return;
        }
        --ctx.directories_left;

        auto current = std::make_shared<VectorVfsDirectory>(
            std::vector<VirtualFile>{}, std::vector<VirtualDir>{}, entry->second);

        if (entry->first.child_file != ROMFS_ENTRY_EMPTY) {
            ProcessFile(ctx, entry->first.child_file, current);
        }

        if (entry->first.child_dir != ROMFS_ENTRY_EMPTY) {
            ProcessDirectory(ctx, entry->first.child_dir, current, depth + 1);
        }

        if (ctx.corrupt) {
            return;
        }

        parent->AddDirectory(current);
        this_dir_offset = entry->first.sibling;
    }
}
struct WatchedRange {
    u64 begin;
    u64 end;
    std::string path;
};

void CollectWatchedRanges(const VirtualDir& dir, const std::string& prefix,
                          const RomFSReadObserver& observer, std::vector<WatchedRange>& out) {
    for (const auto& file : dir->GetFiles()) {
        auto path = prefix + "/" + file->GetName();
        // ExtractRomFS makes every file an OffsetVfsFile into the image.
        if (file->GetSize() != 0 && observer.accepts(path)) {
            const u64 begin = static_cast<const OffsetVfsFile*>(file.get())->GetOffset();
            out.push_back({begin, begin + file->GetSize(), std::move(path)});
        }
    }
    for (const auto& subdir : dir->GetSubdirectories()) {
        CollectWatchedRanges(subdir, prefix + "/" + subdir->GetName(), observer, out);
    }
}

// A RomFS image whose reads report the watched files they touch. Games read the image as one
// storage, so the files are found by offset.
class WatchedRomFSFile final : public VfsFile {
public:
    WatchedRomFSFile(VirtualFile source_, std::vector<WatchedRange> ranges_,
                     std::function<void(std::string_view)> on_read_)
        : source{std::move(source_)}, ranges{std::move(ranges_)}, on_read{std::move(on_read_)} {}

    std::string GetName() const override { return source->GetName(); }
    std::size_t GetSize() const override { return source->GetSize(); }
    bool Resize(std::size_t size) override { return source->Resize(size); }
    VirtualDir GetContainingDirectory() const override { return source->GetContainingDirectory(); }
    bool IsWritable() const override { return source->IsWritable(); }
    bool IsReadable() const override { return source->IsReadable(); }
    std::size_t Read(u8* data, std::size_t size, std::size_t offset) const override {
        const auto count = source->Read(data, size, offset);
        if (count == 0) {
            return count;
        }
        const u64 end = offset + count;
        auto it = std::upper_bound(ranges.begin(), ranges.end(), u64{offset},
                                   [](u64 value, const WatchedRange& range) {
                                       return value < range.begin;
                                   });
        if (it != ranges.begin()) {
            --it;
        }
        for (; it != ranges.end() && it->begin < end; ++it) {
            if (it->end > offset) {
                on_read(it->path);
            }
        }
        return count;
    }
    std::size_t Write(const u8* data, std::size_t size, std::size_t offset) override {
        return source->Write(data, size, offset);
    }
    bool Rename(std::string_view name) override { return source->Rename(name); }

private:
    VirtualFile source;
    std::vector<WatchedRange> ranges;
    std::function<void(std::string_view)> on_read;
};

} // Anonymous namespace

VirtualDir ExtractRomFS(const VirtualFile& file) {
    auto root_container = std::make_shared<VectorVfsDirectory>();
    if (!file) {
        return root_container;
    }

    RomFSTraversalContext ctx{};

    if (file->ReadObject(&ctx.header) != sizeof(RomFSHeader)) {
        return nullptr;
    }

    if (ctx.header.header_size != sizeof(RomFSHeader)) {
        return nullptr;
    }

    ctx.file = file;
    ctx.directory_meta =
        file->ReadBytes(ctx.header.directory_meta.size, ctx.header.directory_meta.offset);
    ctx.file_meta = file->ReadBytes(ctx.header.file_meta.size, ctx.header.file_meta.offset);
    ctx.directories_left = ctx.directory_meta.size() / sizeof(DirectoryEntry);
    ctx.files_left = ctx.file_meta.size() / sizeof(FileEntry);

    ProcessDirectory(ctx, 0, root_container, 0);

    if (ctx.corrupt) {
        LOG_ERROR(Loader, "RomFS {} has an invalid directory tree", file->GetName());
        return nullptr;
    }

    return root_container->GetSubdirectory("");
}

VirtualFile CreateRomFS(const VirtualDir& dir, VirtualDir ext, RomFSReadObserver observer) {
    if (dir == nullptr)
        return nullptr;

    RomFSBuildContext ctx{dir, std::move(ext)};
    return ConcatenatedVfsFile::MakeConcatenatedFile(0, dir->GetName(), ctx.Build(observer));
}

VirtualFile WatchRomFSReads(VirtualFile romfs, const RomFSReadObserver& observer) {
    if (!romfs || !observer.accepts || !observer.on_read) {
        return romfs;
    }
    const auto root = ExtractRomFS(romfs);
    if (!root) {
        return romfs;
    }
    std::vector<WatchedRange> ranges;
    CollectWatchedRanges(root, {}, observer, ranges);
    if (ranges.empty()) {
        return romfs;
    }
    std::ranges::sort(ranges, {}, &WatchedRange::begin);
    return std::make_shared<WatchedRomFSFile>(std::move(romfs), std::move(ranges),
                                              observer.on_read);
}

} // namespace FileSys

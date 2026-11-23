// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/file_sys/romfs.h"
#include "core/file_sys/vfs/vfs_vector.h"
#include "core/loader/companion_scenes.h"
#include "core/loader/loader.h"

namespace {

using FileSys::VectorVfsDirectory;
using FileSys::VectorVfsFile;
using FileSys::VirtualDir;
using FileSys::VirtualFile;

constexpr std::size_t RomFSHeaderSize = 0x50;
constexpr std::size_t DirectoryEntrySize = 0x18;
constexpr std::size_t FileEntrySize = 0x20;
constexpr u32 NoEntry = 0xFFFFFFFF;

struct DirectoryLinks {
    u32 sibling = NoEntry;
    u32 child_dir = NoEntry;
    u32 child_file = NoEntry;
};

template <typename T>
void Put(std::vector<u8>& bytes, std::size_t offset, T value) {
    std::memcpy(bytes.data() + offset, &value, sizeof(T));
}

/// Hand-built tables, for shapes the builder cannot produce. Entries have empty names, so each one
/// is its index times the entry size into its table. `siblings` links the files.
std::vector<u8> MakeTables(const std::vector<DirectoryLinks>& directories,
                           const std::vector<u32>& siblings) {
    const std::size_t directory_size = directories.size() * DirectoryEntrySize;
    const std::size_t file_size = siblings.size() * FileEntrySize;
    std::vector<u8> bytes(RomFSHeaderSize + directory_size + file_size);

    Put<u64>(bytes, 0x00, RomFSHeaderSize);
    Put<u64>(bytes, 0x18, RomFSHeaderSize);
    Put<u64>(bytes, 0x20, directory_size);
    Put<u64>(bytes, 0x38, RomFSHeaderSize + directory_size);
    Put<u64>(bytes, 0x40, file_size);
    Put<u64>(bytes, 0x48, bytes.size());

    for (std::size_t i = 0; i < directories.size(); ++i) {
        const std::size_t entry = RomFSHeaderSize + i * DirectoryEntrySize;
        Put<u32>(bytes, entry + 0x4, directories[i].sibling);
        Put<u32>(bytes, entry + 0x8, directories[i].child_dir);
        Put<u32>(bytes, entry + 0xC, directories[i].child_file);
    }
    for (std::size_t i = 0; i < siblings.size(); ++i) {
        Put<u32>(bytes, RomFSHeaderSize + directory_size + i * FileEntrySize + 0x4, siblings[i]);
    }
    return bytes;
}

/// A line of directories, each the only child of the one before it.
std::vector<DirectoryLinks> Chain(std::size_t length) {
    std::vector<DirectoryLinks> directories(length);
    for (std::size_t i = 0; i + 1 < length; ++i) {
        directories[i].child_dir = static_cast<u32>((i + 1) * DirectoryEntrySize);
    }
    return directories;
}

VirtualFile File(std::string name, std::vector<u8> data) {
    return std::make_shared<VectorVfsFile>(std::move(data), std::move(name));
}

VirtualDir Dir(std::string name, std::vector<VirtualFile> files, std::vector<VirtualDir> dirs = {}) {
    return std::make_shared<VectorVfsDirectory>(std::move(files), std::move(dirs), std::move(name));
}

std::vector<u8> Pattern(std::size_t size) {
    std::vector<u8> data(size);
    for (std::size_t i = 0; i < size; ++i) {
        data[i] = static_cast<u8>(i * 31);
    }
    return data;
}

} // namespace

TEST_CASE("Ocarina scene is recognized from scene and baked tile reads", "[core][romfs]") {
    using Loader::OcarinaSceneFromRomFSPath;

    CHECK(OcarinaSceneFromRomFSPath(
              "/Banc/GameBancParam/LostWood_Child.game__banc__GameBancParam.bgyml") ==
          "LostWood_Child");
    CHECK(OcarinaSceneFromRomFSPath("/Bake/Scene/KokiriForest_Child_G_3_2.bkres.zs") ==
          "KokiriForest_Child");
    CHECK(OcarinaSceneFromRomFSPath("/Bake/Scene/LostWood_Child_G_0_1.bkres.zs") ==
          "LostWood_Child");
    CHECK(OcarinaSceneFromRomFSPath("/Tex/RBake_Scene_KokiriForest_Adult_G_3_2_d0.bntx.zs")
              .empty());
    CHECK(OcarinaSceneFromRomFSPath("/Bake/Scene/LostWood_Child_G_.bkres.zs").empty());
    CHECK(OcarinaSceneFromRomFSPath("/Bake/Scene/LostWood_Child_G_x_1.bkres.zs").empty());
    CHECK(OcarinaSceneFromRomFSPath("/Bake/Scene/Other/LostWood_Child_G_0_1.bkres.zs")
              .empty());
}

TEST_CASE("Breath of the Wild scene is recognized from shrine, beast and map reads",
          "[core][romfs]") {
    using Loader::BreathOfTheWildSceneFromRomFSPath;

    CHECK(BreathOfTheWildSceneFromRomFSPath("/Pack/Dungeon005.pack") == "Dungeon005");
    CHECK(BreathOfTheWildSceneFromRomFSPath("/Pack/Dungeon119.pack") == "Dungeon119");
    CHECK(BreathOfTheWildSceneFromRomFSPath("/Pack/RemainsWind.pack") == "RemainsWind");
    CHECK(BreathOfTheWildSceneFromRomFSPath("/Pack/RemainsElectric.pack") == "RemainsElectric");
    CHECK(BreathOfTheWildSceneFromRomFSPath("/Map/MainField/D-6/D-6_Dynamic.smubin") ==
          "MainField");
    CHECK(BreathOfTheWildSceneFromRomFSPath("/Pack/Bootup.pack").empty());  // codespell:ignore bootup
    CHECK(BreathOfTheWildSceneFromRomFSPath("/Pack/TitleBG.pack").empty());
    CHECK(BreathOfTheWildSceneFromRomFSPath("/Pack/Dungeon5.pack").empty());
    CHECK(BreathOfTheWildSceneFromRomFSPath("/Pack/DungeonABC.pack").empty());
    CHECK(BreathOfTheWildSceneFromRomFSPath("/Pack/Dungeon005.pack.bak").empty());
    CHECK(BreathOfTheWildSceneFromRomFSPath("/Actor/Pack/Dungeon005.pack").empty());
    CHECK(BreathOfTheWildSceneFromRomFSPath("/NavMesh/MainField/16-23.shknm2").empty());
}

TEST_CASE("Companion scenes are watched only for games with a companion", "[core][romfs]") {
    const auto tracker = std::make_shared<Loader::SceneTracker>();
    CHECK_FALSE(Loader::CompanionSceneObserver(0x0100000000000000ULL, tracker).accepts);
    const auto observer = Loader::CompanionSceneObserver(Loader::BreathOfTheWildProgramId, tracker);
    REQUIRE(observer.accepts);
    CHECK(observer.accepts("/Pack/RemainsFire.pack"));
    CHECK_FALSE(observer.accepts("/Pack/Bootup.pack"));  // codespell:ignore bootup
    observer.on_read("/Pack/RemainsFire.pack");
    CHECK(tracker->Last() == "RemainsFire");
    observer.on_read("/Map/MainField/E-6/E-6_Dynamic.smubin");
    CHECK(tracker->Last() == "MainField");
}

TEST_CASE("RomFS read watcher finds the files a packed image's reads touch", "[core][romfs]") {
    const auto base = Dir("", {File("other.bin", Pattern(0x40))},
                          {Dir("Pack", {File("Dungeon005.pack", Pattern(0x300)),
                                        File("RemainsWind.pack", Pattern(0x200)),
                                        File("Bootup.pack", Pattern(0x100))})});  // codespell:ignore bootup
    const auto image = FileSys::CreateRomFS(base);
    REQUIRE(image != nullptr);
    std::vector<std::string> observed;
    FileSys::RomFSReadObserver observer{
        .accepts = [](std::string_view path) {
            return path == "/Pack/Dungeon005.pack" || path == "/Pack/RemainsWind.pack";
        },
        .on_read = [&observed](std::string_view path) { observed.emplace_back(path); },
    };
    const auto watched = FileSys::WatchRomFSReads(image, observer);
    REQUIRE(watched != image);
    CHECK(watched->GetSize() == image->GetSize());

    // The game parses the tables itself, then reads files by offset.
    const auto root = FileSys::ExtractRomFS(watched);
    REQUIRE(root != nullptr);
    CHECK(observed.empty());
    const auto pack = root->GetSubdirectory("Pack");
    CHECK(pack->GetFile("Bootup.pack")->ReadAllBytes() == Pattern(0x100));  // codespell:ignore bootup
    CHECK(root->GetFile("other.bin")->ReadAllBytes() == Pattern(0x40));
    CHECK(observed.empty());
    std::vector<u8> middle(0x10);
    CHECK(pack->GetFile("Dungeon005.pack")->Read(middle.data(), middle.size(), 0x150) == 0x10);
    REQUIRE(observed.size() == 1);
    CHECK(observed.front() == "/Pack/Dungeon005.pack");
    CHECK(pack->GetFile("RemainsWind.pack")->ReadAllBytes() == Pattern(0x200));
    REQUIRE(observed.size() == 2);
    CHECK(observed.back() == "/Pack/RemainsWind.pack");

    // One read across the whole image touches both watched files, in order.
    observed.clear();
    CHECK(watched->ReadAllBytes() == image->ReadAllBytes());
    CHECK(observed.size() == 2);
}

TEST_CASE("RomFS read watcher leaves an image alone when no file is watched", "[core][romfs]") {
    const auto image = FileSys::CreateRomFS(Dir("", {File("a.bin", {1, 2, 3})}));
    REQUIRE(image != nullptr);
    FileSys::RomFSReadObserver none{
        .accepts = [](std::string_view) { return false; },
        .on_read = [](std::string_view) {},
    };
    CHECK(FileSys::WatchRomFSReads(image, none) == image);
    CHECK(FileSys::WatchRomFSReads(image, {}) == image);
    CHECK(FileSys::WatchRomFSReads(nullptr, none) == nullptr);
}

TEST_CASE("RomFS keeps a tree through building and extraction", "[core][romfs]") {
    // Larger than the builder's 16-byte alignment and block padding, on purpose.
    const auto big = Pattern(0x12345);
    const auto base = Dir("", {File("root.bin", {1, 2, 3}), File("big.bin", big)},
                          {Dir("sub", {File("a.txt", {4})}, {Dir("deep", {File("b.txt", {5, 6})})}),
                           Dir("other", {File("c.txt", {7})})});

    const auto romfs = FileSys::CreateRomFS(base, nullptr);
    REQUIRE(romfs != nullptr);
    const auto root = FileSys::ExtractRomFS(romfs);
    REQUIRE(root != nullptr);

    CHECK(root->GetFiles().size() == 2);
    CHECK(root->GetSubdirectories().size() == 2);
    CHECK(root->GetFile("root.bin")->ReadAllBytes() == std::vector<u8>{1, 2, 3});
    CHECK(root->GetFile("big.bin")->ReadAllBytes() == big);

    const auto sub = root->GetSubdirectory("sub");
    REQUIRE(sub != nullptr);
    CHECK(sub->GetFile("a.txt")->ReadAllBytes() == std::vector<u8>{4});
    const auto deep = sub->GetSubdirectory("deep");
    REQUIRE(deep != nullptr);
    CHECK(deep->GetFile("b.txt")->ReadAllBytes() == std::vector<u8>{5, 6});
    CHECK(root->GetSubdirectory("other")->GetFile("c.txt")->ReadAllBytes() == std::vector<u8>{7});
}

TEST_CASE("RomFS keeps an empty file", "[core][romfs]") {
    const auto root = FileSys::ExtractRomFS(
        FileSys::CreateRomFS(Dir("", {File("empty", {}), File("after", {9})}), nullptr));
    REQUIRE(root != nullptr);
    REQUIRE(root->GetFile("empty") != nullptr);
    CHECK(root->GetFile("empty")->GetSize() == 0);
    CHECK(root->GetFile("after")->ReadAllBytes() == std::vector<u8>{9});
}

TEST_CASE("RomFS observer reports only reads of selected payloads", "[core][romfs]") {
    const auto base = Dir("", {}, {Dir("Banc", {}, {Dir("GameBancParam", {
        File("KokiriForest_Child.game__banc__GameBancParam.bgyml", {1, 2, 3}),
        File("prefetched.bin", {4, 5})})})});
    std::vector<std::string> observed;
    FileSys::RomFSReadObserver observer{
        .accepts = [](std::string_view path) {
            return path.ends_with(".game__banc__GameBancParam.bgyml");
        },
        .on_read = [&observed](std::string_view path) { observed.emplace_back(path); },
    };
    const auto root = FileSys::ExtractRomFS(FileSys::CreateRomFS(base, nullptr, observer));
    REQUIRE(root != nullptr);
    CHECK(observed.empty());
    const auto param = root->GetSubdirectory("Banc")
                           ->GetSubdirectory("GameBancParam")
                           ->GetFile("KokiriForest_Child.game__banc__GameBancParam.bgyml");
    CHECK(root->GetSubdirectory("Banc")
              ->GetSubdirectory("GameBancParam")
              ->GetFile("prefetched.bin")
              ->ReadAllBytes() == std::vector<u8>{4, 5});
    CHECK(observed.empty());
    REQUIRE(param != nullptr);
    CHECK(param->ReadAllBytes() == std::vector<u8>{1, 2, 3});
    REQUIRE(observed.size() == 1);
    CHECK(observed.front() ==
          "/Banc/GameBancParam/KokiriForest_Child.game__banc__GameBancParam.bgyml");
}

TEST_CASE("RomFS mod layer removes and patches files", "[core][romfs]") {
    const auto base =
        Dir("", {File("kept", {1}), File("removed", {2}), File("patched", {0, 0, 0})},
            {Dir("gone", {File("x", {3})}), Dir("sub", {File("removed", {4}), File("kept", {5})})});

    const std::vector<u8> ips{'P', 'A', 'T', 'C', 'H', 0, 0, 1, 0, 1, 0xAA, 'E', 'O', 'F'};
    const auto ext =
        Dir("", {File("removed.stub", {}), File("patched.ips", ips), File("gone.stub", {})},
            {Dir("sub", {File("removed.stub", {})})});

    const auto root = FileSys::ExtractRomFS(FileSys::CreateRomFS(base, ext));
    REQUIRE(root != nullptr);
    CHECK(root->GetFile("kept") != nullptr);
    CHECK(root->GetFile("removed") == nullptr);
    CHECK(root->GetSubdirectory("gone") == nullptr);
    CHECK(root->GetFile("patched")->ReadAllBytes() == std::vector<u8>{0, 0xAA, 0});

    const auto sub = root->GetSubdirectory("sub");
    REQUIRE(sub != nullptr);
    CHECK(sub->GetFile("removed") == nullptr);
    CHECK(sub->GetFile("kept") != nullptr);
}

TEST_CASE("RomFS tables made by hand extract", "[core][romfs]") {
    const auto root = FileSys::ExtractRomFS(File(
        "x", MakeTables({{.child_dir = DirectoryEntrySize, .child_file = 0}, {}}, {FileEntrySize, NoEntry})));
    REQUIRE(root != nullptr);
    CHECK(root->GetFiles().size() == 2);
    CHECK(root->GetSubdirectories().size() == 1);
}

TEST_CASE("RomFS stops at the deepest path the format allows", "[core][romfs]") {
    CHECK(FileSys::ExtractRomFS(File("x", MakeTables(Chain(300), {}))) != nullptr);
    CHECK(FileSys::ExtractRomFS(File("x", MakeTables(Chain(5000), {}))) == nullptr);
}

TEST_CASE("RomFS with broken links is rejected", "[core][romfs]") {
    SECTION("no root directory") {
        CHECK(FileSys::ExtractRomFS(File("x", MakeTables({}, {}))) == nullptr);
    }
    SECTION("directory is its own sibling") {
        CHECK(FileSys::ExtractRomFS(File("x", MakeTables({{.sibling = 0}}, {}))) == nullptr);
    }
    SECTION("directory is its own child") {
        CHECK(FileSys::ExtractRomFS(File("x", MakeTables({{.child_dir = 0}}, {}))) == nullptr);
    }
    SECTION("file is its own sibling") {
        CHECK(FileSys::ExtractRomFS(File("x", MakeTables({{.child_file = 0}}, {0}))) == nullptr);
    }
    SECTION("sibling past the directory table") {
        CHECK(FileSys::ExtractRomFS(File("x", MakeTables({{.sibling = 0x1000}}, {}))) == nullptr);
    }
    SECTION("child past the directory table") {
        CHECK(FileSys::ExtractRomFS(File("x", MakeTables({{.child_dir = 0x1000}}, {}))) == nullptr);
    }
    SECTION("file past the file table") {
        CHECK(FileSys::ExtractRomFS(File("x", MakeTables({{.child_file = 0x1000}}, {}))) == nullptr);
    }
}

TEST_CASE("RomFS table sizes are limited by the file", "[core][romfs]") {
    auto bytes = MakeTables({{}}, {});
    Put<u64>(bytes, 0x20, std::numeric_limits<u64>::max());
    CHECK(FileSys::ExtractRomFS(File("x", std::move(bytes))) != nullptr);
}

TEST_CASE("RomFS rejects files that are not one", "[core][romfs]") {
    SECTION("no file gives an empty directory") {
        const auto root = FileSys::ExtractRomFS(nullptr);
        REQUIRE(root != nullptr);
        CHECK(root->GetFiles().empty());
    }
    SECTION("shorter than the header") {
        CHECK(FileSys::ExtractRomFS(File("x", std::vector<u8>(RomFSHeaderSize - 1))) == nullptr);
    }
    SECTION("wrong header size") {
        std::vector<u8> bytes(RomFSHeaderSize);
        const u64 header_size = RomFSHeaderSize + 8;
        std::memcpy(bytes.data(), &header_size, sizeof(header_size));
        CHECK(FileSys::ExtractRomFS(File("x", std::move(bytes))) == nullptr);
    }
    SECTION("no directory gives no RomFS") {
        CHECK(FileSys::CreateRomFS(nullptr, nullptr) == nullptr);
    }
}

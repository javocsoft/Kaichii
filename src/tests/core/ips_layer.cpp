// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/file_sys/ips_layer.h"
#include "core/file_sys/vfs/vfs_vector.h"

namespace {

using FileSys::IPSwitchCompiler;
using FileSys::PatchIPS;
using Bytes = std::vector<u8>;

FileSys::VirtualFile File(Bytes bytes) {
    return std::make_shared<FileSys::VectorVfsFile>(std::move(bytes), "file");
}

FileSys::VirtualFile Text(std::string_view text) {
    return File(Bytes(text.begin(), text.end()));
}

/// IPS: "PATCH", then 3-byte offset and big-endian 2-byte size with data, then "EOF".
Bytes Ips(std::initializer_list<u8> records, std::string_view end = "EOF") {
    Bytes bytes{'P', 'A', 'T', 'C', 'H'};
    bytes.insert(bytes.end(), records);
    bytes.insert(bytes.end(), end.begin(), end.end());
    return bytes;
}

Bytes Ips32(std::initializer_list<u8> records) {
    Bytes bytes{'I', 'P', 'S', '3', '2'};
    bytes.insert(bytes.end(), records);
    const std::string_view end{"EEOF"};
    bytes.insert(bytes.end(), end.begin(), end.end());
    return bytes;
}

Bytes Patched(const FileSys::VirtualFile& file) {
    REQUIRE(file != nullptr);
    return file->ReadAllBytes();
}

const Bytes Base(8, 0xFF);

} // namespace

TEST_CASE("IPS records replace bytes at their offsets", "[core][ips]") {
    const auto ips = Ips({0, 0, 1, 0, 2, 0xAA, 0xBB, 0, 0, 6, 0, 1, 0xCC});
    CHECK(Patched(PatchIPS(File(Base), File(ips))) ==
          Bytes{0xFF, 0xAA, 0xBB, 0xFF, 0xFF, 0xFF, 0xCC, 0xFF});
}

TEST_CASE("IPS32 uses four-byte offsets and EEOF", "[core][ips]") {
    Bytes base(0x20000, 0);
    const auto ips = Ips32({0, 1, 0, 0, 0, 1, 0xAA});
    const auto result = Patched(PatchIPS(File(base), File(ips)));
    CHECK(result[0x10000] == 0xAA);
    CHECK(result[0xFFFF] == 0);
}

TEST_CASE("IPS run-length records fill a range", "[core][ips]") {
    const auto ips = Ips({0, 0, 2, 0, 0, 0, 3, 0x11});
    CHECK(Patched(PatchIPS(File(Base), File(ips))) ==
          Bytes{0xFF, 0xFF, 0x11, 0x11, 0x11, 0xFF, 0xFF, 0xFF});
}

TEST_CASE("IPS run-length record stops at the end of the file", "[core][ips]") {
    const auto ips = Ips({0, 0, 6, 0, 0, 0, 5, 0x11});
    const auto result = Patched(PatchIPS(File(Base), File(ips)));
    CHECK(result.size() == Base.size());
    CHECK(result == Bytes{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x11, 0x11});
}

// [!shouldfail] marks a known bug (docs/ai/known-issues.md). When the bug is fixed this test passes,
// which fails the suite until the tag is removed.
TEST_CASE("IPS record stops at the end of the file like a run-length record",
          "[core][ips][!shouldfail]") {
    const auto ips = Ips({0, 0, 7, 0, 3, 0xAA, 0xBB, 0xCC});
    const auto patched = PatchIPS(File(Base), File(ips));
    REQUIRE(patched != nullptr);
    CHECK(patched->ReadAllBytes()[7] == 0xAA);
}

TEST_CASE("IPS leaves the base file unchanged", "[core][ips]") {
    const auto base = File(Base);
    REQUIRE(PatchIPS(base, File(Ips({0, 0, 0, 0, 1, 0xAA}))) != nullptr);
    CHECK(base->ReadAllBytes() == Base);
}

TEST_CASE("Invalid IPS patches are refused whole", "[core][ips]") {
    const auto base = File(Base);
    SECTION("missing files") {
        CHECK(PatchIPS(nullptr, File(Ips({}))) == nullptr);
        CHECK(PatchIPS(base, nullptr) == nullptr);
    }
    SECTION("unknown magic") {
        CHECK(PatchIPS(base, File({'N', 'O', 'P', 'E', '!'})) == nullptr);
    }
    SECTION("empty base") {
        CHECK(PatchIPS(File({}), File(Ips({}))) == nullptr);
    }
    SECTION("no end marker") {
        CHECK(PatchIPS(base, File({'P', 'A', 'T', 'C', 'H', 0, 0, 0, 0, 1, 0xAA})) == nullptr);
    }
    SECTION("offset past the end") {
        CHECK(PatchIPS(base, File(Ips({0, 1, 0, 0, 1, 0xAA}))) == nullptr);
    }
    SECTION("record cut short") {
        CHECK(PatchIPS(base, File({'P', 'A', 'T', 'C', 'H', 0, 0, 0, 0, 4, 0xAA})) == nullptr);
    }
}

TEST_CASE("IPSwitch applies enabled patches and skips disabled ones", "[core][ips]") {
    const IPSwitchCompiler compiler{Text("@enabled\n"
                                         "00000001 AABB\n"
                                         "@disabled\n"
                                         "00000004 CCDD\n"
                                         "@enabled\n"
                                         "00000006 EE // comment\n")};
    CHECK(Patched(compiler.Apply(File(Base))) ==
          Bytes{0xFF, 0xAA, 0xBB, 0xFF, 0xFF, 0xFF, 0xEE, 0xFF});
}

TEST_CASE("IPSwitch reads the build id and pads it", "[core][ips]") {
    const IPSwitchCompiler compiler{Text("@nsobid-0102030405\n")};
    std::array<u8, 0x20> expected{1, 2, 3, 4, 5};
    CHECK(compiler.GetBuildID() == expected);
}

TEST_CASE("IPSwitch string records keep their escapes", "[core][ips]") {
    const IPSwitchCompiler compiler{Text("@enabled\n00000001 \"a\\n\\\"b\"\n")};
    CHECK(Patched(compiler.Apply(File(Base))) ==
          Bytes{0xFF, 'a', '\n', '"', 'b', 0xFF, 0xFF, 0xFF});
}

TEST_CASE("IPSwitch offset shift moves later records", "[core][ips]") {
    const IPSwitchCompiler compiler{Text("@flag offset_shift 0x2\n@enabled\n00000001 AA\n")};
    CHECK(Patched(compiler.Apply(File(Base))) ==
          Bytes{0xFF, 0xFF, 0xFF, 0xAA, 0xFF, 0xFF, 0xFF, 0xFF});
}

TEST_CASE("IPSwitch stops at @stop", "[core][ips]") {
    const IPSwitchCompiler compiler{Text("@enabled\n00000000 AA\n@stop\n00000001 BB\n")};
    CHECK(Patched(compiler.Apply(File(Base)))[1] == 0xFF);
}

TEST_CASE("IPSwitch records past the end of the file are cut or dropped", "[core][ips]") {
    const IPSwitchCompiler compiler{Text("@enabled\n00000007 AABB\n00000010 CC\n")};
    CHECK(Patched(compiler.Apply(File(Base))) ==
          Bytes{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xAA});
}

TEST_CASE("IPSwitch without a file gives no file", "[core][ips]") {
    CHECK(IPSwitchCompiler{Text("@enabled\n")}.Apply(nullptr) == nullptr);
}

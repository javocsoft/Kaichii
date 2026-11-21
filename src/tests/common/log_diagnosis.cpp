// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "common/log_diagnosis.h"

using Common::LogDiagnosis::Analyze;
using Common::LogDiagnosis::Finding;
using Common::LogDiagnosis::Redact;

namespace {

constexpr std::string_view Header =
    "[   0.002] Frontend <Info> qt_common/qt_common.cpp:242:Init: Kaichii Version: Kaichii b48 | "
    "nx2-abc\n"
    "[   0.003] Frontend <Info> qt_common/qt_common.cpp:258:Init: Host CPU: Intel i7 | AVX2\n";

bool Has(const Common::LogDiagnosis::Analysis& analysis, Finding finding) {
    return std::ranges::find(analysis.findings, finding) != analysis.findings.end();
}

} // Anonymous namespace

TEST_CASE("A game that doesn't load without keys points at the keys", "[common][log_diagnosis]") {
    const std::string log =
        std::string{Header} +
        "[   0.160] Frontend <Warning> yuzu/main_window.cpp:5378:OnCheckFirmwareDecryption: "
        "Decryption keys are missing from /home/ana/.local/share/eden/keys: NCA, NSP and XCI "
        "games need them\n"
        "[   9.100] Frontend <Critical> yuzu/main_window.cpp:2006:LoadROM: Failed to load ROM! "
        "(0007-000E)\n";
    const auto analysis = Analyze(log);
    CHECK(Has(analysis, Finding::MissingKeys));
    CHECK_FALSE(Has(analysis, Finding::LoadFailed));
    CHECK(analysis.build.find("Kaichii b48") != std::string::npos);
    // The user's name never leaves the log.
    for (const auto& line : analysis.lines) {
        CHECK(line.find("ana") == std::string::npos);
    }
}

TEST_CASE("Keys missing alone are no problem for a .nup player", "[common][log_diagnosis]") {
    const std::string log =
        std::string{Header} +
        "[   0.160] Frontend <Warning> x: Decryption keys are missing from /home/ana/keys: NCA, "
        "NSP and XCI games need them\n"
        "[   5.000] Frontend <Info> x: Booting game: 0400486024DEC000 | The Legend of Zelda | \n"
        "[  60.000] Render.Vulkan <Info> x: frame\n";
    const auto analysis = Analyze(log);
    CHECK(analysis.findings.empty());
    CHECK(analysis.game_booted);
    CHECK(analysis.program_id == "0400486024DEC000");
}

TEST_CASE("The last session's driver and memory problems are found", "[common][log_diagnosis]") {
    const std::string log =
        std::string{Header} +
        "[   5.000] Frontend <Info> x: Booting game: 040074A01BF12000 | Donkey Kong | \n"
        "[  80.000] Render.Vulkan <Critical> x: Device loss occurred!\n"
        "[  80.100] Render.Vulkan <Error> x: Device fault: page fault at 0x1000\n"
        "[ 120.000] Frontend <Info> x: Booting game: 0400486024DEC000 | Ocarina | \n"
        "[ 130.000] HW.GPU <Warning> x: Out of device memory: keeping 512 MiB more free\n"
        "[ 131.000] HW.GPU <Error> x: Native graphics shader rejected: unknown opcode\n"
        "[ 131.100] HW.GPU <Error> x: Native graphics shader rejected: unknown opcode\n"
        "[ 140.000] Core.ARM <Critical> x: Stopping execution due to unmapped memory access at "
        "0x8\n";
    const auto analysis = Analyze(log);
    // Only the last session: the device loss belongs to the earlier game.
    CHECK_FALSE(Has(analysis, Finding::DeviceLost));
    CHECK(Has(analysis, Finding::OutOfVideoMemory));
    CHECK(Has(analysis, Finding::ShadersRejected));
    CHECK(Has(analysis, Finding::GuestCrashed));
    CHECK(analysis.program_id == "0400486024DEC000");
    // A repeated message is kept once, with its count.
    const auto repeated = std::ranges::count_if(analysis.lines, [](const std::string& line) {
        return line.find("shader rejected") != std::string::npos;
    });
    CHECK(repeated == 1);
    CHECK(std::ranges::any_of(analysis.lines, [](const std::string& line) {
        return line.find("(x2)") != std::string::npos;
    }));
}

TEST_CASE("Vulkan that doesn't start is found without a game", "[common][log_diagnosis]") {
    const std::string log = std::string{Header} +
                            "[   0.500] Render.Vulkan <Error> x: Vulkan initialization failed "
                            "with error: VK_ERROR_INCOMPATIBLE_DRIVER\n";
    const auto analysis = Analyze(log);
    CHECK(Has(analysis, Finding::VulkanUnavailable));
    CHECK_FALSE(analysis.game_booted);
}

TEST_CASE("A load failure without a key problem is a load failure", "[common][log_diagnosis]") {
    const std::string log = std::string{Header} +
                            "[   2.000] Frontend <Info> x: Installed firmware: No firmware "
                            "available\n"
                            "[   9.000] Frontend <Critical> x: Failed to load ROM! (0007-000C)\n";
    const auto analysis = Analyze(log);
    CHECK(Has(analysis, Finding::LoadFailed));
    CHECK(Has(analysis, Finding::NoFirmware));
    CHECK_FALSE(Has(analysis, Finding::MissingKeys));
}

TEST_CASE("User names are taken out of paths", "[common][log_diagnosis]") {
    CHECK(Redact("open /home/ana/games/x.nsp") == "open ~/games/x.nsp");
    CHECK(Redact("/Users/bob/Library/x and /Users/bob/y") == "~/Library/x and ~/y");
    CHECK(Redact("load C:\\Users\\Ana Lopez\\Games\\x.nsp") == "load ~\\Games\\x.nsp");
    CHECK(Redact("/home/ana") == "~");
    CHECK(Redact("no path here") == "no path here");
}

TEST_CASE("The kept lines are capped", "[common][log_diagnosis]") {
    std::string log{Header};
    for (int i = 0; i < 400; ++i) {
        log += "[ 1.0] HW.GPU <Error> x: problem " + std::to_string(i) + " " +
               std::string(400, 'a') + "\n";
    }
    const auto analysis = Analyze(log);
    CHECK(analysis.lines.size() == Common::LogDiagnosis::MaxLines);
    for (const auto& line : analysis.lines) {
        CHECK(line.size() <= Common::LogDiagnosis::MaxLineLength + 8);
    }
}

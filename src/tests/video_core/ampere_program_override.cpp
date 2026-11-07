// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <cstring>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "shader_recompiler/frontend/ampere/program_override.h"

namespace {

using Shader::Ampere::DecodeProgramOverride;
using Shader::Ampere::EncodeProgramOverride;
using Shader::Ampere::Instruction;
using Shader::Ampere::LoweredProgram;
using Shader::Ampere::NativeProgramKey;
using Shader::Ampere::ProgramOverrideSourceSize;

constexpr std::size_t HeaderWords = 6;
constexpr u32 VersionOne = 0x3130564f;

const std::array<Instruction, 2> Source{{{{1, 2, 3, 4}}, {{5, 6, 7, 8}}}};

LoweredProgram MakeProgram() {
    LoweredProgram program;
    program.instructions = {0x1111, 0x2222, 0x3333};
    program.source_instruction_offsets = {0, 2};
    program.source_size = sizeof(Source);
    program.required_texture_buffer = 3;
    return program;
}

void PutWord(std::vector<u8>& bytes, std::size_t word, u32 value) {
    std::memcpy(bytes.data() + word * sizeof(u32), &value, sizeof(u32));
}

} // namespace

TEST_CASE("Native program key is FNV-1a over the instruction bytes",
          "[video_core][ampere][program_override]") {
    // Dump file names and EDEN_NX2_REJECT_HASH lists use this value, so it may not change.
    CHECK(NativeProgramKey(std::span<const Instruction>{}) == 14695981039346656037ULL);
    CHECK(NativeProgramKey(std::span{Source}.first(1)) == 0x84c39a079fc08121ULL);
    const std::array<Instruction, 1> reordered{{{{2, 1, 3, 4}}}};
    CHECK(NativeProgramKey(reordered) == 0x91f3507ec8951fd1ULL);
}

TEST_CASE("Program override keeps a lowered program", "[video_core][ampere][program_override]") {
    const auto program = MakeProgram();
    const auto bytes = EncodeProgramOverride(Source, program);
    REQUIRE_FALSE(bytes.empty());
    CHECK(ProgramOverrideSourceSize(bytes) == sizeof(Source));

    const auto decoded = DecodeProgramOverride(bytes, Source);
    INFO(decoded.error);
    REQUIRE(decoded);
    CHECK(decoded.instructions == program.instructions);
    CHECK(decoded.source_instruction_offsets == program.source_instruction_offsets);
    CHECK(decoded.source_size == program.source_size);
    CHECK(decoded.required_texture_buffer == program.required_texture_buffer);
}

TEST_CASE("Version one program override has no address map",
          "[video_core][ampere][program_override]") {
    auto program = MakeProgram();
    auto bytes = EncodeProgramOverride(Source, program);
    bytes.resize(bytes.size() - program.source_instruction_offsets.size() * sizeof(u32));
    PutWord(bytes, 1, VersionOne);
    PutWord(bytes, 5, 0);

    const auto decoded = DecodeProgramOverride(bytes, Source);
    INFO(decoded.error);
    REQUIRE(decoded);
    CHECK(decoded.instructions == program.instructions);
    CHECK(decoded.source_instruction_offsets.empty());
}

TEST_CASE("A program that cannot be represented encodes to nothing",
          "[video_core][ampere][program_override]") {
    SECTION("failed lowering") {
        auto program = MakeProgram();
        program.error = "failed";
        CHECK(EncodeProgramOverride(Source, program).empty());
    }
    SECTION("source size differs") {
        auto program = MakeProgram();
        program.source_size = sizeof(Instruction);
        CHECK(EncodeProgramOverride(Source, program).empty());
    }
    SECTION("address map does not cover the source") {
        auto program = MakeProgram();
        program.source_instruction_offsets = {0};
        CHECK(EncodeProgramOverride(Source, program).empty());
    }
}

TEST_CASE("Damaged program override headers are refused",
          "[video_core][ampere][program_override]") {
    const auto valid = EncodeProgramOverride(Source, MakeProgram());
    REQUIRE_FALSE(valid.empty());
    auto bytes = valid;

    SECTION("too short for a header") {
        bytes.resize(HeaderWords * sizeof(u32) - 1);
    }
    SECTION("magic") {
        PutWord(bytes, 0, 0);
    }
    SECTION("version") {
        PutWord(bytes, 1, 0x3330564f);
    }
    SECTION("source size not a whole instruction") {
        PutWord(bytes, 2, sizeof(Instruction) + 1);
    }
    SECTION("no source") {
        PutWord(bytes, 2, 0);
    }
    SECTION("source over the limit") {
        PutWord(bytes, 2, 1048576 + sizeof(Instruction));
    }
    SECTION("no instructions") {
        PutWord(bytes, 3, 0);
    }
    SECTION("texture bank out of range") {
        PutWord(bytes, 4, 18);
    }
    SECTION("address map length") {
        PutWord(bytes, 5, 1);
    }
    SECTION("trailing bytes") {
        bytes.push_back(0);
    }
    SECTION("missing bytes") {
        bytes.pop_back();
    }

    CHECK(ProgramOverrideSourceSize(bytes) == 0);
    CHECK_FALSE(DecodeProgramOverride(bytes, Source));
}

TEST_CASE("Program override applies only to its exact source",
          "[video_core][ampere][program_override]") {
    const auto bytes = EncodeProgramOverride(Source, MakeProgram());

    const std::array<Instruction, 2> changed{{{{1, 2, 3, 4}}, {{5, 6, 7, 9}}}};
    const auto mismatch = DecodeProgramOverride(bytes, changed);
    CHECK_FALSE(mismatch);
    CHECK(mismatch.instructions.empty());

    const auto shorter = DecodeProgramOverride(bytes, std::span{Source}.first(1));
    CHECK_FALSE(shorter);
    CHECK(shorter.error == "native override source length mismatch");
}

TEST_CASE("Program override address map must be ordered and in range",
          "[video_core][ampere][program_override]") {
    auto bytes = EncodeProgramOverride(Source, MakeProgram());
    const std::size_t map_word = HeaderWords + sizeof(Source) / sizeof(u32) + 3 * 2;

    SECTION("past the instructions") {
        PutWord(bytes, map_word + 1, 3);
    }
    SECTION("going backwards") {
        PutWord(bytes, map_word, 2);
        PutWord(bytes, map_word + 1, 1);
    }

    const auto decoded = DecodeProgramOverride(bytes, Source);
    CHECK_FALSE(decoded);
    CHECK(decoded.source_instruction_offsets.empty());
}

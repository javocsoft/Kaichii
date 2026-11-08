// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <bit>
#include <cmath>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <catch2/catch_test_macros.hpp>

#include "shader_recompiler/backend/spirv/emit_spirv.h"
#include "shader_recompiler/frontend/maxwell/structured_control_flow.h"
#include "shader_recompiler/frontend/maxwell/translate_program.h"
#include "shader_recompiler/host_translate_info.h"

namespace {
namespace IR = Shader::IR;

// Three instructions follow each scheduling word; the program starts at 0x50.
u32 Address(u32 index) {
    return 0x50 + index * 8 + ((index + 1) / 3) * 8;
}

class MaxwellEnvironment final : public Shader::Environment {
public:
    explicit MaxwellEnvironment(std::span<const u64> code_) : code{code_} {
        stage = Shader::Stage::Compute;
    }

    u64 ReadInstruction(u32 address) override {
        for (u32 index = 0; index < code.size(); ++index) {
            if (Address(index) == address)
                return code[index];
        }
        FAIL("Read outside the instruction fixture at " << address);
        return 0;
    }
    u32 ReadCbufValue(u32, u32) override {
        FAIL("Unexpected constant buffer specialization");
        return 0;
    }
    Shader::TextureType ReadTextureType(u32) override {
        return Shader::TextureType::Color2D;
    }
    Shader::TexturePixelFormat ReadTexturePixelFormat(u32) override {
        return Shader::TexturePixelFormat::A8B8G8R8_UNORM;
    }
    bool IsTexturePixelFormatInteger(u32) override { return false; }
    u32 ReadViewportTransformState() override { return 1; }
    u32 TextureBoundBuffer() const override { return 0; }
    u32 LocalMemorySize() const override { return 0; }
    u32 SharedMemorySize() const override { return 4096; }
    std::array<u32, 3> WorkgroupSize() const override { return {32, 1, 1}; }
    bool HasHLEMacroState() const override { return false; }
    std::optional<Shader::ReplaceConstant> GetReplaceConstBuffer(u32, u32) override {
        return std::nullopt;
    }
    void Dump(u64, u64) override {}

private:
    std::span<const u64> code;
};

// Every instruction below runs unconditionally: its guard predicate (bits 16-19) is PT.
constexpr u64 PT = 7ULL << 16;

constexpr u64 Exit = 0xe30000000007000fULL;

// LDS.32 dest, [RZ + offset]
constexpr u64 LoadShared(u32 dest, u32 offset) {
    return 0xef48000000000000ULL | PT | dest | (255ULL << 8) | (u64{offset} << 20) | (4ULL << 48);
}

// STS.32 [RZ + offset], source
constexpr u64 StoreShared(u32 source, u32 offset) {
    return 0xef58000000000000ULL | PT | source | (255ULL << 8) | (u64{offset} << 20) | (4ULL << 48);
}

// FFMA.FMZ R0, R1, R2, R3, with or without .SAT
constexpr u64 FfmaFmz(bool sat) {
    return 0x5980000000000000ULL | PT | (1ULL << 8) | (2ULL << 20) | (3ULL << 39) |
           (u64{sat} << 50) | (2ULL << 53);
}

// HFMA2.FMZ R0, R1.H1_H0, R2.H1_H0, R3.H1_H0, with or without .SAT
constexpr u64 Hfma2Fmz(bool sat) {
    return 0x5d00000000000000ULL | PT | (1ULL << 8) | (2ULL << 20) | (u64{sat} << 32) |
           (2ULL << 37) | (3ULL << 39);
}

struct Translated {
    Shader::ObjectPool<IR::Inst> inst_pool;
    Shader::ObjectPool<IR::Block> block_pool;
    IR::Program program;
};

void Translate(Translated& out, std::span<const u64> code, bool float16 = false) {
    MaxwellEnvironment env{code};
    Shader::ObjectPool<Shader::Maxwell::Flow::Block> flow_pool;
    Shader::Maxwell::Flow::CFG cfg{env, flow_pool, Shader::Maxwell::Location{0x50}};
    Shader::HostTranslateInfo host_info{};
    host_info.support_float16 = float16;
    out.program =
        Shader::Maxwell::TranslateProgram(out.inst_pool, out.block_pool, env, cfg, host_info);
}

// Evaluates a value of the program, with shared memory words given by byte offset.
u32 Evaluate(IR::Value value, const std::map<u32, u32>& shared) {
    value = value.Resolve();
    if (value.IsImmediate()) {
        if (value.Type() == IR::Type::F32) {
            return std::bit_cast<u32>(value.F32());
        }
        return value.Type() == IR::Type::U1 ? u32{value.U1()} : value.U32();
    }
    const auto& inst = *value.Inst();
    const auto arg = [&](size_t index) { return Evaluate(inst.Arg(index), shared); };
    const auto f32 = [&](size_t index) { return std::bit_cast<float>(arg(index)); };
    switch (inst.GetOpcode()) {
    case IR::Opcode::Identity:
    case IR::Opcode::BitCastU32F32:
    case IR::Opcode::BitCastF32U32:
        return arg(0);
    case IR::Opcode::LoadSharedU32:
        return shared.at(arg(0));
    case IR::Opcode::FPFma32:
        return std::bit_cast<u32>(std::fma(f32(0), f32(1), f32(2)));
    case IR::Opcode::FPOrdEqual32:
        return f32(0) == f32(1);
    case IR::Opcode::LogicalOr:
        return arg(0) != 0 || arg(1) != 0;
    case IR::Opcode::SelectF32:
        return arg(0) != 0 ? arg(1) : arg(2);
    case IR::Opcode::FPSaturate32: {
        // The hardware saturates NaN to zero.
        const float x = f32(0);
        return std::bit_cast<u32>(std::isnan(x) ? 0.0f : std::fmin(std::fmax(x, 0.0f), 1.0f));
    }
    default:
        throw std::logic_error(std::string{IR::NameOf(inst.GetOpcode())});
    }
}

float StoredResult(const IR::Program& program, const std::map<u32, u32>& shared) {
    std::optional<float> result;
    for (const auto* block : program.blocks) {
        for (const auto& inst : block->Instructions()) {
            if (inst.GetOpcode() == IR::Opcode::WriteSharedU32) {
                REQUIRE_FALSE(result);
                INFO(IR::DumpProgram(program));
                result = std::bit_cast<float>(Evaluate(inst.Arg(1), shared));
            }
        }
    }
    REQUIRE(result);
    return *result;
}

size_t CountOpcode(const IR::Program& program, IR::Opcode opcode) {
    size_t count = 0;
    for (const auto* block : program.blocks) {
        for (const auto& inst : block->Instructions()) {
            count += inst.GetOpcode() == opcode ? 1 : 0;
        }
    }
    return count;
}

void EmitSpirv(IR::Program& program) {
    Shader::Profile profile{};
    profile.supported_spirv = 0x00010400;
    const auto spirv = Shader::Backend::SPIRV::EmitSPIRV(profile, program);
    REQUIRE(spirv.size() > 5);
    CHECK(spirv.front() == 0x07230203);
}
} // namespace

TEST_CASE("FFMA.FMZ keeps the addend when a zero multiplies Inf or NaN, with and without SAT",
          "[video_core][shader][fmz]") {
    const auto bits = [](float value) { return std::bit_cast<u32>(value); };
    const float inf = std::numeric_limits<float>::infinity();
    const float nan = std::numeric_limits<float>::quiet_NaN();
    for (const bool sat : {false, true}) {
        INFO("SAT " << sat);
        const std::array code{LoadShared(1, 0), LoadShared(2, 4), LoadShared(3, 8), FfmaFmz(sat),
                              StoreShared(0, 12), Exit};
        Translated translated;
        Translate(translated, code);
        const auto& program = translated.program;
        // 0 * Inf + 0.75 is 0.75 on the hardware: zero times anything is zero under FMZ.
        CHECK(StoredResult(program, {{0, bits(0.0f)}, {4, bits(inf)}, {8, bits(0.75f)}}) == 0.75f);
        CHECK(StoredResult(program, {{0, bits(nan)}, {4, bits(0.0f)}, {8, bits(0.25f)}}) == 0.25f);
        // Ordinary products are unchanged; SAT clamps them.
        CHECK(StoredResult(program, {{0, bits(2.0f)}, {4, bits(3.0f)}, {8, bits(-5.5f)}}) ==
              0.5f);
        CHECK(StoredResult(program, {{0, bits(2.0f)}, {4, bits(3.0f)}, {8, bits(1.0f)}}) ==
              (sat ? 1.0f : 7.0f));
        EmitSpirv(translated.program);
    }
}

TEST_CASE("HFMA2.FMZ on half pairs selects the addend, with and without SAT",
          "[video_core][shader][fmz]") {
    for (const bool float16 : {false, true}) {
        for (const bool sat : {false, true}) {
            INFO("float16 " << float16 << ", SAT " << sat);
            const std::array code{LoadShared(1, 0), LoadShared(2, 4),   LoadShared(3, 8),
                                  Hfma2Fmz(sat),    StoreShared(0, 12), Exit};
            Translated translated;
            Translate(translated, code, float16);
            const auto& program = translated.program;
            INFO(IR::DumpProgram(program));
            // Each half compares its operands with a zero of their type and selects the addend.
            // Without float16 support, the halves are lowered to 32-bit floats, selects included.
            const auto select = float16 ? IR::Opcode::SelectF16 : IR::Opcode::SelectF32;
            const auto saturate = float16 ? IR::Opcode::FPSaturate16 : IR::Opcode::FPSaturate32;
            CHECK(CountOpcode(program, select) == 2);
            CHECK(CountOpcode(program, saturate) == (sat ? 2 : 0));
            CHECK(CountOpcode(program, IR::Opcode::SelectF16) + CountOpcode(program, IR::Opcode::FPOrdEqual16) ==
                  (float16 ? 6 : 0));
            EmitSpirv(translated.program);
        }
    }
}

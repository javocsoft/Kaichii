// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <cstring>
#include <optional>
#include <span>

#include <catch2/catch_test_macros.hpp>

#include "shader_recompiler/backend/spirv/emit_spirv.h"
#include "shader_recompiler/frontend/ampere/lower_to_maxwell.h"
#include "shader_recompiler/frontend/maxwell/structured_control_flow.h"
#include "shader_recompiler/frontend/maxwell/translate_program.h"
#include "shader_recompiler/host_translate_info.h"

namespace {
using Shader::Ampere::Instruction;
using Shader::Ampere::LowerToMaxwell;
namespace IR = Shader::IR;

constexpr Instruction Exit{{0x0000794d, 0, 0x03800000, 0}};
constexpr Instruction OutputRead{{0x1e237923, 0x00002400, 0x0000b800, 0x000e2400}};
constexpr Instruction ZeroAddress{{0x001e7202, 0x000000ff, 0x00000f00, 0}};
constexpr Instruction WriteWord{{0x1eff7927, 0x0000241e, 0x0000b800, 0}};
constexpr Instruction WriteResult{{0x1eff7927, 0x00002823, 0x0000b800, 0}};
constexpr std::array<u32, 5> Header{0x000c0481, 0, 0, 0, 0xff000000};

class StageBufferEnvironment final : public Shader::Environment {
public:
    StageBufferEnvironment(Shader::Stage stage_, std::span<const u64> code_) : code{code_} {
        stage = stage_;
        is_proprietary_driver = true;
        std::memcpy(&sph, Header.data(), sizeof(Header));
        mesh_registers = {0, 0, 32 | (1U << 12)};
    }

    u64 ReadInstruction(u32 address) override {
        for (u32 index = 0; index < code.size(); ++index) {
            if (address == 0x50 + index * 8 + ((index + 1) / 3) * 8) {
                return code[index];
            }
        }
        FAIL("Read outside synthetic stage buffer program");
        return 0;
    }
    u32 ReadCbufValue(u32 bank, u32 offset) override { return (bank << 16) | offset; }
    Shader::TextureType ReadTextureType(u32) override { return Shader::TextureType::Color2D; }
    Shader::TexturePixelFormat ReadTexturePixelFormat(u32) override {
        return Shader::TexturePixelFormat::A8B8G8R8_UNORM;
    }
    bool IsTexturePixelFormatInteger(u32) override { return false; }
    u32 ReadViewportTransformState() override { return 1; }
    u32 TextureBoundBuffer() const override { return 15; }
    u32 LocalMemorySize() const override { return 0; }
    u32 SharedMemorySize() const override { return 0; }
    std::array<u32, 3> WorkgroupSize() const override { return {32, 1, 1}; }
    bool HasHLEMacroState() const override { return false; }
    bool HasNativeInstructions() const override { return true; }
    bool IsNativeProgram() const override { return true; }
    std::optional<Shader::ReplaceConstant> GetReplaceConstBuffer(u32, u32) override {
        return std::nullopt;
    }
    void Dump(u64, u64) override {}

private:
    std::span<const u64> code;
};

Shader::IR::Program Translate(StageBufferEnvironment& env, Shader::ObjectPool<Shader::Maxwell::Flow::Block>& flow_pool,
                              Shader::ObjectPool<IR::Inst>& inst_pool,
                              Shader::ObjectPool<IR::Block>& block_pool) {
    Shader::Maxwell::Flow::CFG cfg{env, flow_pool, Shader::Maxwell::Location{0x50}};
    return Shader::Maxwell::TranslateProgram(inst_pool, block_pool, env, cfg,
                                             Shader::HostTranslateInfo{});
}
} // namespace

TEST_CASE("Only supported Ampere output attribute reads are lowered",
          "[video_core][ampere][stage_buffer_output]") {
    const auto lowered = LowerToMaxwell(std::array{OutputRead, Exit}, false);
    INFO(lowered.error << " at " << lowered.error_offset);
    REQUIRE(lowered);
    REQUIRE(lowered.instructions[0] == 0xefc0b00002471e23ULL);

    auto skew = OutputRead;
    skew.words[2] |= 1U << 14;
    auto byte = OutputRead;
    byte.words[2] &= ~(1U << 11);
    auto map = OutputRead;
    map.words[2] &= ~(3U << 12);
    auto zero_dest = OutputRead;
    zero_dest.words[0] = (zero_dest.words[0] & ~(0xffU << 16)) | (0xffU << 16);
    for (const Instruction rejected : {skew, byte, map, zero_dest}) {
        REQUIRE_FALSE(LowerToMaxwell(std::array{rejected, Exit}, false));
    }
}

TEST_CASE("Task output attribute read reaches payload SPIR-V in both renderer paths",
          "[video_core][ampere][stage_buffer_output]") {
    const auto lowered = LowerToMaxwell(
        std::array{ZeroAddress, WriteWord, OutputRead, WriteResult, Exit}, false);
    INFO(lowered.error << " at " << lowered.error_offset);
    REQUIRE(lowered);

    StageBufferEnvironment env{Shader::Stage::Task, lowered.instructions};
    Shader::ObjectPool<Shader::Maxwell::Flow::Block> flow_pool;
    Shader::ObjectPool<IR::Inst> inst_pool;
    Shader::ObjectPool<IR::Block> block_pool;
    auto program = Translate(env, flow_pool, inst_pool, block_pool);
    REQUIRE(program.stage == Shader::Stage::Task);

    u32 output_reads{}, input_reads{}, writes{};
    for (const auto* block : program.blocks) {
        for (const auto& inst : *block) {
            output_reads += inst.GetOpcode() == IR::Opcode::IsbeReadOutput;
            input_reads += inst.GetOpcode() == IR::Opcode::IsbeRead;
            writes += inst.GetOpcode() == IR::Opcode::IsbeWriteAttribute;
        }
    }
    REQUIRE(output_reads == 1);
    REQUIRE(input_reads == 0);
    REQUIRE(writes == 2);

    Shader::Profile profile{};
    profile.supported_spirv = 0x00010500;
    for (const bool as_compute : {false, true}) {
        Shader::RuntimeInfo runtime{};
        runtime.mesh_as_compute = as_compute;
        if (as_compute) {
            runtime.mesh_output_binding = 4;
            runtime.mesh_task_payload_binding = 5;
            runtime.mesh_task_offsets_binding = 6;
            runtime.mesh_task_args_binding = 7;
            runtime.mesh_task_payload_stride = program.mesh.payload_words;
        }
        Shader::Backend::Bindings bindings{};
        const auto spirv = Shader::Backend::SPIRV::EmitSPIRV(profile, runtime, program, bindings);
        REQUIRE(spirv.size() > 5);
        REQUIRE(spirv[0] == 0x07230203);
    }
}

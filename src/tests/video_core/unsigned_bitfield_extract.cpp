// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "shader_recompiler/ir_opt/unsigned_bitfield_extract.h"

TEST_CASE("Unsigned shader bitfield extraction handles zero and full widths", "[shader][bitfield]") {
    using Shader::Optimization::ExtractUnsignedBitfield;
    CHECK(ExtractUnsignedBitfield(0xdeadbeef, 0, 0) == 0);
    CHECK(ExtractUnsignedBitfield(0xdeadbeef, 32, 0) == 0);
    CHECK(ExtractUnsignedBitfield(0xdeadbeef, 0, 32) == 0xdeadbeef);
    CHECK(ExtractUnsignedBitfield(0xdeadbeef, 8, 8) == 0xbe);
}

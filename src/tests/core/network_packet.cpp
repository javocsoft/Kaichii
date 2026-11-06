// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "network/packet.h"

using Network::Packet;

TEST_CASE("Packet keeps every value type in order", "[core][network_packet]") {
    Packet packet;
    packet.Write(true)
        .Write(s8{-2})
        .Write(u8{200})
        .Write(s16{-3000})
        .Write(u16{60000})
        .Write(s32{-70000})
        .Write(u32{0xDEADBEEF})
        .Write(s64{std::numeric_limits<s64>::min()})
        .Write(u64{0x0123456789ABCDEF})
        .Write(1.5F)
        .Write(-2.25)
        .Write(std::string{"room"})
        .Write(std::vector<u16>{1, 2, 3})
        .Write(std::array<u32, 2>{4, 5});

    bool flag{};
    s8 tiny{};
    u8 byte{};
    s16 small{};
    u16 usmall{};
    s32 word{};
    u32 uword{};
    s64 big{};
    u64 ubig{};
    float single{};
    double dual{};
    std::string text;
    std::vector<u16> list;
    std::array<u32, 2> fixed{};
    packet.Read(flag)
        .Read(tiny)
        .Read(byte)
        .Read(small)
        .Read(usmall)
        .Read(word)
        .Read(uword)
        .Read(big)
        .Read(ubig)
        .Read(single)
        .Read(dual)
        .Read(text)
        .Read(list)
        .Read(fixed);

    REQUIRE(static_cast<bool>(packet));
    CHECK(flag);
    CHECK(tiny == -2);
    CHECK(byte == 200);
    CHECK(small == -3000);
    CHECK(usmall == 60000);
    CHECK(word == -70000);
    CHECK(uword == 0xDEADBEEF);
    CHECK(big == std::numeric_limits<s64>::min());
    CHECK(ubig == 0x0123456789ABCDEF);
    CHECK(single == 1.5F);
    CHECK(dual == -2.25);
    CHECK(text == "room");
    CHECK(list == std::vector<u16>{1, 2, 3});
    CHECK(fixed == std::array<u32, 2>{4, 5});
    CHECK(packet.EndOfPacket());
}

TEST_CASE("Packet integers are big endian on the wire", "[core][network_packet]") {
    // Rooms talk across platforms, so this order is the protocol.
    Packet packet;
    packet.Write(u32{0x01020304}).Write(u16{0x0506}).Write(u64{0x0708090A0B0C0D0E});
    const std::array<u8, 14> expected{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    REQUIRE(packet.GetDataSize() == expected.size());
    CHECK(std::memcmp(packet.GetData(), expected.data(), expected.size()) == 0);
}

TEST_CASE("Reading past the end invalidates the packet", "[core][network_packet]") {
    Packet packet;
    packet.Write(u8{1});
    u32 value = 0x55;
    packet.Read(value);
    CHECK_FALSE(static_cast<bool>(packet));
}

TEST_CASE("A string longer than the packet is not read", "[core][network_packet]") {
    Packet packet;
    packet.Write(u32{1000}).Write(u8{'x'});
    std::string text;
    packet.Read(text);
    CHECK(text.empty());
    CHECK_FALSE(static_cast<bool>(packet));
}

TEST_CASE("Clear makes a packet reusable", "[core][network_packet]") {
    Packet packet;
    packet.Write(u8{1});
    u32 value{};
    packet.Read(value);
    packet.Clear();
    CHECK(static_cast<bool>(packet));
    CHECK(packet.GetDataSize() == 0);
    CHECK(packet.EndOfPacket());
}

TEST_CASE("IgnoreBytes skips ahead but not past the end", "[core][network_packet]") {
    Packet packet;
    packet.Write(u8{1}).Write(u8{2});
    packet.IgnoreBytes(1);
    u8 value{};
    packet.Read(value);
    CHECK(value == 2);
    packet.IgnoreBytes(10);
    CHECK(packet.EndOfPacket());
}

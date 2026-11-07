// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <enet/enet.h>

#include "network/packet.h"
#include "network/room.h"
#include "network/room_member.h"

using Network::Packet;
using Network::RoomMember;

namespace {

constexpr u16 TestPort = 24899;
const std::string Password = "secret";

bool WaitFor(const std::function<bool()>& condition) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{10};
    while (!condition()) {
        if (std::chrono::steady_clock::now() > deadline) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    return true;
}

struct EnetLibrary {
    EnetLibrary() {
        REQUIRE(enet_initialize() == 0);
    }
    ~EnetLibrary() {
        enet_deinitialize();
    }
};

/// Speaks ENet to a room as a client that is free to send what no RoomMember would.
class RawPeer {
public:
    RawPeer() {
        host = enet_host_create(nullptr, 1, 1, 0, 0);
        REQUIRE(host != nullptr);
        ENetAddress address{};
        REQUIRE(enet_address_set_host_ip(&address, "127.0.0.1") == 0);
        address.port = TestPort;
        peer = enet_host_connect(host, &address, 1, 0);
        REQUIRE(peer != nullptr);
        ENetEvent event;
        REQUIRE(enet_host_service(host, &event, 5000) > 0);
        REQUIRE(event.type == ENET_EVENT_TYPE_CONNECT);
    }

    ~RawPeer() {
        enet_peer_reset(peer);
        enet_host_destroy(host);
    }

    void Send(const void* data, size_t size) {
        ENetPacket* const packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
        REQUIRE(enet_peer_send(peer, 0, packet) == 0);
        enet_host_flush(host);
    }

    void Send(const Packet& packet) {
        Send(packet.GetData(), packet.GetDataSize());
    }

private:
    ENetHost* host{};
    ENetPeer* peer{};
};

Packet ProxyBroadcast() {
    Packet packet;
    packet.Write(static_cast<u8>(Network::IdProxyPacket));
    packet.Write(u8{0});
    packet.Write(Network::IPv4Address{});
    packet.Write(u16{0});
    packet.Write(u8{0});
    packet.Write(Network::IPv4Address{});
    packet.Write(u16{0});
    packet.Write(u8{0});
    packet.Write(true);
    packet.Write(std::vector<u8>{1, 2, 3});
    return packet;
}

Packet JoinRequest(const std::string& nickname, const std::string& password) {
    Packet packet;
    packet.Write(static_cast<u8>(Network::IdJoinRequest));
    packet.Write(nickname);
    packet.Write(Network::NoPreferredIP);
    packet.Write(Network::network_version);
    packet.Write(password);
    packet.Write(std::string{});
    return packet;
}

void Join(RoomMember& member, const std::string& nickname) {
    member.Join(nickname, "127.0.0.1", TestPort, 0, Network::NoPreferredIP, Password);
    REQUIRE(WaitFor([&] { return member.GetState() == RoomMember::State::Joined; }));
}

} // namespace

TEST_CASE("A peer that never joined reaches no room member", "[network][room]") {
    EnetLibrary enet;
    Network::Room room;
    REQUIRE(room.Create("test", "", "127.0.0.1", TestPort, Password));

    RoomMember listener;
    std::mutex received_mutex;
    std::vector<std::vector<u8>> received;
    const auto handle = listener.BindOnProxyPacketReceived(
        [&](const Network::ProxyPacket& packet) {
            std::scoped_lock lock{received_mutex};
            received.push_back(packet.data);
        });
    Join(listener, "listener");

    RawPeer intruder;
    intruder.Send(ProxyBroadcast());

    // The intruder's packet is long handled by the time this member has joined, so what arrives
    // first has to be the member's.
    RoomMember sender;
    Join(sender, "sender");
    Network::ProxyPacket packet{};
    packet.broadcast = true;
    packet.data = {4};
    sender.SendProxyPacket(packet);
    REQUIRE(WaitFor([&] {
        std::scoped_lock lock{received_mutex};
        return !received.empty();
    }));
    {
        std::scoped_lock lock{received_mutex};
        CHECK(received == std::vector<std::vector<u8>>{{4}});
    }

    listener.Unbind(handle);
    sender.Leave();
    listener.Leave();
    room.Destroy();
}

TEST_CASE("A room keeps one member for each connection", "[network][room]") {
    EnetLibrary enet;
    Network::Room room;
    REQUIRE(room.Create("test", "", "127.0.0.1", TestPort, Password));

    RawPeer peer;
    peer.Send(JoinRequest("first", Password));
    REQUIRE(WaitFor([&] { return room.GetRoomMemberList().size() == 1; }));
    peer.Send(JoinRequest("second", Password));

    RoomMember witness;
    Join(witness, "witness");
    CHECK(room.GetRoomMemberList().size() == 2);

    witness.Leave();
    room.Destroy();
}

TEST_CASE("A wrong password does not make a peer a member", "[network][room]") {
    EnetLibrary enet;
    Network::Room room;
    REQUIRE(room.Create("test", "", "127.0.0.1", TestPort, Password));

    RawPeer peer;
    peer.Send(JoinRequest("guest", "wrong"));
    RoomMember witness;
    Join(witness, "witness");
    CHECK(room.GetRoomMemberList().size() == 1);

    witness.Leave();
    room.Destroy();
}

TEST_CASE("Malformed packets leave a room running", "[network][room]") {
    EnetLibrary enet;
    Network::Room room;
    REQUIRE(room.Create("test", "", "127.0.0.1", TestPort, Password));

    {
        RawPeer peer;
        // An empty packet, and a join request whose nickname claims to be 4 GiB long.
        peer.Send(nullptr, 0);
        constexpr std::array<u8, 5> huge_string{Network::IdJoinRequest, 0xFF, 0xFF, 0xFF, 0xFF};
        peer.Send(huge_string.data(), huge_string.size());
        for (const u8 type : {Network::IdSetGameInfo, Network::IdLdnPacket, Network::IdChatMessage,
                              Network::IdModKick, Network::IdModBan, Network::IdModUnban}) {
            peer.Send(&type, sizeof(type));
        }
    }

    RoomMember witness;
    Join(witness, "witness");
    CHECK(room.GetState() == Network::Room::State::Open);

    witness.Leave();
    room.Destroy();
}

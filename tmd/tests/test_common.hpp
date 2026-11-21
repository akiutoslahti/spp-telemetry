#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include <spp/expected.hpp>
#include <spp/packet.hpp>
#include <spp/primary_header.hpp>
#include <tmd/uniquefd.hpp>

namespace tmdtest {
std::string make_tmp_dir(const std::string &dir_name);

std::uint16_t get_udp_socket_port(const tmd::UniqueFd &fd);

constexpr spp::Expected<spp::Apid, spp::Error> FakeApid =
    spp::Apid::create(0x0);
constexpr spp::Expected<spp::SequenceCount, spp::Error> FakeSequenceCount =
    spp::SequenceCount::create(0x0);
constexpr spp::PacketPrimaryHeader FakeHeader = {
    .type = spp::PacketType::Telemetry,
    .secondary_header_flag = spp::SecondaryHeaderFlag::NotPresent,
    .apid = *FakeApid,
    .sequence_flags = spp::SequenceFlags::Unsegmented,
    .sequence_count = *FakeSequenceCount,
    .data_length = 0x0};
constexpr std::array<std::byte, 1> FakePacketData = {std::byte{0xAA}};
constexpr spp::Packet FakePacket =
    spp::Packet{.header = FakeHeader, .packet_data = FakePacketData};
constexpr std::size_t FakePacketSize =
    spp::PrimaryHeaderSize + FakePacketData.size();
} // namespace tmdtest

#pragma once

#include <spp/error.hpp>
#include <spp/expected.hpp>
#include <spp/primary_header.hpp>

#include <cstddef>
#include <span>

namespace spp {

struct Packet {
  // Packet Primary Header (6 octets)
  PacketPrimaryHeader header;

  // Packet Data Field (1 - 65536 octets)
  // Note:
  //  * Size of the Packet Data Field must match with Packet Data Length defined
  //    in header (header.data_length == packet_data.size() - 1)
  //  * packet_data points into the buffer passed to parse_packet() function
  std::span<const std::byte> packet_data;
};

[[nodiscard]] Expected<std::size_t, Error>
serialize_packet(Packet packet, std::span<std::byte> dst) noexcept;

[[nodiscard]] Expected<Packet, Error>
parse_packet(std::span<const std::byte> src) noexcept;
} // namespace spp

#include <spp/error.hpp>
#include <spp/expected.hpp>
#include <spp/packet.hpp>
#include <spp/primary_header.hpp>

#include <algorithm>
#include <cstddef>
#include <span>

namespace {
[[nodiscard]] constexpr std::size_t
data_length_to_real_length(std::size_t length) noexcept {
  return length + 1;
}
} // namespace

namespace spp {
[[nodiscard]] Expected<std::size_t, Error>
serialize_packet(Packet packet, std::span<std::byte> dst) noexcept {

  const std::size_t real_data_length =
      data_length_to_real_length(packet.header.data_length);
  if (packet.packet_data.size() != real_data_length) {
    return Unexpected(Error::PacketDataLengthMismatch);
  }

  if (dst.size() < PrimaryHeaderSize + real_data_length) {
    return Unexpected(Error::DestinationBufferTooShort);
  }

  auto res = serialize_header(packet.header, dst);
  if (!res) {
    return Unexpected(res.error());
  }

  std::ranges::copy(packet.packet_data,
                    dst.subspan(PrimaryHeaderSize, real_data_length).begin());

  return PrimaryHeaderSize + real_data_length;
}

[[nodiscard]] Expected<Packet, Error>
parse_packet(std::span<const std::byte> src) noexcept {
  auto header = parse_header(src);
  if (!header) {
    return Unexpected(header.error());
  }

  const std::size_t real_data_length =
      data_length_to_real_length(header->data_length);
  if (src.size() < PrimaryHeaderSize + real_data_length) {
    return Unexpected(Error::TruncatedPacketDataField);
  }

  auto packet_data = src.subspan(PrimaryHeaderSize, real_data_length);

  return Packet{.header = *header, .packet_data = packet_data};
}
} // namespace spp
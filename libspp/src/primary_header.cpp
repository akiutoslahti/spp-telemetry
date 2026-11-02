#include <spp/error.hpp>
#include <spp/expected.hpp>
#include <spp/primary_header.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace {
constexpr std::size_t BitsPerOctet = 8U;

[[nodiscard]] constexpr std::uint16_t
read_be16(std::span<const std::byte, 2> src) noexcept {
  return static_cast<std::uint16_t>(
      (std::to_integer<std::uint16_t>(src[0]) << BitsPerOctet) |
      std::to_integer<std::uint16_t>(src[1]));
}
} // namespace

namespace spp {
constexpr std::size_t PacketVersionNumberShift = 13U;
constexpr std::size_t PacketTypeShift = 12U;
constexpr std::uint16_t PacketTypeMask = 0x1U;
constexpr std::size_t SecondaryHeaderFlagShift = 11U;
constexpr std::uint16_t SecondaryHeaderFlagMask = 0x1U;
constexpr std::uint16_t ApidMask = 0x7FFU;
constexpr std::size_t SequenceFlagsShift = 14U;
constexpr std::uint16_t SequenceCountMask = 0x3FFFU;

Expected<std::size_t, Error>
serialize_header(PacketPrimaryHeader header,
                 std::span<std::byte> dst) noexcept {
  (void)header;
  (void)dst;

  return Unexpected(Error::Internal);
}

Expected<PacketPrimaryHeader, Error>
parse_header(std::span<const std::byte> src) noexcept {
  if (src.size() < PrimaryHeaderSize) {
    return Unexpected(Error::PrimaryHeaderTooShort);
  }

  const auto header = src.first<PrimaryHeaderSize>();

  const auto packet_identification = read_be16(header.subspan<0, 2>());

  if (static_cast<std::uint8_t>(packet_identification >>
                                PacketVersionNumberShift) !=
      SupportedPacketVersionNumber) {
    return Unexpected(Error::UnsupportedPacketVersionNumber);
  }

  auto type = static_cast<PacketType>(
      (packet_identification >> PacketTypeShift) & PacketTypeMask);

  auto secondary_header_flag = static_cast<SecondaryHeaderFlag>(
      (packet_identification >> SecondaryHeaderFlagShift) &
      SecondaryHeaderFlagMask);

  auto apid = Apid::create(packet_identification & ApidMask);

  if (!apid) {
    return Unexpected(apid.error());
  }

  const auto packet_sequence_control = read_be16(header.subspan<2, 2>());

  auto sequence_flags =
      static_cast<SequenceFlags>(packet_sequence_control >> SequenceFlagsShift);

  auto sequence_count =
      SequenceCount::create(packet_sequence_control & SequenceCountMask);

  if (!sequence_count) {
    return Unexpected(sequence_count.error());
  }

  const auto data_length = read_be16(header.subspan<4, 2>());

  return PacketPrimaryHeader{.type = type,
                             .secondary_header_flag = secondary_header_flag,
                             .apid = *apid,
                             .sequence_flags = sequence_flags,
                             .sequence_count = *sequence_count,
                             .data_length = data_length};
}
} // namespace spp

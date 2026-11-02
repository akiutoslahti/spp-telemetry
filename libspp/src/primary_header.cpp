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

constexpr void write_be16(std::uint16_t val,
                          std::span<std::byte, 2> dst) noexcept {
  dst[0] = std::byte{static_cast<std::uint8_t>(val >> BitsPerOctet)};
  dst[1] = std::byte{static_cast<std::uint8_t>(val)};
}

[[nodiscard]] constexpr bool
packet_type_is_valid(spp::PacketType type) noexcept {
  switch (type) {
  case spp::PacketType::Telemetry:
  case spp::PacketType::Telecommand:
    return true;
  }

  return false;
}

[[nodiscard]] constexpr bool
secondary_header_flag_is_valid(spp::SecondaryHeaderFlag flag) noexcept {
  switch (flag) {
  case spp::SecondaryHeaderFlag::NotPresent:
  case spp::SecondaryHeaderFlag::Present:
    return true;
  }

  return false;
}

[[nodiscard]] constexpr bool
sequence_flags_is_valid(spp::SequenceFlags flags) noexcept {
  switch (flags) {
  case spp::SequenceFlags::ContinuationSegment:
  case spp::SequenceFlags::FirstSegment:
  case spp::SequenceFlags::LastSegment:
  case spp::SequenceFlags::Unsegmented:
    return true;
  }

  return false;
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
  if (dst.size() < PrimaryHeaderSize) {
    return Unexpected(Error::DestinationBufferTooShort);
  }

  if (!packet_type_is_valid(header.type)) {
    return Unexpected(Error::InvalidPacketType);
  }

  if (!secondary_header_flag_is_valid(header.secondary_header_flag)) {
    return Unexpected(Error::InvalidSecondaryHeaderFlag);
  }

  if (!sequence_flags_is_valid(header.sequence_flags)) {
    return Unexpected(Error::InvalidSequenceFlags);
  }

  const auto packet_identification = static_cast<std::uint16_t>(
      (SupportedPacketVersionNumber << PacketVersionNumberShift) |
      (static_cast<std::uint8_t>(header.type) << PacketTypeShift) |
      (static_cast<std::uint8_t>(header.secondary_header_flag)
       << SecondaryHeaderFlagShift) |
      header.apid.get());

  const auto packet_sequence_control = static_cast<std::uint16_t>(
      (static_cast<std::uint8_t>(header.sequence_flags) << SequenceFlagsShift) |
      header.sequence_count.get());

  auto dst_header = dst.first<PrimaryHeaderSize>();
  write_be16(packet_identification, dst_header.subspan<0, 2>());
  write_be16(packet_sequence_control, dst_header.subspan<2, 2>());
  write_be16(header.data_length, dst_header.subspan<4, 2>());

  return PrimaryHeaderSize;
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

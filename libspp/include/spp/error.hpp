#pragma once

#include <cstdint>
#include <string_view>

namespace spp {
enum class Error : std::uint8_t {
  Internal,
  InvalidApid,
  InvalidSequenceCount,
  PrimaryHeaderTooShort,
  UnsupportedPacketVersionNumber,
  DestinationBufferTooShort,
  InvalidPacketType,
  InvalidSecondaryHeaderFlag,
  InvalidSequenceFlags,
  IdlePacketWithSecondaryHeader,
  TruncatedPacketDataField,
  PacketDataLengthMismatch
};

constexpr std::size_t NrErrors =
    static_cast<std::size_t>(Error::PacketDataLengthMismatch) + 1;

[[nodiscard]] constexpr std::string_view to_string(Error error) noexcept {
  switch (error) {
  case Error::Internal:
    return "Internal error";
  case Error::InvalidApid:
    return "Invalid APID";
  case Error::InvalidSequenceCount:
    return "Invalid Sequence Count";
  case Error::PrimaryHeaderTooShort:
    return "Packet Primary Header is too short";
  case Error::UnsupportedPacketVersionNumber:
    return "Packet Version Number is unsupported";
  case Error::DestinationBufferTooShort:
    return "Destination buffer is too short";
  case Error::InvalidPacketType:
    return "Invalid Packet Type";
  case Error::InvalidSecondaryHeaderFlag:
    return "Invalid Secondary Header Flag";
  case Error::InvalidSequenceFlags:
    return "Invalid Sequence Flags";
  case Error::IdlePacketWithSecondaryHeader:
    return "Invalid packet (Idle packet with Secondary Header)";
  case Error::TruncatedPacketDataField:
    return "Truncated Packet Data Field";
  case Error::PacketDataLengthMismatch:
    return "Packet Data Length (in header) does not match length of the Packet "
           "Data Field";
  }

  return "Unknown error";
}
} // namespace spp

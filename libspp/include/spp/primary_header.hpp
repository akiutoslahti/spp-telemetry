#pragma once

#include <spp/error.hpp>
#include <spp/expected.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace spp {
constexpr std::size_t PrimaryHeaderSize = 6U;

constexpr std::uint8_t SupportedPacketVersionNumber = 0U;

constexpr std::uint16_t ApidMax = 0x7FFU;
constexpr std::uint16_t ApidIdlePacket = ApidMax;
constexpr std::uint16_t SequenceCountMax = 0x3FFFU;

enum class PacketType : std::uint8_t { Telemetry = 0, Telecommand = 1 };

enum class SecondaryHeaderFlag : std::uint8_t { NotPresent = 0, Present = 1 };

class Apid {
private:
  std::uint16_t m_apid;

  constexpr explicit Apid(std::uint16_t apid) noexcept : m_apid(apid) {}

public:
  [[nodiscard]] static constexpr Expected<Apid, Error>
  create(std::uint16_t apid) noexcept {
    if (apid > ApidMax) {
      return Unexpected(Error::InvalidApid);
    }
    return Apid(apid);
  }

  [[nodiscard]] constexpr std::uint16_t get() const noexcept { return m_apid; }
};

enum class SequenceFlags : std::uint8_t {
  ContinuationSegment = 0,
  FirstSegment = 1,
  LastSegment = 2,
  Unsegmented = 3
};

class SequenceCount {
private:
  std::uint16_t m_seq_count;

  constexpr explicit SequenceCount(std::uint16_t seq_count) noexcept
      : m_seq_count(seq_count) {}

public:
  [[nodiscard]] static constexpr Expected<SequenceCount, Error>
  create(std::uint16_t seq_count) noexcept {
    if (seq_count > SequenceCountMax) {
      return Unexpected(Error::InvalidSequenceCount);
    }
    return SequenceCount(seq_count);
  }

  [[nodiscard]] constexpr std::uint16_t get() const noexcept {
    return m_seq_count;
  }
};

struct PacketPrimaryHeader {
  // Packet Version Number (3b) omitted as CCSDS 133.0-B-2 defines only one
  // valid version (000b).

  //
  // Packet Identification
  //

  // Packet Type (1b)
  PacketType type;
  // Secondary Header Flag (1b)
  SecondaryHeaderFlag secondary_header_flag;
  // Application Process Identifier (APID) (11b)
  Apid apid;

  //
  // Packet Sequence Control
  //

  // Sequence Flags (2b)
  SequenceFlags sequence_flags;
  // Packet Sequence Count or Packet Name (14b)
  SequenceCount sequence_count;

  // Packet Data Length (16b) equals one fewer than the length of the Packet
  // Data Field (in octets). Ref. CCSDS 133.0-B-2 section 4.1.3.5.2
  std::uint16_t data_length;
};

[[nodiscard]] Expected<std::size_t, Error>
serialize_header(PacketPrimaryHeader header, std::span<std::byte> dst) noexcept;

[[nodiscard]] Expected<PacketPrimaryHeader, Error>
parse_header(std::span<const std::byte> src) noexcept;
} // namespace spp

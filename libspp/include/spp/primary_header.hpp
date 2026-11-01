#pragma once

#include <spp/error.hpp>
#include <spp/expected.hpp>

#include <cstdint>

namespace spp {
constexpr std::uint16_t ApidMax = 0x7FFU;
constexpr std::uint16_t ApidIdlePacket = ApidMax;

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

struct PacketPrimaryHeader {
  // Packet Version Number (3b)

  // Packet Identification
  PacketType type;
  SecondaryHeaderFlag secondary_header_flag;
  Apid apid;

  // Packet Sequence Control:
  SequenceFlags sequence_flags;
  // Packet Sequence Count (14b)

  // Packet Data Length (16b)
};
} // namespace spp

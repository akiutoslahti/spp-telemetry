#include <spp/error.hpp>
#include <spp/expected.hpp>
#include <spp/packet.hpp>
#include <spp/primary_header.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

namespace {
// min length packet
constexpr spp::Expected<spp::Apid, spp::Error> FakeApid =
    spp::Apid::create(0x1);
constexpr spp::Expected<spp::SequenceCount, spp::Error> FakeSequenceCount =
    spp::SequenceCount::create(0x1);
constexpr spp::PacketPrimaryHeader FakeHeader = {
    .type = spp::PacketType::Telemetry,
    .secondary_header_flag = spp::SecondaryHeaderFlag::NotPresent,
    .apid = *FakeApid,
    .sequence_flags = spp::SequenceFlags::ContinuationSegment,
    .sequence_count = *FakeSequenceCount,
    .data_length = 0x0};
constexpr spp::Packet FakePacket = {
    .header = FakeHeader, .packet_data = std::span<const std::byte, 0>{}};
constexpr std::array<std::byte, spp::PrimaryHeaderSize> FakeHeaderVector = {
    std::byte{0x00}, std::byte{0x01}, std::byte{0x00},
    std::byte{0x01}, std::byte{0x00}, std::byte{0x00}};
constexpr std::byte PacketDataSentinel = {std::byte{0xAA}};
constexpr std::byte InitializerSentinel = {std::byte{0x55}};

bool is_sentinel(std::byte value) { return value == InitializerSentinel; }
} // namespace

//
// Parse packet
//

TEST_CASE("parse_packet forwards Error from parse_header", "[ParsePacket]") {
  std::array<std::byte, spp::PrimaryHeaderSize + 1> vector = {};

  std::fill(vector.begin(), vector.end(), PacketDataSentinel);
  std::ranges::copy(FakeHeaderVector, vector.begin());
  vector[0] = std::byte{0x80};

  auto res = spp::parse_packet(vector);
  REQUIRE_FALSE(res.has_value());
  REQUIRE(res.error() == spp::Error::UnsupportedPacketVersionNumber);
}

TEST_CASE("parse_packet rejects too short input (missing Packet Data Field)",
          "[ParsePacket]") {
  std::array<std::byte, spp::PrimaryHeaderSize> vector = {};

  std::fill(vector.begin(), vector.end(), PacketDataSentinel);
  std::ranges::copy(FakeHeaderVector, vector.begin());

  auto res = spp::parse_packet(vector);
  REQUIRE_FALSE(res.has_value());
  REQUIRE(res.error() == spp::Error::TruncatedPacketDataField);
}

TEST_CASE("parse_packet rejects too short input (truncated Packet Data Field)",
          "[ParsePacket]") {
  std::array<std::byte, spp::PrimaryHeaderSize + 1> vector = {};

  std::fill(vector.begin(), vector.end(), PacketDataSentinel);
  std::ranges::copy(FakeHeaderVector, vector.begin());
  vector[5] = std::byte{0x01};

  auto res = spp::parse_packet(vector);
  REQUIRE_FALSE(res.has_value());
  REQUIRE(res.error() == spp::Error::TruncatedPacketDataField);
}

TEST_CASE("parse_packet parses minimum length packet", "[ParsePacket]") {
  std::array<std::byte, spp::PrimaryHeaderSize + 1> vector = {};

  std::fill(vector.begin(), vector.end(), PacketDataSentinel);
  std::ranges::copy(FakeHeaderVector, vector.begin());

  auto res = spp::parse_packet(vector);
  REQUIRE(res.has_value());
  REQUIRE(res->header == FakeHeader);
  REQUIRE(res->packet_data.data() ==
          std::span(vector)
              .subspan(spp::PrimaryHeaderSize,
                       vector.size() - spp::PrimaryHeaderSize)
              .data());
  REQUIRE(res->packet_data.size() == vector.size() - spp::PrimaryHeaderSize);
}

TEST_CASE("parse_packet parses maximum length packet", "[ParsePacket]") {
  std::array<std::byte, spp::PrimaryHeaderSize + 1 +
                            std::numeric_limits<std::uint16_t>::max()>
      vector = {};

  std::fill(vector.begin(), vector.end(), PacketDataSentinel);
  std::ranges::copy(FakeHeaderVector, vector.begin());
  vector[4] = std::byte{0xFF};
  vector[5] = std::byte{0xFF};

  auto header = FakeHeader;
  header.data_length = std::numeric_limits<std::uint16_t>::max();

  auto res = spp::parse_packet(vector);
  REQUIRE(res.has_value());
  REQUIRE(res->header == header);
  REQUIRE(res->packet_data.data() ==
          std::span(vector)
              .subspan(spp::PrimaryHeaderSize,
                       vector.size() - spp::PrimaryHeaderSize)
              .data());
  REQUIRE(res->packet_data.size() == vector.size() - spp::PrimaryHeaderSize);
}

TEST_CASE("parse_packet parses packet with extra bytes", "[ParsePacket]") {
  std::array<std::byte, spp::PrimaryHeaderSize + 2> vector = {};

  std::fill(vector.begin(), vector.end(), PacketDataSentinel);
  std::ranges::copy(FakeHeaderVector, vector.begin());

  auto res = spp::parse_packet(vector);
  REQUIRE(res.has_value());
  REQUIRE(res->header == FakeHeader);
  REQUIRE(res->packet_data.data() ==
          std::span(vector)
              .subspan(spp::PrimaryHeaderSize,
                       vector.size() - 1 - spp::PrimaryHeaderSize)
              .data());
  REQUIRE(res->packet_data.size() ==
          vector.size() - 1 - spp::PrimaryHeaderSize);
}

//
// Serialize packet
//

TEST_CASE("serialize_packet rejects packets whose Packet Data length (in "
          "header) does not match length of the Packet Data Field (too long)",
          "[SerializePacket]") {
  std::array<std::byte, spp::PrimaryHeaderSize + 2> dst = {};
  std::array<std::byte, 2> packet_data = {};

  std::fill(dst.begin(), dst.end(), InitializerSentinel);
  std::fill(packet_data.begin(), packet_data.end(), PacketDataSentinel);

  auto packet = FakePacket;
  packet.header.data_length = 0;
  packet.packet_data = packet_data;

  auto res = spp::serialize_packet(packet, dst);
  REQUIRE_FALSE(res.has_value());
  REQUIRE(res.error() == spp::Error::PacketDataLengthMismatch);
  REQUIRE(std::all_of(dst.begin(), dst.end(), is_sentinel));
}

TEST_CASE("serialize_packet rejects packets whose Packet Data length (in "
          "header) does not match length of the Packet Data Field (too short)",
          "[SerializePacket]") {
  std::array<std::byte, spp::PrimaryHeaderSize + 2> dst = {};
  std::array<std::byte, 1> packet_data = {};

  std::fill(dst.begin(), dst.end(), InitializerSentinel);
  std::fill(packet_data.begin(), packet_data.end(), PacketDataSentinel);

  auto packet = FakePacket;
  packet.header.data_length = 2;
  packet.packet_data = packet_data;

  auto res = spp::serialize_packet(packet, dst);
  REQUIRE_FALSE(res.has_value());
  REQUIRE(res.error() == spp::Error::PacketDataLengthMismatch);
  REQUIRE(std::all_of(dst.begin(), dst.end(), is_sentinel));
}

TEST_CASE("serialize_packet rejects packets whose Packet Data length (in "
          "header) does not match length of the Packet Data Field (empty)",
          "[SerializePacket]") {
  std::array<std::byte, spp::PrimaryHeaderSize + 1> dst = {};
  std::array<std::byte, 0> packet_data = {};

  std::fill(dst.begin(), dst.end(), InitializerSentinel);

  auto packet = FakePacket;
  packet.header.data_length = 0;
  packet.packet_data = packet_data;

  auto res = spp::serialize_packet(packet, dst);
  REQUIRE_FALSE(res.has_value());
  REQUIRE(res.error() == spp::Error::PacketDataLengthMismatch);
  REQUIRE(std::all_of(dst.begin(), dst.end(), is_sentinel));
}

TEST_CASE("serialize_packet rejects too short destination buffer",
          "[SerializePacket]") {
  std::array<std::byte, spp::PrimaryHeaderSize + 1> dst = {};
  std::array<std::byte, 2> packet_data = {};

  std::fill(dst.begin(), dst.end(), InitializerSentinel);
  std::fill(packet_data.begin(), packet_data.end(), PacketDataSentinel);

  auto packet = FakePacket;
  packet.header.data_length = 1;
  packet.packet_data = packet_data;

  auto res = spp::serialize_packet(packet, dst);
  REQUIRE_FALSE(res.has_value());
  REQUIRE(res.error() == spp::Error::DestinationBufferTooShort);
  REQUIRE(std::all_of(dst.begin(), dst.end(), is_sentinel));
}

TEST_CASE("serialize_packet forwards Error from serialize_header",
          "[SerializePacket]") {
  std::array<std::byte, spp::PrimaryHeaderSize + 1> dst = {};
  std::array<std::byte, 1> packet_data = {};

  std::fill(dst.begin(), dst.end(), InitializerSentinel);
  std::fill(packet_data.begin(), packet_data.end(), PacketDataSentinel);

  auto packet = FakePacket;
  // NOLINTBEGIN(clang-analyzer-optin.core.EnumCastOutOfRange)
  packet.header.type = static_cast<spp::PacketType>(
      static_cast<std::uint8_t>(spp::PacketType::Telecommand) + 1);
  // NOLINTEND(clang-analyzer-optin.core.EnumCastOutOfRange)
  packet.packet_data = packet_data;

  auto res = spp::serialize_packet(packet, dst);
  REQUIRE_FALSE(res.has_value());
  REQUIRE(res.error() == spp::Error::InvalidPacketType);
  REQUIRE(std::all_of(dst.begin(), dst.end(), is_sentinel));
}

TEST_CASE("serialize_packet serializes minimum length packet",
          "[SerializePacket]") {
  std::array<std::byte, spp::PrimaryHeaderSize + 1> dst = {};
  std::array<std::byte, 1> packet_data = {};

  std::fill(packet_data.begin(), packet_data.end(), PacketDataSentinel);

  auto packet = FakePacket;
  packet.packet_data = packet_data;

  auto res = spp::serialize_packet(packet, dst);
  REQUIRE(res.has_value());
  REQUIRE(*res == spp::PrimaryHeaderSize + packet_data.size());
  REQUIRE_THAT(std::span{dst}.subspan(0, spp::PrimaryHeaderSize),
               Catch::Matchers::RangeEquals(FakeHeaderVector));
  REQUIRE_THAT(std::span{dst}.subspan(spp::PrimaryHeaderSize,
                                      dst.size() - spp::PrimaryHeaderSize),
               Catch::Matchers::RangeEquals(packet_data));
}

TEST_CASE("serialize_packet serializes maximum length packet",
          "[SerializePacket]") {
  std::array<std::byte, spp::PrimaryHeaderSize + 1 +
                            std::numeric_limits<std::uint16_t>::max()>
      dst = {};
  std::array<std::byte, 1 + std::numeric_limits<std::uint16_t>::max()>
      packet_data = {};

  std::fill(packet_data.begin(), packet_data.end(), PacketDataSentinel);

  auto packet = FakePacket;
  packet.header.data_length = std::numeric_limits<std::uint16_t>::max();
  packet.packet_data = packet_data;

  auto vector = FakeHeaderVector;
  vector[4] = std::byte{0xFF};
  vector[5] = std::byte{0xFF};

  auto res = spp::serialize_packet(packet, dst);
  REQUIRE(res.has_value());
  REQUIRE(*res == spp::PrimaryHeaderSize + packet_data.size());
  REQUIRE_THAT(std::span{dst}.subspan(0, spp::PrimaryHeaderSize),
               Catch::Matchers::RangeEquals(vector));
  REQUIRE_THAT(std::span{dst}.subspan(spp::PrimaryHeaderSize,
                                      dst.size() - spp::PrimaryHeaderSize),
               Catch::Matchers::RangeEquals(packet_data));
}

TEST_CASE("serialize_packet serializes into larger destination buffer",
          "[SerializePacket]") {
  std::array<std::byte, spp::PrimaryHeaderSize + 2> dst = {};
  std::array<std::byte, 1> packet_data = {};

  std::fill(dst.begin(), dst.end(), InitializerSentinel);
  std::fill(packet_data.begin(), packet_data.end(), PacketDataSentinel);

  auto packet = FakePacket;
  packet.packet_data = packet_data;

  auto res = spp::serialize_packet(packet, dst);
  REQUIRE(res.has_value());
  REQUIRE(*res == spp::PrimaryHeaderSize + packet_data.size());
  REQUIRE_THAT(std::span{dst}.subspan(0, spp::PrimaryHeaderSize),
               Catch::Matchers::RangeEquals(FakeHeaderVector));
  REQUIRE_THAT(std::span{dst}.subspan(spp::PrimaryHeaderSize,
                                      dst.size() - 1 - spp::PrimaryHeaderSize),
               Catch::Matchers::RangeEquals(packet_data));
  REQUIRE(dst.back() == InitializerSentinel);
}

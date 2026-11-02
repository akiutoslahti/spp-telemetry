#include <spp/error.hpp>
#include <spp/primary_header.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <utility>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_tostring.hpp>

namespace Catch {
template <> struct StringMaker<spp::Error> {
  static std::string convert(spp::Error const &err) {
    return std::string(to_string(err));
  }
};
} // namespace Catch

//
// Apid
//

TEST_CASE("Apid create accepts 0 and ApidMax", "[Apid]") {
  auto apid_min = spp::Apid::create(0);
  REQUIRE(apid_min.has_value());
  REQUIRE(apid_min->get() == 0);

  auto apid_max = spp::Apid::create(spp::ApidMax);
  REQUIRE(apid_max.has_value());
  REQUIRE(apid_max->get() == spp::ApidMax);
}

TEST_CASE("Apid create rejects values above ApidMax", "[Apid]") {
  auto apid_too_large = spp::Apid::create(spp::ApidMax + 1U);
  REQUIRE_FALSE(apid_too_large.has_value());
  REQUIRE(apid_too_large.error() == spp::Error::InvalidApid);

  apid_too_large = spp::Apid::create(std::numeric_limits<std::uint16_t>::max());
  REQUIRE_FALSE(apid_too_large.has_value());
  REQUIRE(apid_too_large.error() == spp::Error::InvalidApid);
}

TEST_CASE("Apid create works at compile time", "[Apid]") {
  constexpr auto apid_min = spp::Apid::create(0);
  STATIC_REQUIRE(apid_min.has_value());
  STATIC_REQUIRE(apid_min->get() == 0);

  constexpr auto apid_too_large = spp::Apid::create(spp::ApidMax + 1U);
  STATIC_REQUIRE_FALSE(apid_too_large.has_value());
  STATIC_REQUIRE(apid_too_large.error() == spp::Error::InvalidApid);
}

TEST_CASE("Apid create factory can't throw", "[Apid]") {
  STATIC_REQUIRE(noexcept(spp::Apid::create(0)));
}

TEST_CASE("Apid get accessor can't throw", "[Apid]") {
  STATIC_REQUIRE(noexcept(std::declval<spp::Apid>().get()));
}

//
// Sequence Count
//

TEST_CASE("SequenceCount create accepts 0 and SequenceCountMax",
          "[SequenceCount]") {
  auto seq_count_min = spp::SequenceCount::create(0);
  REQUIRE(seq_count_min.has_value());
  REQUIRE(seq_count_min->get() == 0);

  auto seq_count_max = spp::SequenceCount::create(spp::SequenceCountMax);
  REQUIRE(seq_count_max.has_value());
  REQUIRE(seq_count_max->get() == spp::SequenceCountMax);
}

TEST_CASE("SequenceCount create rejects values above SequenceCountMax",
          "[SequenceCount]") {
  auto seq_count_too_large =
      spp::SequenceCount::create(spp::SequenceCountMax + 1U);
  REQUIRE_FALSE(seq_count_too_large.has_value());
  REQUIRE(seq_count_too_large.error() == spp::Error::InvalidSequenceCount);

  seq_count_too_large =
      spp::SequenceCount::create(std::numeric_limits<std::uint16_t>::max());
  REQUIRE_FALSE(seq_count_too_large.has_value());
  REQUIRE(seq_count_too_large.error() == spp::Error::InvalidSequenceCount);
}

TEST_CASE("SequenceCount create works at compile time", "[SequenceCount]") {
  constexpr auto seq_count_min = spp::SequenceCount::create(0);
  STATIC_REQUIRE(seq_count_min.has_value());
  STATIC_REQUIRE(seq_count_min->get() == 0);

  constexpr auto seq_count_too_large =
      spp::SequenceCount::create(spp::SequenceCountMax + 1U);
  STATIC_REQUIRE_FALSE(seq_count_too_large.has_value());
  STATIC_REQUIRE(seq_count_too_large.error() ==
                 spp::Error::InvalidSequenceCount);
}

TEST_CASE("SequenceCount create factory can't throw", "[SequenceCount]") {
  STATIC_REQUIRE(noexcept(spp::SequenceCount::create(0)));
}

TEST_CASE("SequenceCount get accessor can't throw", "[SequenceCount]") {
  STATIC_REQUIRE(noexcept(std::declval<spp::SequenceCount>().get()));
}

//
// Serialize header
//

//
// Parse header
//

TEST_CASE("parse_header rejects empty input", "[ParseHeader]") {
  std::array<std::byte, 0> arr = {};

  auto res = spp::parse_header(arr);
  REQUIRE(!res.has_value());
  REQUIRE(res.error() == spp::Error::PrimaryHeaderTooShort);
}

TEST_CASE("parse_header rejects input shorter than PrimaryHeaderSize",
          "[ParseHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize - 1> arr = {};

  auto res = spp::parse_header(arr);
  REQUIRE(!res.has_value());
  REQUIRE(res.error() == spp::Error::PrimaryHeaderTooShort);
}

TEST_CASE("parse_header rejects unsupported Packet Version Number",
          "[ParseHeader]") {
  const std::array<std::uint8_t, 3> vers = {0x20, 0x40, 0x80};
  std::array<std::byte, spp::PrimaryHeaderSize> arr = {};

  for (const std::uint8_t ver : vers) {
    auto ver_uint = static_cast<unsigned int>(ver);
    CAPTURE(ver_uint);
    arr[0] = std::byte{ver};
    auto res = spp::parse_header(arr);
    REQUIRE(!res.has_value());
    REQUIRE(res.error() == spp::Error::UnsupportedPacketVersionNumber);
  }
}

TEST_CASE("parse_header parses Packet Type correctly", "[ParseHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> arr = {};

  arr[0] = std::byte{0x10};

  auto res = spp::parse_header(arr);
  REQUIRE(res.has_value());
  REQUIRE(res->type == spp::PacketType::Telecommand);
}

TEST_CASE("parse_header parses Secondary Header Flag correctly",
          "[ParseHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> arr = {};

  arr[0] = std::byte{0x8};

  auto res = spp::parse_header(arr);
  REQUIRE(res.has_value());
  REQUIRE(res->secondary_header_flag == spp::SecondaryHeaderFlag::Present);
}

TEST_CASE("parse_header parses APID correctly", "[ParseHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> arr = {};

  arr[0] = std::byte{0x5};
  arr[1] = std::byte{0xAA};

  auto res = spp::parse_header(arr);
  REQUIRE(res.has_value());
  REQUIRE(res->apid.get() == 0x5AA);
}

TEST_CASE("parse_header parses Sequence Flags correctly", "[ParseHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> arr = {};

  arr[2] = std::byte{0x80};

  auto res = spp::parse_header(arr);
  REQUIRE(res.has_value());
  REQUIRE(res->sequence_flags == spp::SequenceFlags::LastSegment);
}

TEST_CASE("parse_header parses Packet Sequence Count correctly",
          "[ParseHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> arr = {};

  arr[2] = std::byte{0x2A};
  arr[3] = std::byte{0xAA};

  auto res = spp::parse_header(arr);
  REQUIRE(res.has_value());
  REQUIRE(res->sequence_count.get() == 0x2AAA);
}

TEST_CASE("parse_header parses Packet Data Length correctly", "[ParseHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> arr = {};

  arr[4] = std::byte{0xAA};
  arr[5] = std::byte{0xAA};

  auto res = spp::parse_header(arr);
  REQUIRE(res.has_value());
  REQUIRE(res->data_length == 0xAAAA);
}

TEST_CASE("parse_header accepts all zeros", "[ParseHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> arr = {};

  auto res = spp::parse_header(arr);
  REQUIRE(res.has_value());

  REQUIRE(res->type == spp::PacketType::Telemetry);
  REQUIRE(res->secondary_header_flag == spp::SecondaryHeaderFlag::NotPresent);
  REQUIRE(res->apid.get() == 0);
  REQUIRE(res->sequence_flags == spp::SequenceFlags::ContinuationSegment);
  REQUIRE(res->sequence_count.get() == 0);
  REQUIRE(res->data_length == 0);
}

TEST_CASE("parse_header accepts all ones (except Packet Version Number)",
          "[ParseHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> arr = {
      std::byte{0x1F}, std::byte{0xFF}, std::byte{0xFF},
      std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}};

  auto res = spp::parse_header(arr);
  REQUIRE(res.has_value());

  REQUIRE(res->type == spp::PacketType::Telecommand);
  REQUIRE(res->secondary_header_flag == spp::SecondaryHeaderFlag::Present);
  REQUIRE(res->apid.get() == spp::ApidMax);
  REQUIRE(res->sequence_flags == spp::SequenceFlags::Unsegmented);
  REQUIRE(res->sequence_count.get() == spp::SequenceCountMax);
  REQUIRE(res->data_length == std::numeric_limits<std::uint16_t>::max());
}

TEST_CASE("parse_header parses golden vector correctly", "[ParseHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> arr = {
      std::byte{0x15}, std::byte{0x55}, std::byte{0x55},
      std::byte{0x55}, std::byte{0x55}, std::byte{0x55}};

  auto res = spp::parse_header(arr);
  REQUIRE(res.has_value());

  REQUIRE(res->type == spp::PacketType::Telecommand);
  REQUIRE(res->secondary_header_flag == spp::SecondaryHeaderFlag::NotPresent);
  REQUIRE(res->apid.get() == 0x555);
  REQUIRE(res->sequence_flags == spp::SequenceFlags::FirstSegment);
  REQUIRE(res->sequence_count.get() == 0x1555);
  REQUIRE(res->data_length == 0x5555);
}

TEST_CASE("parse_header accepts input longer than PrimaryHeaderSize",
          "[ParseHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize + 1> arr = {};

  arr[arr.size() - 1] = std::byte{0xFF};

  auto res = spp::parse_header(arr);
  REQUIRE(res.has_value());

  REQUIRE(res->type == spp::PacketType::Telemetry);
  REQUIRE(res->secondary_header_flag == spp::SecondaryHeaderFlag::NotPresent);
  REQUIRE(res->apid.get() == 0);
  REQUIRE(res->sequence_flags == spp::SequenceFlags::ContinuationSegment);
  REQUIRE(res->sequence_count.get() == 0);
  REQUIRE(res->data_length == 0);
}

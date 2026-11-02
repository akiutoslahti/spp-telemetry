#include <spp/error.hpp>
#include <spp/expected.hpp>
#include <spp/primary_header.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_tostring.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>

namespace Catch {
template <> struct StringMaker<spp::Error> {
  static std::string convert(spp::Error const &err) {
    return std::string(to_string(err));
  }
};

template <> struct StringMaker<spp::PacketType> {
  static std::string convert(spp::PacketType const &type) {
    switch (type) {
    case spp::PacketType::Telemetry:
      return "Telemetry";
    case spp::PacketType::Telecommand:
      return "Telecommand";
    }
    return std::format("Unknown {:#x}", static_cast<unsigned int>(type));
  }
};

template <> struct StringMaker<spp::SecondaryHeaderFlag> {
  static std::string convert(spp::SecondaryHeaderFlag const &flag) {
    switch (flag) {
    case spp::SecondaryHeaderFlag::NotPresent:
      return "NotPresent";
    case spp::SecondaryHeaderFlag::Present:
      return "Present";
    }
    return std::format("Unknown {:#x}", static_cast<unsigned int>(flag));
  }
};

template <> struct StringMaker<spp::SequenceFlags> {
  static std::string convert(spp::SequenceFlags const &flags) {
    switch (flags) {
    case spp::SequenceFlags::ContinuationSegment:
      return "ContinuationSegment";
    case spp::SequenceFlags::FirstSegment:
      return "FirstSegment";
    case spp::SequenceFlags::LastSegment:
      return "LastSegment";
    case spp::SequenceFlags::Unsegmented:
      return "Unsegmented";
    }
    return std::format("Unknown {:#x}", static_cast<unsigned int>(flags));
  }
};

template <> struct StringMaker<spp::PacketPrimaryHeader> {
  static std::string convert(spp::PacketPrimaryHeader const &header) {
    return std::format(
        "PacketPrimaryHeader(type='{}', secondary_header_flag='{}', "
        "apid={:#x}, "
        "sequence_flags='{}', sequence_count={:#x}, data_length={:#x})",
        Catch::Detail::stringify(header.type),
        Catch::Detail::stringify(header.secondary_header_flag),
        header.apid.get(), Catch::Detail::stringify(header.sequence_flags),
        header.sequence_count.get(), header.data_length);
  }
};
} // namespace Catch

namespace {
constexpr spp::Expected<spp::Apid, spp::Error> GoldenVectorApid =
    spp::Apid::create(0x555);
constexpr spp::Expected<spp::SequenceCount, spp::Error>
    GoldenVectorSequenceCount = spp::SequenceCount::create(0x1555);
constexpr spp::PacketPrimaryHeader GoldenVectorHeader = {
    .type = spp::PacketType::Telecommand,
    .secondary_header_flag = spp::SecondaryHeaderFlag::NotPresent,
    .apid = *GoldenVectorApid,
    .sequence_flags = spp::SequenceFlags::FirstSegment,
    .sequence_count = *GoldenVectorSequenceCount,
    .data_length = 0x5555};
constexpr std::array<std::byte, spp::PrimaryHeaderSize> GoldenVector = {
    std::byte{0x15}, std::byte{0x55}, std::byte{0x55},
    std::byte{0x55}, std::byte{0x55}, std::byte{0x55}};

constexpr spp::Expected<spp::Apid, spp::Error> MinVectorApid =
    spp::Apid::create(0);
constexpr spp::Expected<spp::SequenceCount, spp::Error> MinVectorSequenceCount =
    spp::SequenceCount::create(0);
constexpr spp::PacketPrimaryHeader MinVectorHeader = {
    .type = spp::PacketType::Telemetry,
    .secondary_header_flag = spp::SecondaryHeaderFlag::NotPresent,
    .apid = *MinVectorApid,
    .sequence_flags = spp::SequenceFlags::ContinuationSegment,
    .sequence_count = *MinVectorSequenceCount,
    .data_length = 0x0000};
constexpr std::array<std::byte, spp::PrimaryHeaderSize> MinVector = {};

constexpr spp::Expected<spp::Apid, spp::Error> MaxVectorApid =
    spp::Apid::create(spp::ApidMax - 1);
constexpr spp::Expected<spp::SequenceCount, spp::Error> MaxVectorSequenceCount =
    spp::SequenceCount::create(spp::SequenceCountMax);
constexpr spp::PacketPrimaryHeader MaxVectorHeader = {
    .type = spp::PacketType::Telecommand,
    .secondary_header_flag = spp::SecondaryHeaderFlag::Present,
    .apid = *MaxVectorApid,
    .sequence_flags = spp::SequenceFlags::Unsegmented,
    .sequence_count = *MaxVectorSequenceCount,
    .data_length = 0xFFFF};
constexpr std::array<std::byte, spp::PrimaryHeaderSize> MaxVector = {
    std::byte{0x1F}, std::byte{0xFE}, std::byte{0xFF},
    std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}};

constexpr std::byte SentinelByte = std::byte{0xAA};
constexpr std::array<std::byte, spp::PrimaryHeaderSize> SentinelVector = {
    SentinelByte, SentinelByte, SentinelByte,
    SentinelByte, SentinelByte, SentinelByte};
} // namespace

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

  apid_too_large = spp::Apid::create(spp::ApidMax + 1);
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

TEST_CASE("Apid operator== works", "[Apid]") {
  constexpr auto apid_min = spp::Apid::create(0);
  constexpr auto apid_max = spp::Apid::create(spp::ApidMax);

  STATIC_REQUIRE(*apid_min == *apid_min);
  STATIC_REQUIRE_FALSE(*apid_min == *apid_max);
}

//
// Packet Sequence Count
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

  seq_count_too_large = spp::SequenceCount::create(spp::SequenceCountMax + 1);
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

TEST_CASE("SequenceCount operator== works", "[SequenceCount]") {
  constexpr auto seq_count_min = spp::SequenceCount::create(0);
  constexpr auto seq_count_max =
      spp::SequenceCount::create(spp::SequenceCountMax);

  STATIC_REQUIRE(*seq_count_min == *seq_count_min);
  STATIC_REQUIRE_FALSE(*seq_count_min == *seq_count_max);
}

//
// Packet Primary Header
//
TEST_CASE("PacketPrimaryHeader operator== works", "[PacketPrimaryHeader]") {
  STATIC_REQUIRE(MinVectorHeader == MinVectorHeader);
  STATIC_REQUIRE_FALSE(MinVectorHeader == MaxVectorHeader);
}

TEST_CASE("PacketPrimaryHeader operator== detects mismatch in Packet Type",
          "[PacketPrimaryHeader]") {
  constexpr spp::PacketPrimaryHeader header = [] {
    spp::PacketPrimaryHeader ret = MinVectorHeader;
    ret.type = spp::PacketType::Telecommand;
    return ret;
  }();

  STATIC_REQUIRE_FALSE(MinVectorHeader == header);
}

TEST_CASE(
    "PacketPrimaryHeader operator== detects mismatch in Secondary Header Flag",
    "[PacketPrimaryHeader]") {
  constexpr spp::PacketPrimaryHeader header = [] {
    spp::PacketPrimaryHeader ret = MinVectorHeader;
    ret.secondary_header_flag = spp::SecondaryHeaderFlag::Present;
    return ret;
  }();

  STATIC_REQUIRE_FALSE(MinVectorHeader == header);
}

TEST_CASE("PacketPrimaryHeader operator== detects mismatch in APID",
          "[PacketPrimaryHeader]") {
  constexpr spp::PacketPrimaryHeader header = [] {
    spp::PacketPrimaryHeader ret = MinVectorHeader;
    ret.apid = *MaxVectorApid;
    return ret;
  }();

  STATIC_REQUIRE_FALSE(MinVectorHeader == header);
}

TEST_CASE("PacketPrimaryHeader operator== detects mismatch in Sequence Flags",
          "[PacketPrimaryHeader]") {
  constexpr spp::PacketPrimaryHeader header = [] {
    spp::PacketPrimaryHeader ret = MinVectorHeader;
    ret.sequence_flags = spp::SequenceFlags::FirstSegment;
    return ret;
  }();

  STATIC_REQUIRE_FALSE(MinVectorHeader == header);
}

TEST_CASE(
    "PacketPrimaryHeader operator== detects mismatch in Packet Sequence Count",
    "[PacketPrimaryHeader]") {
  constexpr spp::PacketPrimaryHeader header = [] {
    spp::PacketPrimaryHeader ret = MinVectorHeader;
    ret.sequence_count = *MaxVectorSequenceCount;
    return ret;
  }();

  STATIC_REQUIRE_FALSE(MinVectorHeader == header);
}

TEST_CASE(
    "PacketPrimaryHeader operator== detects mismatch in Packet Data Length",
    "[PacketPrimaryHeader]") {
  constexpr spp::PacketPrimaryHeader header = [] {
    spp::PacketPrimaryHeader ret = MinVectorHeader;
    ret.data_length = 0xFFFF;
    return ret;
  }();

  STATIC_REQUIRE_FALSE(MinVectorHeader == header);
}

//
// Serialize header
//

TEST_CASE("serialize_header rejects empty destination buffer with "
          "DestinationBufferTooShort",
          "[SerializeHeader]") {
  std::array<std::byte, 0> dst = {};

  auto res = spp::serialize_header(MinVectorHeader, dst);
  REQUIRE_FALSE(res.has_value());
  REQUIRE(res.error() == spp::Error::DestinationBufferTooShort);
}

TEST_CASE("serialize_header rejects too short destination buffer with "
          "DestinationBufferTooShort",
          "[SerializeHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize - 1> dst = {};

  auto res = spp::serialize_header(MinVectorHeader, dst);
  REQUIRE_FALSE(res.has_value());
  REQUIRE(res.error() == spp::Error::DestinationBufferTooShort);
}

TEST_CASE("serialize_header serializes golden vector correctly",
          "[SerializeHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> dst = SentinelVector;

  auto res = spp::serialize_header(GoldenVectorHeader, dst);
  REQUIRE(res.has_value());
  REQUIRE(*res == spp::PrimaryHeaderSize);
  REQUIRE_THAT(dst, Catch::Matchers::RangeEquals(GoldenVector));
}

TEST_CASE("serialize_header serializes minimum value header correctly",
          "[SerializeHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> dst = SentinelVector;

  auto res = spp::serialize_header(MinVectorHeader, dst);
  REQUIRE(res.has_value());
  REQUIRE(*res == spp::PrimaryHeaderSize);
  REQUIRE_THAT(dst, Catch::Matchers::RangeEquals(MinVector));
}

TEST_CASE("serialize_header serializes maximum value header correctly",
          "[SerializeHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> dst = SentinelVector;

  auto res = spp::serialize_header(MaxVectorHeader, dst);
  REQUIRE(res.has_value());
  REQUIRE(*res == spp::PrimaryHeaderSize);
  REQUIRE_THAT(dst, Catch::Matchers::RangeEquals(MaxVector));
}

TEST_CASE(
    "serialize_header accepts destination buffer longer than PrimaryHeaderSize",
    "[SerializeHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize + 1> dst = {
      SentinelByte, SentinelByte, SentinelByte, SentinelByte,
      SentinelByte, SentinelByte, SentinelByte};

  auto res = spp::serialize_header(GoldenVectorHeader, dst);
  REQUIRE(res.has_value());
  REQUIRE(*res == spp::PrimaryHeaderSize);
  REQUIRE_THAT(std::span{dst}.first<spp::PrimaryHeaderSize>(),
               Catch::Matchers::RangeEquals(GoldenVector));
  REQUIRE(dst.back() == SentinelByte);
}

TEST_CASE("serialize_header rejects unknown Packet Type with InvalidPacketType",
          "[SerializeHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> dst = SentinelVector;

  spp::PacketPrimaryHeader header = GoldenVectorHeader;
  // NOLINTBEGIN(clang-analyzer-optin.core.EnumCastOutOfRange)
  header.type = static_cast<spp::PacketType>(
      static_cast<std::underlying_type_t<spp::PacketType>>(
          spp::PacketType::Telecommand) +
      1);
  // NOLINTEND(clang-analyzer-optin.core.EnumCastOutOfRange)

  auto res = spp::serialize_header(header, dst);
  REQUIRE_FALSE(res.has_value());
  REQUIRE(res.error() == spp::Error::InvalidPacketType);
  REQUIRE_THAT(dst, Catch::Matchers::RangeEquals(SentinelVector));
}

TEST_CASE("serialize_header rejects unknown Secondary Header Flag with "
          "InvalidSecondaryHeaderFlag",
          "[SerializeHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> dst = SentinelVector;

  spp::PacketPrimaryHeader header = GoldenVectorHeader;
  // NOLINTBEGIN(clang-analyzer-optin.core.EnumCastOutOfRange)
  header.secondary_header_flag = static_cast<spp::SecondaryHeaderFlag>(
      static_cast<std::underlying_type_t<spp::SecondaryHeaderFlag>>(
          spp::SecondaryHeaderFlag::Present) +
      1);
  // NOLINTEND(clang-analyzer-optin.core.EnumCastOutOfRange)

  auto res = spp::serialize_header(header, dst);
  REQUIRE_FALSE(res.has_value());
  REQUIRE(res.error() == spp::Error::InvalidSecondaryHeaderFlag);
  REQUIRE_THAT(dst, Catch::Matchers::RangeEquals(SentinelVector));
}

TEST_CASE(
    "serialize_header rejects unknown Sequence Flags with InvalidSequenceFlags",
    "[SerializeHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> dst = SentinelVector;

  spp::PacketPrimaryHeader header = GoldenVectorHeader;
  // NOLINTBEGIN(clang-analyzer-optin.core.EnumCastOutOfRange)
  header.sequence_flags = static_cast<spp::SequenceFlags>(
      static_cast<std::underlying_type_t<spp::SequenceFlags>>(
          spp::SequenceFlags::Unsegmented) +
      1);
  // NOLINTEND(clang-analyzer-optin.core.EnumCastOutOfRange)

  auto res = spp::serialize_header(header, dst);
  REQUIRE_FALSE(res.has_value());
  REQUIRE(res.error() == spp::Error::InvalidSequenceFlags);
  REQUIRE_THAT(dst, Catch::Matchers::RangeEquals(SentinelVector));
}

//
// Parse header
//

TEST_CASE("parse_header rejects empty input", "[ParseHeader]") {
  std::array<std::byte, 0> arr = {};

  auto res = spp::parse_header(arr);
  REQUIRE_FALSE(res.has_value());
  REQUIRE(res.error() == spp::Error::PrimaryHeaderTooShort);
}

TEST_CASE("parse_header rejects input shorter than PrimaryHeaderSize",
          "[ParseHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize - 1> arr = {};

  auto res = spp::parse_header(arr);
  REQUIRE_FALSE(res.has_value());
  REQUIRE(res.error() == spp::Error::PrimaryHeaderTooShort);
}

TEST_CASE("parse_header rejects unsupported Packet Version Number",
          "[ParseHeader]") {
  const std::array<std::uint8_t, 3> vers = {0x20, 0x40, 0x80};
  std::array<std::byte, spp::PrimaryHeaderSize> arr = MinVector;

  for (const std::uint8_t ver : vers) {
    auto ver_uint = static_cast<unsigned int>(ver);
    CAPTURE(ver_uint);
    arr[0] = std::byte{ver};
    auto res = spp::parse_header(arr);
    REQUIRE_FALSE(res.has_value());
    REQUIRE(res.error() == spp::Error::UnsupportedPacketVersionNumber);
  }
}

TEST_CASE("parse_header parses Packet Type correctly", "[ParseHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> arr = MinVector;

  arr[0] = std::byte{0x10};

  auto res = spp::parse_header(arr);
  REQUIRE(res.has_value());
  REQUIRE(res->type == spp::PacketType::Telecommand);
}

TEST_CASE("parse_header parses Secondary Header Flag correctly",
          "[ParseHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> arr = MinVector;

  arr[0] = std::byte{0x8};

  auto res = spp::parse_header(arr);
  REQUIRE(res.has_value());
  REQUIRE(res->secondary_header_flag == spp::SecondaryHeaderFlag::Present);
}

TEST_CASE("parse_header parses APID correctly", "[ParseHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> arr = MinVector;

  arr[0] = std::byte{0x5};
  arr[1] = std::byte{0xAA};

  auto res = spp::parse_header(arr);
  REQUIRE(res.has_value());
  REQUIRE(res->apid.get() == 0x5AA);
}

TEST_CASE("parse_header parses Sequence Flags correctly", "[ParseHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> arr = MinVector;

  arr[2] = std::byte{0x80};

  auto res = spp::parse_header(arr);
  REQUIRE(res.has_value());
  REQUIRE(res->sequence_flags == spp::SequenceFlags::LastSegment);
}

TEST_CASE("parse_header parses Packet Sequence Count correctly",
          "[ParseHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> arr = MinVector;

  arr[2] = std::byte{0x2A};
  arr[3] = std::byte{0xAA};

  auto res = spp::parse_header(arr);
  REQUIRE(res.has_value());
  REQUIRE(res->sequence_count.get() == 0x2AAA);
}

TEST_CASE("parse_header parses Packet Data Length correctly", "[ParseHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> arr = MinVector;

  arr[4] = std::byte{0xAA};
  arr[5] = std::byte{0xAA};

  auto res = spp::parse_header(arr);
  REQUIRE(res.has_value());
  REQUIRE(res->data_length == 0xAAAA);
}

TEST_CASE("parse_header parses minimum vector correctly", "[ParseHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize> arr = MinVector;

  auto res = spp::parse_header(arr);
  REQUIRE(res.has_value());
  REQUIRE(*res == MinVectorHeader);
}

TEST_CASE("parse_header parses maximum vector correctly", "[ParseHeader]") {

  std::array<std::byte, spp::PrimaryHeaderSize> arr = MaxVector;

  auto res = spp::parse_header(arr);
  REQUIRE(res.has_value());
  REQUIRE(*res == MaxVectorHeader);
}

TEST_CASE("parse_header parses golden vector correctly", "[ParseHeader]") {
  auto res = spp::parse_header(GoldenVector);
  REQUIRE(res.has_value());
  REQUIRE(*res == GoldenVectorHeader);
}

TEST_CASE("parse_header accepts input longer than PrimaryHeaderSize",
          "[ParseHeader]") {
  std::array<std::byte, spp::PrimaryHeaderSize + 1> arr = {};

  arr[arr.size() - 1] = std::byte{0xFF};

  auto res = spp::parse_header(arr);
  REQUIRE(res.has_value());
  REQUIRE(*res == MinVectorHeader);
}

//
// Conformance
//

TEST_CASE("serialize_header rejects Idle packets with Secondary Header",
          "[Conformance]") {
  std::array<std::byte, spp::PrimaryHeaderSize> dst = SentinelVector;
  spp::PacketPrimaryHeader header = GoldenVectorHeader;
  header.secondary_header_flag = spp::SecondaryHeaderFlag::Present;
  header.apid = *spp::Apid::create(spp::ApidIdlePacket);

  auto res = spp::serialize_header(header, dst);
  REQUIRE_FALSE(res.has_value());
  REQUIRE(res.error() == spp::Error::IdlePacketWithSecondaryHeader);
  REQUIRE_THAT(dst, Catch::Matchers::RangeEquals(SentinelVector));
}

TEST_CASE("parse_header rejects Idle packets with Secondary Header",
          "[Conformance]") {
  std::array<std::byte, spp::PrimaryHeaderSize> src = GoldenVector;

  src[0] = std::byte{0x1F};
  src[1] = std::byte{0xFF};

  auto res = spp::parse_header(src);
  REQUIRE_FALSE(res.has_value());
  REQUIRE(res.error() == spp::Error::IdlePacketWithSecondaryHeader);
}

//
// Random roundtrips
//

TEST_CASE("Random parse -> serialize roundtrip", "[Roundtrip]") {
  // 1000 iterations gives near-certain coverage of every bit and every pair of
  // bits
  std::array<std::byte, spp::PrimaryHeaderSize> random_vector = GENERATE(
      take(1000, map(
                     [](std::vector<std::uint8_t> const &vec) {
                       std::array<std::byte, spp::PrimaryHeaderSize> arr{};
                       std::ranges::transform(
                           vec, arr.begin(),
                           [](std::uint8_t val) { return std::byte{val}; });
                       // Packet Version Number (3b) can't be random; reset them
                       arr[0] &= std::byte{0x1FU};
                       if ((arr[0] & std::byte{0xFU}) == std::byte{0xFU} &&
                           arr[1] == std::byte{0xFFU}) {
                         // Idle Packets can't have Secondary Header, so
                         // flip Secondary Header off, or
                         arr[0] ^= std::byte{0x8U};
                         // decrement APID by one
                         // arr[1] ^= std::byte{0x1U};
                       }
                       return arr;
                     },
                     chunk(spp::PrimaryHeaderSize,
                           random<std::uint8_t>(
                               0, std::numeric_limits<std::uint8_t>::max())))));

  CAPTURE(random_vector);
  auto header = spp::parse_header(random_vector);
  REQUIRE(header.has_value());

  std::array<std::byte, spp::PrimaryHeaderSize> dst = SentinelVector;
  auto res = spp::serialize_header(*header, dst);
  REQUIRE(res.has_value());
  REQUIRE(*res == spp::PrimaryHeaderSize);
  REQUIRE_THAT(dst, Catch::Matchers::RangeEquals(random_vector));
}

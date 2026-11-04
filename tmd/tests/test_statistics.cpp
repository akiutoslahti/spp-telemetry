#include <spp/error.hpp>
#include <spp/expected.hpp>
#include <spp/packet.hpp>
#include <spp/primary_header.hpp>
#include <tmd/statistics.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <string>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_tostring.hpp>

namespace Catch {
template <> struct StringMaker<tmd::StatisticsEntry> {
  static std::string convert(tmd::StatisticsEntry const &entry) {
    return to_string(entry);
  }
};
} // namespace Catch

namespace {
constexpr spp::Expected<spp::Apid, spp::Error> FakeApid =
    spp::Apid::create(0x0);
constexpr spp::Expected<spp::SequenceCount, spp::Error> FakeSequenceCount =
    spp::SequenceCount::create(0x0);
constexpr spp::PacketPrimaryHeader FakeHeader = {
    .type = spp::PacketType::Telemetry,
    .secondary_header_flag = spp::SecondaryHeaderFlag::NotPresent,
    .apid = *FakeApid,
    .sequence_flags = spp::SequenceFlags::Unsegmented,
    .sequence_count = *FakeSequenceCount,
    .data_length = 0x0};
constexpr std::array<std::byte, 1> FakePacketData = {std::byte{0xAA}};
constexpr spp::Packet FakePacket =
    spp::Packet{.header = FakeHeader, .packet_data = FakePacketData};
constexpr std::size_t FakePacketSize =
    spp::PrimaryHeaderSize + FakePacketData.size();

constexpr tmd::StatisticsEntry EmptyStatEntry = {.nr_packets = 0,
                                                 .nr_bytes = 0,
                                                 .nr_gap_events = 0,
                                                 .nr_lost_packets = 0,
                                                 .nr_duplicates = 0};

class StatisticsFixture {
public:
  // Fixture to avoid placing local Statistics to each test case
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  tmd::Statistics stats;
  constexpr StatisticsFixture() noexcept = default;
};
} // namespace

TEST_CASE_METHOD(StatisticsFixture,
                 "Get returns nullopt for APID that has not been seen",
                 "[Statistics]") {
  REQUIRE(stats.get(*FakeApid) == std::nullopt);
}

TEST_CASE_METHOD(
    StatisticsFixture,
    "First packet on a new APID is counted, and not reported as a gap",
    "[Statistics]") {
  auto expected = EmptyStatEntry;

  expected.nr_packets = 1;
  expected.nr_bytes = FakePacketSize;
  stats.record(FakePacket);

  REQUIRE(stats.get(*FakeApid) == expected);
}

TEST_CASE_METHOD(StatisticsFixture,
                 "Packets arriving in order: packet and byte totals add up",
                 "[Statistics]") {
  auto expected = EmptyStatEntry;
  expected.nr_packets = 2;
  expected.nr_bytes = 2 * FakePacketSize;

  stats.record(FakePacket);

  auto packet = FakePacket;
  packet.header.sequence_count =
      *spp::SequenceCount::create(packet.header.sequence_count.next());
  stats.record(packet);

  REQUIRE(stats.get(*FakeApid) == expected);
}

TEST_CASE_METHOD(StatisticsFixture,
                 "Gap: the gap and lost-packet counts are right",
                 "[Statistics]") {
  auto expected = EmptyStatEntry;
  expected.nr_packets = 2;
  expected.nr_bytes = 2 * FakePacketSize;
  expected.nr_gap_events = 1;
  expected.nr_lost_packets = 2;

  stats.record(FakePacket);

  auto packet = FakePacket;
  packet.header.sequence_count =
      *spp::SequenceCount::create(*FakeSequenceCount.value() + 3);
  stats.record(packet);

  REQUIRE(stats.get(*FakeApid) == expected);
}

TEST_CASE_METHOD(
    StatisticsFixture,
    "Wraparound: going from count 16383 to 0 is in order, not a gap",
    "[Statistics]") {
  auto expected = EmptyStatEntry;
  expected.nr_packets = 2;
  expected.nr_bytes = 2 * FakePacketSize;

  auto packet = FakePacket;
  packet.header.sequence_count =
      *spp::SequenceCount::create(spp::SequenceCountMax);
  stats.record(packet);

  packet.header.sequence_count =
      *spp::SequenceCount::create(packet.header.sequence_count.next());
  stats.record(packet);

  REQUIRE(stats.get(*FakeApid) == expected);
}

TEST_CASE_METHOD(StatisticsFixture,
                 "Duplicate counts as a packet and as a duplicate",
                 "[Statistics]") {
  auto expected = EmptyStatEntry;
  expected.nr_packets = 2;
  expected.nr_bytes = 2 * FakePacketSize;
  expected.nr_duplicates = 1;

  stats.record(FakePacket);
  stats.record(FakePacket);

  REQUIRE(stats.get(*FakeApid) == expected);
}

TEST_CASE_METHOD(StatisticsFixture, "Two APIDs stay independent of each other",
                 "[Statistics]") {
  auto expected = EmptyStatEntry;
  expected.nr_packets = 1;
  expected.nr_bytes = FakePacketSize;

  stats.record(FakePacket);

  auto packet = FakePacket;
  auto second_apid = spp::Apid::create(*FakeApid.value() + 1);
  packet.header.apid = *second_apid;
  stats.record(packet);

  REQUIRE(stats.get(*FakeApid) == expected);
  REQUIRE(stats.get(*second_apid) == expected);
}

TEST_CASE_METHOD(StatisticsFixture, "Idle Packet: gaps ignored",
                 "[Statistics]") {
  auto expected = EmptyStatEntry;
  expected.nr_packets = 2;
  expected.nr_bytes = 2 * FakePacketSize;

  auto packet = FakePacket;
  auto idle_apid = spp::Apid::create(spp::ApidIdlePacket);
  packet.header.apid = *idle_apid;
  stats.record(packet);

  packet.header.sequence_count =
      *spp::SequenceCount::create(*FakeSequenceCount.value() + 3);
  stats.record(packet);

  REQUIRE(stats.get(*idle_apid) == expected);
}

TEST_CASE_METHOD(StatisticsFixture, "Idle Packet: duplicates ignored",
                 "[Statistics]") {
  auto expected = EmptyStatEntry;
  expected.nr_packets = 2;
  expected.nr_bytes = 2 * FakePacketSize;

  auto packet = FakePacket;
  auto idle_apid = spp::Apid::create(spp::ApidIdlePacket);
  packet.header.apid = *idle_apid;
  stats.record(packet);
  stats.record(packet);

  REQUIRE(stats.get(*idle_apid) == expected);
}

TEST_CASE_METHOD(StatisticsFixture,
                 "Each malformed Error kind lands in its own counter",
                 "[Statistics]") {
  std::array<std::uint64_t, spp::NrErrors> expected = {};
  expected[static_cast<size_t>(spp::Error::InvalidApid)] = 1;
  expected[static_cast<size_t>(spp::Error::InvalidSequenceFlags)] = 2;

  stats.record(spp::Unexpected(spp::Error::InvalidApid));
  stats.record(spp::Unexpected(spp::Error::InvalidSequenceFlags));
  stats.record(spp::Unexpected(spp::Error::InvalidSequenceFlags));

  REQUIRE(stats.get_errors() == expected);
}

TEST_CASE_METHOD(StatisticsFixture,
                 "Reset clears the counters: next packet isn't a gap and error "
                 "counts are valid",
                 "[Statistics]") {
  auto expected_stats = EmptyStatEntry;
  expected_stats.nr_packets = 1;
  expected_stats.nr_bytes = FakePacketSize;
  expected_stats.nr_gap_events = 0;
  expected_stats.nr_lost_packets = 0;
  std::array<std::uint64_t, spp::NrErrors> expected_errors = {};
  expected_errors[static_cast<size_t>(spp::Error::InvalidPacketType)] = 1;

  stats.record(FakePacket);
  stats.record(spp::Unexpected(spp::Error::DestinationBufferTooShort));

  stats.reset();

  auto packet = FakePacket;
  packet.header.sequence_count =
      *spp::SequenceCount::create(*FakeSequenceCount.value() + 3);
  stats.record(packet);
  stats.record(spp::Unexpected(spp::Error::InvalidPacketType));

  REQUIRE(stats.get(*FakeApid) == expected_stats);
  REQUIRE(stats.get_errors() == expected_errors);
}

TEST_CASE_METHOD(StatisticsFixture, "Report: empty", "[Statistics]") {
  REQUIRE(stats.report() == "Valid packets: None\nErrors: None");
}

TEST_CASE_METHOD(StatisticsFixture, "Report: two in-order packets, no errors",
                 "[Statistics]") {
  stats.record(FakePacket);

  auto packet = FakePacket;
  packet.header.sequence_count =
      *spp::SequenceCount::create(packet.header.sequence_count.next());
  stats.record(packet);

  auto entry = EmptyStatEntry;
  entry.nr_packets = 2;
  entry.nr_bytes = 2 * FakePacketSize;

  REQUIRE(stats.report() ==
          std::format("Valid packets:\n  APID={:#x}, {}\nErrors: None",
                      *FakeApid.value(), to_string(entry)));
}

TEST_CASE_METHOD(StatisticsFixture,
                 "Report: no packet, two errors of same kind", "[Statistics]") {
  stats.record(spp::Unexpected(spp::Error::InvalidPacketType));
  stats.record(spp::Unexpected(spp::Error::InvalidPacketType));
  REQUIRE(stats.report() ==
          std::format("Valid packets: None\nErrors: '{}'={}",
                      to_string(spp::Error::InvalidPacketType), 2));
}

TEST_CASE_METHOD(
    StatisticsFixture,
    "Report: two APIDs and two error kinds, sorted, with separators",
    "[Statistics]") {
  auto packet = FakePacket;
  packet.header.apid = *spp::Apid::create(spp::ApidMax);
  stats.record(packet);

  packet = FakePacket;
  stats.record(packet);
  packet.header.sequence_count = *spp::SequenceCount::create(4);
  stats.record(packet);
  stats.record(packet);
  stats.record(packet);

  stats.record(spp::Unexpected(spp::Error::InvalidSequenceCount));
  stats.record(spp::Unexpected(spp::Error::InvalidSequenceCount));
  stats.record(spp::Unexpected(spp::Error::InvalidApid));
  stats.record(spp::Unexpected(spp::Error::InvalidApid));
  stats.record(spp::Unexpected(spp::Error::InvalidApid));

  const std::string expected =
      "Valid packets:\n  APID=0x0, packets=4, bytes=28, gaps=1, lost=3, "
      "duplicates=2\n  APID=0x7ff, packets=1, bytes=7, gaps=0, lost=0, "
      "duplicates=0\nErrors: 'Invalid APID'=3, 'Invalid Sequence Count'=2";

  REQUIRE(stats.report() == expected);
}

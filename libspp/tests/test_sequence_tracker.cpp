#include <spp/error.hpp>
#include <spp/expected.hpp>
#include <spp/primary_header.hpp>
#include <spp/sequence_tracker.hpp>

#include <format>
#include <string>
#include <type_traits>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_tostring.hpp>

namespace {
constexpr spp::Expected<spp::Apid, spp::Error> FakeApid =
    spp::Apid::create(0x0);
constexpr spp::Expected<spp::SequenceCount, spp::Error> FakeSequenceCount =
    spp::SequenceCount::create(0x0);
constexpr spp::PacketPrimaryHeader FakeHeader = {
    .type = spp::PacketType::Telemetry,
    .secondary_header_flag = spp::SecondaryHeaderFlag::NotPresent,
    .apid = *FakeApid,
    .sequence_flags = spp::SequenceFlags::ContinuationSegment,
    .sequence_count = *FakeSequenceCount,
    .data_length = 0x0};

class SequenceTrackerFixture {
public:
  // Fixture to avoid placing local SequenceTracker to each test case
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  spp::SequenceTracker tracker;
  constexpr SequenceTrackerFixture() noexcept = default;
};
} // namespace

namespace Catch {
template <> struct StringMaker<spp::SequenceResult> {
  static std::string convert(spp::SequenceResult const &result) {
    switch (result) {
    case spp::SequenceResult::InOrder:
      return "InOrder";
    case spp::SequenceResult::Gap:
      return "Gap";
    case spp::SequenceResult::Duplicate:
      return "Duplicate";
    case spp::SequenceResult::Ignored:
      return "Ignored";
    }
    return std::format("Unknown {:#x}", static_cast<unsigned int>(result));
  }
};
} // namespace Catch

TEST_CASE_METHOD(SequenceTrackerFixture,
                 "First packet sets the baseline and reports no gap",
                 "[SequenceTracker]") {
  auto res = tracker.log(FakeHeader);

  REQUIRE(res.result == spp::SequenceResult::InOrder);
  REQUIRE(res.lost_count == 0);
}

TEST_CASE_METHOD(SequenceTrackerFixture,
                 "In order packet reports no gap (mid-range)",
                 "[SequenceTracker]") {
  auto header = FakeHeader;

  header.sequence_count = *spp::SequenceCount::create(0xFF);
  auto res = tracker.log(header);
  REQUIRE(res.result == spp::SequenceResult::InOrder);
  REQUIRE(res.lost_count == 0);

  header.sequence_count = *spp::SequenceCount::create(0x100);
  res = tracker.log(header);
  REQUIRE(res.result == spp::SequenceResult::InOrder);
  REQUIRE(res.lost_count == 0);
}

TEST_CASE_METHOD(SequenceTrackerFixture,
                 "In order packet reports no gap (wraparound)",
                 "[SequenceTracker]") {
  auto header = FakeHeader;

  header.sequence_count = *spp::SequenceCount::create(spp::SequenceCountMax);
  auto res = tracker.log(header);
  REQUIRE(res.result == spp::SequenceResult::InOrder);
  REQUIRE(res.lost_count == 0);

  header.sequence_count = *spp::SequenceCount::create(0x0);
  res = tracker.log(header);
  REQUIRE(res.result == spp::SequenceResult::InOrder);
  REQUIRE(res.lost_count == 0);
}

TEST_CASE_METHOD(SequenceTrackerFixture, "Gap of 1", "[SequenceTracker]") {
  auto header = FakeHeader;

  auto res = tracker.log(header);
  REQUIRE(res.result == spp::SequenceResult::InOrder);
  REQUIRE(res.lost_count == 0);

  header.sequence_count = *spp::SequenceCount::create(0x2);
  res = tracker.log(header);
  REQUIRE(res.result == spp::SequenceResult::Gap);
  REQUIRE(res.lost_count == 1);
}

TEST_CASE_METHOD(SequenceTrackerFixture, "Gap across the wraparound",
                 "[SequenceTracker]") {
  auto header = FakeHeader;

  header.sequence_count =
      *spp::SequenceCount::create(spp::SequenceCountMax - 1);
  auto res = tracker.log(header);
  REQUIRE(res.result == spp::SequenceResult::InOrder);
  REQUIRE(res.lost_count == 0);

  header.sequence_count = *spp::SequenceCount::create(0x1);
  res = tracker.log(header);
  REQUIRE(res.result == spp::SequenceResult::Gap);
  REQUIRE(res.lost_count == 2);
}

TEST_CASE_METHOD(SequenceTrackerFixture, "Largest possible gap",
                 "[SequenceTracker]") {
  auto header = FakeHeader;

  auto res = tracker.log(header);
  REQUIRE(res.result == spp::SequenceResult::InOrder);
  REQUIRE(res.lost_count == 0);

  header.sequence_count = *spp::SequenceCount::create(spp::SequenceCountMax);
  res = tracker.log(header);
  REQUIRE(res.result == spp::SequenceResult::Gap);
  REQUIRE(res.lost_count == spp::SequenceCountMax - 1);
}

TEST_CASE_METHOD(SequenceTrackerFixture, "Duplicate", "[SequenceTracker]") {
  auto header = FakeHeader;

  auto res = tracker.log(header);
  REQUIRE(res.result == spp::SequenceResult::InOrder);
  REQUIRE(res.lost_count == 0);

  res = tracker.log(header);
  REQUIRE(res.result == spp::SequenceResult::Duplicate);
  REQUIRE(res.lost_count == 0);
}

TEST_CASE_METHOD(SequenceTrackerFixture,
                 "Packet after a gap is in order against the new baseline",
                 "[SequenceTracker]") {
  auto header = FakeHeader;

  auto res = tracker.log(header);
  REQUIRE(res.result == spp::SequenceResult::InOrder);
  REQUIRE(res.lost_count == 0);

  header.sequence_count = *spp::SequenceCount::create(0x2);
  res = tracker.log(header);
  REQUIRE(res.result == spp::SequenceResult::Gap);
  REQUIRE(res.lost_count == 1);

  header.sequence_count = *spp::SequenceCount::create(0x3);
  res = tracker.log(header);
  REQUIRE(res.result == spp::SequenceResult::InOrder);
  REQUIRE(res.lost_count == 0);
}

TEST_CASE_METHOD(SequenceTrackerFixture, "Idle packet is ignored",
                 "[SequenceTracker]") {
  auto header = FakeHeader;

  header.apid = *spp::Apid::create(spp::ApidIdlePacket);
  auto res = tracker.log(header);
  REQUIRE(res.result == spp::SequenceResult::Ignored);
  REQUIRE(res.lost_count == 0);

  header.sequence_count = *spp::SequenceCount::create(0x1);
  res = tracker.log(header);
  REQUIRE(res.result == spp::SequenceResult::Ignored);
  REQUIRE(res.lost_count == 0);
}

TEST_CASE_METHOD(SequenceTrackerFixture, "reset clears the baseline",
                 "[SequenceTracker]") {
  auto header1 = FakeHeader;
  auto header2 = FakeHeader;

  auto res = tracker.log(header1);
  REQUIRE(res.result == spp::SequenceResult::InOrder);
  REQUIRE(res.lost_count == 0);

  header2.apid = *spp::Apid::create(0xF);
  header2.sequence_count = *spp::SequenceCount::create(0xFF);
  res = tracker.log(header2);
  REQUIRE(res.result == spp::SequenceResult::InOrder);
  REQUIRE(res.lost_count == 0);

  tracker.reset(header1.apid);

  header1.sequence_count = *spp::SequenceCount::create(2);
  res = tracker.log(header1);
  REQUIRE(res.result == spp::SequenceResult::InOrder);
  REQUIRE(res.lost_count == 0);

  header2.sequence_count = *spp::SequenceCount::create(0x101);
  res = tracker.log(header2);
  REQUIRE(res.result == spp::SequenceResult::Gap);
  REQUIRE(res.lost_count == 1);
}

TEST_CASE_METHOD(SequenceTrackerFixture,
                 "reset_all clears the baseline for all", "[SequenceTracker]") {
  auto header1 = FakeHeader;
  auto header2 = FakeHeader;

  auto res = tracker.log(header1);
  REQUIRE(res.result == spp::SequenceResult::InOrder);
  REQUIRE(res.lost_count == 0);

  header2.apid = *spp::Apid::create(0xF);
  header2.sequence_count = *spp::SequenceCount::create(0xFF);
  res = tracker.log(header2);
  REQUIRE(res.result == spp::SequenceResult::InOrder);
  REQUIRE(res.lost_count == 0);

  tracker.reset_all();

  header1.sequence_count = *spp::SequenceCount::create(2);
  res = tracker.log(header1);
  REQUIRE(res.result == spp::SequenceResult::InOrder);
  REQUIRE(res.lost_count == 0);

  header2.sequence_count = *spp::SequenceCount::create(0x101);
  res = tracker.log(header2);
  REQUIRE(res.result == spp::SequenceResult::InOrder);
  REQUIRE(res.lost_count == 0);
}

TEST_CASE_METHOD(SequenceTrackerFixture, "APIDs are independent",
                 "[SequenceTracker]") {
  auto header1 = FakeHeader;
  auto header2 = FakeHeader;

  auto res = tracker.log(header1);
  REQUIRE(res.result == spp::SequenceResult::InOrder);
  REQUIRE(res.lost_count == 0);

  header2.apid = *spp::Apid::create(spp::ApidMax - 1);
  header2.sequence_count = *spp::SequenceCount::create(0xFF);
  res = tracker.log(header2);
  REQUIRE(res.result == spp::SequenceResult::InOrder);
  REQUIRE(res.lost_count == 0);

  header1.sequence_count = *spp::SequenceCount::create(2);
  res = tracker.log(header1);
  REQUIRE(res.result == spp::SequenceResult::Gap);
  REQUIRE(res.lost_count == 1);

  header2.sequence_count = *spp::SequenceCount::create(0x100);
  res = tracker.log(header2);
  REQUIRE(res.result == spp::SequenceResult::InOrder);
  REQUIRE(res.lost_count == 0);
}

TEST_CASE("SequenceTracker constructor can't throw", "[SequenceTracker]") {
  STATIC_REQUIRE(std::is_nothrow_constructible_v<spp::SequenceTracker>);
}

TEST_CASE("SequenceTracker log can't throw", "[SequenceTracker]") {
  STATIC_REQUIRE(
      noexcept(std::declval<spp::SequenceTracker &>().log(FakeHeader)));
}

TEST_CASE("SequenceTracker reset can't throw", "[SequenceTracker]") {
  STATIC_REQUIRE(
      noexcept(std::declval<spp::SequenceTracker &>().reset(*FakeApid)));
}

TEST_CASE("SequenceTracker reset_all can't throw", "[SequenceTracker]") {
  STATIC_REQUIRE(noexcept(std::declval<spp::SequenceTracker &>().reset_all()));
}

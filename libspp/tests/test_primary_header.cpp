#include <spp/error.hpp>
#include <spp/primary_header.hpp>

#include <cstdint>
#include <limits>
#include <utility>

#include <catch2/catch_test_macros.hpp>

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

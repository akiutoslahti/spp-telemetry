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

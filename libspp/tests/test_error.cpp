#include <spp/error.hpp>

#include <limits>
#include <string_view>
#include <type_traits>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Error to_string returns a correct error", "[Error]") {
  const spp::Error error = spp::Error::Internal;
  REQUIRE(to_string(error) == "Internal error");
}

TEST_CASE("Error to_string returns a correct error at compile time",
          "[Error]") {
  constexpr spp::Error error = spp::Error::Internal;
  STATIC_REQUIRE(to_string(error) == "Internal error");
}

TEST_CASE("Error to_string returns a fallback on unknown error", "[Error]") {
  // needed for testing the fallback
  // well-defined because Error has a fixed underlying type
  // NOLINTBEGIN(clang-analyzer-optin.core.EnumCastOutOfRange)
  const auto error = static_cast<spp::Error>(
      std::numeric_limits<std::underlying_type_t<spp::Error>>::max());
  // NOLINTEND(clang-analyzer-optin.core.EnumCastOutOfRange)
  REQUIRE(to_string(error) == "Unknown error");
}

TEST_CASE("Error to_string can't throw", "[Error]") {
  STATIC_REQUIRE(noexcept(to_string(spp::Error::Internal)));
}

TEST_CASE("Error to_string doesn't allocate", "[Error]") {
  STATIC_REQUIRE(std::is_same_v<std::string_view,
                                decltype(to_string(spp::Error::Internal))>);
}

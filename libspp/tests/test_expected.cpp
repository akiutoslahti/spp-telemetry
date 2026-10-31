#include <spp/expected.hpp>

#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>

#include <catch2/catch_test_macros.hpp>

namespace {
// test helper; only the move must be user-provided
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
struct MayThrowOnMove {
  MayThrowOnMove(MayThrowOnMove && /*unused*/) noexcept(false) {}
};

enum class FakeError : std::uint8_t { Generic };
} // namespace

//
// Tests for Unexpected
//

TEST_CASE("Unexpected holds error and returns it", "[Unexpected]") {
  const spp::Unexpected<int> unexp = spp::Unexpected(42);
  REQUIRE(unexp.error() == 42);
}

TEST_CASE("Unexpected works at compile time", "[Unexpected]") {
  constexpr spp::Unexpected<int> unexp = spp::Unexpected(42);
  static_assert(unexp.error() == 42);
}

TEST_CASE("Unexpected can deduce its type", "[Unexpected]") {
  auto unexp = spp::Unexpected(42);
  static_assert(std::is_same_v<decltype(unexp), spp::Unexpected<int>>);
}

TEST_CASE("Unexpected can't be created implicitly", "[Unexpected]") {
  static_assert(!std::is_convertible_v<int, spp::Unexpected<int>>);
}

TEST_CASE("Unexpected has no default constructor", "[Unexpected]") {
  static_assert(!std::is_default_constructible_v<spp::Unexpected<int>>);
}

TEST_CASE("Unexpected error accessor (const&) can't throw", "[Unexpected]") {
  static_assert(noexcept(std::declval<const spp::Unexpected<int> &>().error()));
}

TEST_CASE("Unexpected error accessor (&&) can't throw", "[Unexpected]") {
  static_assert(noexcept(std::declval<spp::Unexpected<int>>().error()));
}

TEST_CASE("Unexpected constructor is noexcept exactly when E's move is",
          "[Unexpected]") {
  static_assert(std::is_nothrow_constructible_v<spp::Unexpected<int>, int>);
  static_assert(
      !std::is_nothrow_constructible_v<spp::Unexpected<MayThrowOnMove>,
                                       MayThrowOnMove>);
}

TEST_CASE("Unexpected constructor moves its error argument", "[Unexpected]") {
  std::unique_ptr<int> ptr = std::make_unique<int>(42);
  auto unexp = spp::Unexpected(std::move(ptr));
  REQUIRE(unexp.error() != nullptr);
  REQUIRE(ptr == nullptr);
  REQUIRE(*unexp.error() == 42);
}

TEST_CASE("Unexpected error can be moved out", "[Unexpected]") {
  std::unique_ptr<int> ptr = std::make_unique<int>(42);
  auto unexp = spp::Unexpected(std::move(ptr));
  auto ptr2 = std::move(unexp).error();
  REQUIRE(*ptr2 == 42);
}

TEST_CASE("Unexpected error moves only from an rvalue", "[Unexpected]") {
  static_assert(
      std::is_same_v<const int &,
                     decltype(std::declval<spp::Unexpected<int> &>().error())>);
  static_assert(
      std::is_same_v<int &&,
                     decltype(std::declval<spp::Unexpected<int>>().error())>);
}

//
// Tests for Expected
//

TEST_CASE("Expected has value", "[Expected]") {
  const spp::Expected<int, FakeError> exp = 42;
  REQUIRE(exp.has_value());
  REQUIRE(exp.value() == 42);
}

TEST_CASE("Expected has value at compile time", "[Expected]") {
  constexpr spp::Expected<int, FakeError> exp = 42;
  static_assert(exp.has_value());
  static_assert(exp.value() == 42);
}

TEST_CASE("Expected has error", "[Expected]") {
  const spp::Expected<int, FakeError> exp = spp::Unexpected(FakeError::Generic);
  REQUIRE(!exp.has_value());
  REQUIRE(exp.error() == FakeError::Generic);
}

TEST_CASE("Expected has error at compile time", "[Expected]") {
  constexpr spp::Expected<int, FakeError> exp =
      spp::Unexpected(FakeError::Generic);
  static_assert(!exp.has_value());
  static_assert(exp.error() == FakeError::Generic);
}

TEST_CASE("Expected can be created implicitly from value", "[Expected]") {
  static_assert(std::is_convertible_v<int, spp::Expected<int, FakeError>>);
}

TEST_CASE("Expected can be created implicitly from error", "[Expected]") {
  static_assert(std::is_convertible_v<spp::Unexpected<FakeError>,
                                      spp::Expected<int, FakeError>>);
}

TEST_CASE("Expected can't be created implicitly from error withouth Unexpected "
          "wrapper",
          "[Expected]") {
  static_assert(
      !std::is_convertible_v<FakeError, spp::Expected<int, FakeError>>);
}

TEST_CASE("Expected has no default constructor", "[Expected]") {
  static_assert(
      !std::is_default_constructible_v<spp::Expected<int, FakeError>>);
}

TEST_CASE("Expected constructor moves its value argument", "[Expected]") {
  std::unique_ptr<int> ptr = std::make_unique<int>(42);
  const spp::Expected<std::unique_ptr<int>, FakeError> exp = std::move(ptr);
  REQUIRE(ptr == nullptr);
  REQUIRE(exp.has_value());
  REQUIRE(*exp.value() == 42);
}

TEST_CASE("Expected query has_value can't throw", "[Expected]") {
  static_assert(noexcept(
      std::declval<const spp::Expected<int, FakeError>>().has_value()));
}

TEST_CASE("Expected accessor value can't throw", "[Expected]") {
  static_assert(
      noexcept(std::declval<const spp::Expected<int, FakeError>>().value()));
}

TEST_CASE("Expected accessor error can't throw", "[Expected]") {
  static_assert(
      noexcept(std::declval<const spp::Expected<int, FakeError>>().error()));
}

TEST_CASE(
    "Expected constructor from value is noexcept exactly when T's move is",
    "[Expected]") {
  static_assert(
      std::is_nothrow_constructible_v<spp::Expected<int, FakeError>, int>);
  static_assert(
      !std::is_nothrow_constructible_v<spp::Expected<MayThrowOnMove, FakeError>,
                                       MayThrowOnMove>);
}

TEST_CASE("Expected can be constructed implicitly from value and error when "
          "they share type",
          "[Expected]") {
  const spp::Expected<int, int> exp = 42;
  REQUIRE(exp.has_value());
  REQUIRE(exp.value() == 42);
  const spp::Expected<int, int> exp2 = spp::Unexpected(42);
  REQUIRE(!exp2.has_value());
  REQUIRE(exp2.error() == 42);
}

TEST_CASE("Expected constructor moves its error argument", "[Expected]") {
  std::unique_ptr<int> ptr = std::make_unique<int>(42);
  auto unexp = spp::Unexpected(std::move(ptr));
  const spp::Expected<int, std::unique_ptr<int>> exp = std::move(unexp);
  REQUIRE(!exp.has_value());
  REQUIRE(*exp.error() == 42);
}

TEST_CASE(
    "Expected constructor from error is noexcept exactly when E's move is",
    "[Expected]") {
  static_assert(std::is_nothrow_constructible_v<spp::Expected<int, int>,
                                                spp::Unexpected<int>>);
  static_assert(
      !std::is_nothrow_constructible_v<spp::Expected<int, MayThrowOnMove>,
                                       spp::Unexpected<MayThrowOnMove>>);
}

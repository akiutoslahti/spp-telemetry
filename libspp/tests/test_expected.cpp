#include <spp/expected.hpp>

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
} // namespace

TEST_CASE("Unexpected holds and returns its value", "[Unexpected]") {
  const spp::Unexpected<int> unexp = spp::Unexpected(42);
  REQUIRE(unexp.error() == 42);
}

TEST_CASE("Unexpected works at compile time", "[Unexpected]") {
  constexpr spp::Unexpected<int> unexp = spp::Unexpected(42);
  static_assert(unexp.error() == 42);
}

TEST_CASE("Unexpected deduces its type", "[Unexpected]") {
  auto unexp = spp::Unexpected(42);
  static_assert(std::is_same_v<decltype(unexp), spp::Unexpected<int>>);
}

TEST_CASE("Unexpected can't be created implicitly", "[Unexpected]") {
  static_assert(!std::is_convertible_v<int, spp::Unexpected<int>>);
}

TEST_CASE("Unexpected has no default constructor", "[Unexpected]") {
  static_assert(!std::is_default_constructible_v<spp::Unexpected<int>>);
}

TEST_CASE("Unexpected accessor can't throw", "[Unexpected]") {
  static_assert(noexcept(std::declval<const spp::Unexpected<int>>().error()));
}

TEST_CASE("Unexpected constructor is noexcept exactly when E's move is",
          "[Unexpected]") {
  static_assert(std::is_nothrow_constructible_v<spp::Unexpected<int>, int>);
  static_assert(
      !std::is_nothrow_constructible_v<spp::Unexpected<MayThrowOnMove>,
                                       MayThrowOnMove>);
}

TEST_CASE("Unexpected constructor moves its argument", "[Unexpected]") {
  std::unique_ptr<int> ptr = std::make_unique<int>(42);
  auto unexp = spp::Unexpected(std::move(ptr));
  REQUIRE(unexp.error() != nullptr);
  REQUIRE(ptr == nullptr);
  REQUIRE(*unexp.error() == 42);
}

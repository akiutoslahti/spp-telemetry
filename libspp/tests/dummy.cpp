#include <catch2/catch_test_macros.hpp>

namespace {
int question_of_life() { return 42; }
} // namespace

TEST_CASE("Answer to the question of life is 42", "[question_of_life]") {
  REQUIRE(question_of_life() == 42);
}

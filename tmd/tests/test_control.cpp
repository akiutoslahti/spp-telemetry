#include <spp/expected.hpp>
#include <spp/packet.hpp>
#include <spp/primary_header.hpp>
#include <tmd/control.hpp>
#include <tmd/statistics.hpp>

#include "test_common.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {
class ExecuteCommandFixture {
public:
  // Fixture to avoid placing local Statistics to each test case
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  tmd::Statistics stats;
  constexpr ExecuteCommandFixture() noexcept = default;
};
} // namespace

TEST_CASE_METHOD(ExecuteCommandFixture,
                 "Stats on empty statistics; empty report + newline",
                 "[ExecuteCommand]") {
  REQUIRE(tmd::execute_command("stats", stats) ==
          "Valid packets: None\nErrors: None\n");
}

TEST_CASE_METHOD(
    ExecuteCommandFixture,
    "stats after recording a packet: the report contains that APID's line",
    "[ExecuteCommand]") {
  stats.record(tmdtest::FakePacket);

  REQUIRE(tmd::execute_command("stats", stats) ==
          "Valid packets:\n  APID=0x0, packets=1, bytes=7, gaps=0, lost=0, "
          "duplicates=0\nErrors: None\n");
}

TEST_CASE_METHOD(
    ExecuteCommandFixture,
    "reset replies with the confirmation, and afterwards get() for that "
    "APID is empty",
    "[ExecuteCommand]") {
  stats.record(tmdtest::FakePacket);

  REQUIRE(tmd::execute_command("stats", stats) ==
          "Valid packets:\n  APID=0x0, packets=1, bytes=7, gaps=0, lost=0, "
          "duplicates=0\nErrors: None\n");

  REQUIRE(tmd::execute_command("reset", stats) == "ok\n");

  REQUIRE(!stats.get(*tmdtest::FakeApid).has_value());
}

TEST_CASE_METHOD(ExecuteCommandFixture,
                 "stats after reset gives the empty report",
                 "[ExecuteCommand]") {
  stats.record(tmdtest::FakePacket);

  REQUIRE(tmd::execute_command("stats", stats) ==
          "Valid packets:\n  APID=0x0, packets=1, bytes=7, gaps=0, lost=0, "
          "duplicates=0\nErrors: None\n");

  REQUIRE(tmd::execute_command("reset", stats) == "ok\n");

  REQUIRE(tmd::execute_command("stats", stats) ==
          "Valid packets: None\nErrors: None\n");
}

TEST_CASE_METHOD(
    ExecuteCommandFixture,
    "An unknown command gives the error line, and the statistics are unchanged",
    "[ExecuteCommand]") {
  stats.record(tmdtest::FakePacket);

  REQUIRE(tmd::execute_command("stats", stats) ==
          "Valid packets:\n  APID=0x0, packets=1, bytes=7, gaps=0, lost=0, "
          "duplicates=0\nErrors: None\n");

  REQUIRE(tmd::execute_command("asd", stats) == "Unknown command: \"asd\"\n");

  REQUIRE(tmd::execute_command("stats", stats) ==
          "Valid packets:\n  APID=0x0, packets=1, bytes=7, gaps=0, lost=0, "
          "duplicates=0\nErrors: None\n");
}

TEST_CASE_METHOD(ExecuteCommandFixture, "An empty command gives error line",
                 "[ExecuteCommand]") {
  REQUIRE(tmd::execute_command("", stats) == "Empty command\n");
}

TEST_CASE_METHOD(ExecuteCommandFixture,
                 "A whitespace only command gives error line",
                 "[ExecuteCommand]") {
  REQUIRE(tmd::execute_command("    ", stats) == "Empty command\n");
}

TEST_CASE_METHOD(ExecuteCommandFixture, "'stats\\r' behaves like stats",
                 "[ExecuteCommand]") {
  REQUIRE(tmd::execute_command("stats\r", stats) ==
          "Valid packets: None\nErrors: None\n");
}

TEST_CASE_METHOD(ExecuteCommandFixture, "' stats ' behaves like stats",
                 "[ExecuteCommand]") {
  REQUIRE(tmd::execute_command(" stats ", stats) ==
          "Valid packets: None\nErrors: None\n");
}

TEST_CASE_METHOD(ExecuteCommandFixture,
                 "View into 'stats\\nreset' behaves like stats",
                 "[ExecuteCommand]") {
  std::string_view cmd = "stats\nreset";
  // NOLINTNEXTLINE(readability-magic-numbers)
  cmd.remove_suffix(6);
  REQUIRE(tmd::execute_command(cmd, stats) ==
          "Valid packets: None\nErrors: None\n");
}

TEST_CASE_METHOD(ExecuteCommandFixture, "'stats now' is Unknown",
                 "[ExecuteCommand]") {
  REQUIRE(tmd::execute_command("stats now", stats) ==
          "Unknown command: \"stats now\"\n");
}

TEST_CASE_METHOD(ExecuteCommandFixture, "'STATS' is Unknown",
                 "[ExecuteCommand]") {
  REQUIRE(tmd::execute_command("STATS", stats) ==
          "Unknown command: \"STATS\"\n");
}

TEST_CASE("Empty buffer without EOF has no command yet", "[FindCommand]") {
  REQUIRE_FALSE(tmd::find_command("", false).has_value());
}

TEST_CASE("Empty buffer at EOF has no command", "[FindCommand]") {
  REQUIRE_FALSE(tmd::find_command("", true).has_value());
}

TEST_CASE("Partial command without newline waits for more data",
          "[FindCommand]") {
  REQUIRE_FALSE(tmd::find_command("stats", false).has_value());
}

TEST_CASE("Partial command at EOF is the whole buffer", "[FindCommand]") {
  REQUIRE(tmd::find_command("stats", true) == "stats");
}

TEST_CASE("Command ending in newline is returned without the newline",
          "[FindCommand]") {
  REQUIRE(tmd::find_command("stats\n", false) == "stats");
}

TEST_CASE("Newline decides the command even at EOF", "[FindCommand]") {
  REQUIRE(tmd::find_command("stats\n", true) == "stats");
}

TEST_CASE("Only the first line of several is returned", "[FindCommand]") {
  REQUIRE(tmd::find_command("stats\nreset\n", false) == "stats");
}

TEST_CASE("Lone newline is an empty command, not no command", "[FindCommand]") {
  REQUIRE(tmd::find_command("\n", false).has_value());
  REQUIRE(tmd::find_command("\n", false) == "");
}

TEST_CASE("Carriage return is left for execute_command to trim",
          "[FindCommand]") {
  REQUIRE(tmd::find_command("stats\r\n", false) == "stats\r");
}

TEST_CASE("Newline just past the end of the view is not seen",
          "[FindCommand]") {
  const std::string_view sv1 = "stats\nX";
  REQUIRE_FALSE(tmd::find_command(sv1.substr(0, 5), false).has_value());
}

TEST_CASE("Returned command points into the input buffer", "[FindCommand]") {
  const std::string_view sv1 = "stats\n";
  auto cmd = tmd::find_command(sv1, false);
  REQUIRE(cmd.has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access) - guarded above
  REQUIRE(cmd.value().data() == sv1.data());
}

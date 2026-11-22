#include <span>
#include <string>
#include <string_view>
#include <syslog.h>
#include <utility>
#include <variant>
#include <vector>

#include <spp/expected.hpp>
#include <tmd/options.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

namespace {
const tmd::Config DefaultConfig = {
    .udp_port = tmd::UdpPortDefault,
    .control_socket = std::string(tmd::ControlSocketDefault),
    .log_level = tmd::LogLevelDefault,
    .pid_file = std::string(tmd::PidFileDefault)};

tmd::Config
get_config(spp::Expected<std::variant<tmd::Config, tmd::HelpRequested>,
                         tmd::ParseError> const &parse_res) {
  REQUIRE(parse_res.has_value());
  REQUIRE(std::holds_alternative<tmd::Config>(parse_res.value()));
  return *std::get_if<tmd::Config>(&parse_res.value());
}

tmd::ParseError
get_error(spp::Expected<std::variant<tmd::Config, tmd::HelpRequested>,
                        tmd::ParseError> const &parse_res) {
  REQUIRE(!parse_res.has_value());
  return parse_res.error();
}
} // namespace

TEST_CASE("Parser rejects unknown option", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--foo"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto error = get_error(res);
  REQUIRE(error.kind == tmd::ParseErrorKind::UnknownOption);
  REQUIRE(error.token == "--foo");
}

TEST_CASE("Parser rejects positional argument", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--port", "5656", "foo"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto error = get_error(res);
  REQUIRE(error.kind == tmd::ParseErrorKind::UnknownOption);
  REQUIRE(error.token == "foo");
}

TEST_CASE("First error wins", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--foo", "--port", "X"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto error = get_error(res);
  REQUIRE(error.kind == tmd::ParseErrorKind::UnknownOption);
  REQUIRE(error.token == "--foo");
}

TEST_CASE("Last option wins", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--port", "1", "--port", "2"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto config = get_config(res);
  auto expected = DefaultConfig;
  expected.udp_port = 2;
  REQUIRE(config == expected);
}

TEST_CASE("Parser treats subsequent option as missing value",
          "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--socket", "--pid-file"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto error = get_error(res);
  REQUIRE(error.kind == tmd::ParseErrorKind::MissingValue);
  REQUIRE(error.token == "--socket");
}

TEST_CASE("'--help' alone", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--help"};
  auto res = tmd::parse_cli_args(std::span(args));
  REQUIRE(res.has_value());
  REQUIRE(std::holds_alternative<tmd::HelpRequested>(res.value()));
}

TEST_CASE("'--help' wins always", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--port", "5656X", "--help"};
  auto res = tmd::parse_cli_args(std::span(args));
  REQUIRE(res.has_value());
  REQUIRE(std::holds_alternative<tmd::HelpRequested>(res.value()));
}

TEST_CASE("Empty args -> defaults", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {};
  auto res = tmd::parse_cli_args(std::span(args));
  auto config = get_config(res);
  REQUIRE(config == DefaultConfig);
}

TEST_CASE("UDP port rejects missing value", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--port"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto error = get_error(res);
  REQUIRE(error.kind == tmd::ParseErrorKind::MissingValue);
  REQUIRE(error.token == "--port");
}

TEST_CASE("Control socket rejects missing value", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--socket"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto error = get_error(res);
  REQUIRE(error.kind == tmd::ParseErrorKind::MissingValue);
  REQUIRE(error.token == "--socket");
}

TEST_CASE("Log level rejects missing value", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--log-level"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto error = get_error(res);
  REQUIRE(error.kind == tmd::ParseErrorKind::MissingValue);
  REQUIRE(error.token == "--log-level");
}

TEST_CASE("Pid file rejects missing value", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--pid-file"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto error = get_error(res);
  REQUIRE(error.kind == tmd::ParseErrorKind::MissingValue);
  REQUIRE(error.token == "--pid-file");
}

TEST_CASE("UDP port accepts 1", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--port", "1"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto config = get_config(res);
  auto expected = DefaultConfig;
  expected.udp_port = 1;
  REQUIRE(config == expected);
}

TEST_CASE("UDP port accepts 65535", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--port", "65535"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto config = get_config(res);
  auto expected = DefaultConfig;
  expected.udp_port = 65535;
  REQUIRE(config == expected);
}

TEST_CASE("UDP port rejects 0", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--port", "0"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto error = get_error(res);
  REQUIRE(error.kind == tmd::ParseErrorKind::InvalidPort);
  REQUIRE(error.token == "0");
}

TEST_CASE("UDP port rejects 65536", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--port", "65536"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto error = get_error(res);
  REQUIRE(error.kind == tmd::ParseErrorKind::InvalidPort);
  REQUIRE(error.token == "65536");
}

TEST_CASE("UDP port rejects empty string", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--port", ""};
  auto res = tmd::parse_cli_args(std::span(args));
  auto error = get_error(res);
  REQUIRE(error.kind == tmd::ParseErrorKind::EmptyValue);
  REQUIRE(error.token == "--port");
}

TEST_CASE("UDP port rejects non-numeric value", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--port", "foo"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto error = get_error(res);
  REQUIRE(error.kind == tmd::ParseErrorKind::InvalidPort);
  REQUIRE(error.token == "foo");
}

TEST_CASE("UDP port rejects valid value with trailing non-numeric characters",
          "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--port", "5656X"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto error = get_error(res);
  REQUIRE(error.kind == tmd::ParseErrorKind::InvalidPort);
  REQUIRE(error.token == "5656X");
}

TEST_CASE("UDP port rejects negative value", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--port", "-1"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto error = get_error(res);
  REQUIRE(error.kind == tmd::ParseErrorKind::InvalidPort);
  REQUIRE(error.token == "-1");
}

TEST_CASE("UDP port rejects +5", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--port", "+5"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto error = get_error(res);
  REQUIRE(error.kind == tmd::ParseErrorKind::InvalidPort);
  REQUIRE(error.token == "+5");
}

TEST_CASE("UDP port rejects number with leading space", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--port", " 5"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto error = get_error(res);
  REQUIRE(error.kind == tmd::ParseErrorKind::InvalidPort);
  REQUIRE(error.token == " 5");
}

TEST_CASE("Control socket accepts path", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--socket", "/tmp/tmd.sock"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto config = get_config(res);
  auto expected = DefaultConfig;
  expected.control_socket = "/tmp/tmd.sock";
  REQUIRE(config == expected);
}

TEST_CASE("Control socket rejects empty path", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--socket", ""};
  auto res = tmd::parse_cli_args(std::span(args));
  auto error = get_error(res);
  REQUIRE(error.kind == tmd::ParseErrorKind::EmptyValue);
  REQUIRE(error.token == "--socket");
}

TEST_CASE("Pid file accepts path", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--pid-file", "/tmp/tmd.pid"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto config = get_config(res);
  auto expected = DefaultConfig;
  expected.pid_file = "/tmp/tmd.pid";
  REQUIRE(config == expected);
}

TEST_CASE("Pid file rejects empty path", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--pid-file", ""};
  auto res = tmd::parse_cli_args(std::span(args));
  auto error = get_error(res);
  REQUIRE(error.kind == tmd::ParseErrorKind::EmptyValue);
  REQUIRE(error.token == "--pid-file");
}

TEST_CASE("Log level accepts correct values", "[ParseCliArgs]") {
  auto [value, level] = GENERATE(
      std::make_pair("emerg", LOG_EMERG), std::make_pair("alert", LOG_ALERT),
      std::make_pair("crit", LOG_CRIT), std::make_pair("err", LOG_ERR),
      std::make_pair("warning", LOG_WARNING),
      std::make_pair("notice", LOG_NOTICE), std::make_pair("info", LOG_INFO),
      std::make_pair("debug", LOG_DEBUG));

  std::vector<std::string_view> args = {"--log-level", value};
  auto res = tmd::parse_cli_args(std::span(args));
  auto config = get_config(res);
  auto expected = DefaultConfig;
  expected.log_level = level;
  REQUIRE(config == expected);
}

TEST_CASE("Log level rejects empty", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--log-level", ""};
  auto res = tmd::parse_cli_args(std::span(args));
  auto error = get_error(res);
  REQUIRE(error.kind == tmd::ParseErrorKind::EmptyValue);
  REQUIRE(error.token == "--log-level");
}

TEST_CASE("Log level rejects unknown level", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--log-level", "foo"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto error = get_error(res);
  REQUIRE(error.kind == tmd::ParseErrorKind::InvalidLogLevel);
  REQUIRE(error.token == "foo");
}

TEST_CASE("Log level is case sensitive", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--log-level", "INFO"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto error = get_error(res);
  REQUIRE(error.kind == tmd::ParseErrorKind::InvalidLogLevel);
  REQUIRE(error.token == "INFO");
}

TEST_CASE("Parse all successfully #1", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {
      "--log-level", "crit",          "--pid-file", "/tmp/tmd.pid",
      "--socket",    "/tmp/tmd.sock", "--port",     "4123"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto config = get_config(res);
  tmd::Config expected = tmd::Config{.udp_port = 4123,
                                     .control_socket = "/tmp/tmd.sock",
                                     .log_level = LOG_CRIT,
                                     .pid_file = "/tmp/tmd.pid"};
  REQUIRE(config == expected);
}

TEST_CASE("Parse all successfully #2", "[ParseCliArgs]") {
  std::vector<std::string_view> args = {"--port",      "5657",
                                        "--socket",    "/tmp/tmd/tmd.sock",
                                        "--pid-file",  "/tmp/tmd/tmd.pid",
                                        "--log-level", "err"};
  auto res = tmd::parse_cli_args(std::span(args));
  auto config = get_config(res);
  tmd::Config expected = tmd::Config{.udp_port = 5657,
                                     .control_socket = "/tmp/tmd/tmd.sock",
                                     .log_level = LOG_ERR,
                                     .pid_file = "/tmp/tmd/tmd.pid"};
  REQUIRE(config == expected);
}

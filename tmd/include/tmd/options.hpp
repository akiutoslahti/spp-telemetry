#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <syslog.h>
#include <variant>

#include <spp/expected.hpp>

namespace tmd {
constexpr std::uint16_t UdpPortDefault = 5555;
constexpr std::string_view ControlSocketDefault = "/run/tmd.sock";
constexpr int LogLevelDefault = LOG_INFO;
constexpr std::string_view PidFileDefault = "/run/tmd.pid";

struct Config {
  std::uint16_t udp_port;
  std::string control_socket;
  int log_level;
  std::string pid_file;

  [[nodiscard]] friend bool operator==(const Config &,
                                       const Config &) noexcept = default;
};

struct HelpRequested {};

enum class ParseErrorKind : std::uint8_t {
  UnknownOption,
  MissingValue,
  InvalidPort,
  EmptyValue,
  InvalidLogLevel,
};

[[nodiscard]] constexpr std::string_view
to_string(ParseErrorKind kind) noexcept {
  switch (kind) {
  case ParseErrorKind::UnknownOption:
    return "Unknown option";
  case ParseErrorKind::MissingValue:
    return "Missing value";
  case ParseErrorKind::InvalidPort:
    return "Invalid UDP Port";
  case ParseErrorKind::EmptyValue:
    return "Empty value";
  case ParseErrorKind::InvalidLogLevel:
    return "Invalid log level";
  }

  return "Unkown kind";
}

struct ParseError {
  ParseErrorKind kind;
  std::string_view token;
};

spp::Expected<std::variant<Config, HelpRequested>, ParseError>
parse_cli_args(std::span<const std::string_view> args);
} // namespace tmd

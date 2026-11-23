#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <syslog.h>
#include <system_error>
#include <utility>
#include <variant>

#include <spp/expected.hpp>
#include <tmd/options.hpp>

namespace {
enum class ConfigOptions : std::uint8_t {
  UdpPort,
  ControlSocket,
  LogLevel,
  PidFile,
};

constexpr std::array<std::pair<std::string_view, ConfigOptions>, 4>
    config_options = {std::make_pair("--port", ConfigOptions::UdpPort),
                      std::make_pair("--socket", ConfigOptions::ControlSocket),
                      std::make_pair("--log-level", ConfigOptions::LogLevel),
                      std::make_pair("--pid-file", ConfigOptions::PidFile)};

constexpr std::array<std::pair<std::string_view, int>, 8> log_levels = {
    std::make_pair("emerg", LOG_EMERG),
    std::make_pair("alert", LOG_ALERT),
    std::make_pair("crit", LOG_CRIT),
    std::make_pair("err", LOG_ERR),
    std::make_pair("warning", LOG_WARNING),
    std::make_pair("notice", LOG_NOTICE),
    std::make_pair("info", LOG_INFO),
    std::make_pair("debug", LOG_DEBUG)};

spp::Expected<std::uint16_t, tmd::ParseError>
parse_port(const std::string_view token) {
  std::uint16_t u16_val{};
  auto [ptr, ec] =
      std::from_chars(token.data(), token.data() + token.size(), u16_val);

  if (ptr != token.data() + token.size() || ec != std::errc() || u16_val == 0) {
    return spp::Unexpected(tmd::ParseError{
        .kind = tmd::ParseErrorKind::InvalidPort, .token = token});
  }

  return u16_val;
}

spp::Expected<int, tmd::ParseError>
parse_log_level(const std::string_view token) {
  const auto *const it = std::ranges::find(
      log_levels, token, &decltype(log_levels)::value_type::first);
  if (it == log_levels.end()) {
    return spp::Unexpected(tmd::ParseError{
        .kind = tmd::ParseErrorKind::InvalidLogLevel, .token = token});
  }

  return it->second;
}

spp::Expected<std::string_view, tmd::ParseError>
get_value(std::string_view option, std::span<const std::string_view> args) {
  if (args.empty() || args[0].starts_with("--")) {
    return spp::Unexpected(tmd::ParseError{
        .kind = tmd::ParseErrorKind::MissingValue, .token = option});
  }

  return args[0];
}
} // namespace

namespace tmd {
spp::Expected<std::variant<Config, HelpRequested>, ParseError>
parse_cli_args(std::span<const std::string_view> args) {
  auto help_it = std::ranges::find(args, "--help");
  if (help_it != args.end()) {
    return std::variant<Config, HelpRequested>(HelpRequested{});
  }

  Config config = {.udp_port = UdpPortDefault,
                   .control_socket = std::string(ControlSocketDefault),
                   .log_level = LogLevelDefault,
                   .pid_file = std::string(PidFileDefault)};

  for (std::size_t idx = 0; idx < args.size(); idx++) {
    auto option = args[idx];

    const auto *opt_it = std::ranges::find(
        config_options, option, &decltype(config_options)::value_type::first);
    if (opt_it == config_options.end()) {
      return spp::Unexpected(
          ParseError{.kind = ParseErrorKind::UnknownOption, .token = option});
    }

    auto maybe_value = get_value(option, args.subspan(idx + 1));
    if (!maybe_value.has_value()) {
      return spp::Unexpected(maybe_value.error());
    }
    idx++;
    auto value = maybe_value.value();

    if (value.empty()) {
      return spp::Unexpected(
          ParseError{.kind = ParseErrorKind::EmptyValue, .token = option});
    }

    switch (opt_it->second) {
    case ConfigOptions::UdpPort: {
      auto maybe_port = parse_port(value);
      if (!maybe_port.has_value()) {
        return spp::Unexpected(maybe_port.error());
      }
      config.udp_port = maybe_port.value();
    } break;
    case ConfigOptions::ControlSocket:
      config.control_socket = value;
      break;
    case ConfigOptions::LogLevel: {
      auto maybe_log_lvl = parse_log_level(value);
      if (!maybe_log_lvl.has_value()) {
        return spp::Unexpected(maybe_log_lvl.error());
      }
      config.log_level = maybe_log_lvl.value();
    } break;
    case ConfigOptions::PidFile:
      config.pid_file = value;
      break;
    }
  }

  return std::variant<Config, HelpRequested>(config);
}

std::string_view loglvl_to_string(int log_level) {
  const auto *it = std::ranges::find(log_levels, log_level,
                                     &decltype(log_levels)::value_type::second);
  if (it != log_levels.end()) {
    return it->first;
  }
  return "Unknown log level";
}
} // namespace tmd

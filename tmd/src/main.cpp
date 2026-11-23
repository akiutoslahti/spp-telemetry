#include <array>
#include <cinttypes>
#include <cstddef>
#include <filesystem>
#include <format>
#include <iostream>
#include <signal.h> // NOLINT(modernize-deprecated-headers)
#include <span>
#include <string_view>
#include <sysexits.h>
#include <syslog.h>
#include <utility>
#include <variant>
#include <vector>

#include <tmd/descriptors.hpp>
#include <tmd/error.hpp>
#include <tmd/eventloop.hpp>
#include <tmd/options.hpp>

namespace {
// clang-format off
void print_help() {
  std::cout
      << "Usage: tmd [OPTION]...\n"
      << "Receive CCSDS Space Packets over UDP and keep per-APID statistics.\n"
      << "Statistics are queried over a Unix domain control socket.\n"
      << "\n"
      << "Options:\n"
      << "  --port PORT          UDP port to listen on, 1-65535 (default: " << tmd::UdpPortDefault << ")\n"
      << "  --socket PATH        control socket path (default: " << tmd::ControlSocketDefault << ")\n"
      << "  --pid-file PATH      pid file path (default: " << tmd::PidFileDefault << ")\n"
      << "  --log-level LEVEL    lowest syslog level to log (default: " << tmd::loglvl_to_string(tmd::LogLevelDefault) << ");\n"
      << "                       one of: emerg, alert, crit, err, warning,\n"
      << "                       notice, info, debug\n"
      << "  --help               show this help and exit\n"
      << "\n"
      << "Control socket commands (one per connection, newline-terminated):\n"
      << "  stats                print per-APID statistics and error counts\n"
      << "  reset                clear all statistics\n"
      << "\n"
      << "Values must not start with \"--\"; use \"./--name\" for such paths.\n"
      << "tmd runs in the foreground and stops on SIGTERM or SIGINT.\n";
}
// clang-format on

void log_error(int log_level, tmd::Error error, std::string_view msg) {
  auto formatted =
      std::format(R"({} (origin="{}", msg="{}"))", msg, to_string(error.origin),
                  error.condition.message());
  syslog(log_level, "%s", formatted.c_str());
}

void log_crit_error(tmd::Error error, std::string_view msg) {
  log_error(LOG_CRIT, error, msg);
}
} // namespace

// main only throws if memory allocation fails and that is acceptable
// NOLINTNEXTLINE(bugprone-exception-escape)
int main(int argc, char *argv[]) {
  if (argc < 1) {
    std::cerr << "Invalid argument count: " << argc << "\n";
    return EX_SOFTWARE;
  }

  auto nr_args = static_cast<std::size_t>(argc);
  std::vector<std::string_view> args;
  args.reserve(nr_args);
  for (const char *arg : std::span(argv, nr_args).subspan(1)) {
    args.emplace_back(arg);
  }

  auto parse_result = tmd::parse_cli_args(args);
  if (!parse_result.has_value()) {
    auto error = parse_result.error();
    std::cerr << "Attempt to parse CLI arguments resulted in error (kind=\""
              << to_string(error.kind) << "\", token=\"" << error.token
              << "\")\n\nTry \"tmd --help\".\n";
    return EX_USAGE;
  }

  const auto &value = parse_result.value();
  if (std::holds_alternative<tmd::HelpRequested>(value)) {
    print_help();
    return EX_OK;
  }

  auto config = *std::get_if<tmd::Config>(&value);
  openlog("tmd", LOG_PID | LOG_PERROR, LOG_DAEMON);
  (void)setlogmask(LOG_UPTO(config.log_level));

  auto maybe_signal_fd =
      tmd::create_signalfd(std::array<const int, 2>{SIGTERM, SIGINT});
  if (!maybe_signal_fd.has_value()) {
    log_crit_error(maybe_signal_fd.error(),
                   "Failed to create a file descriptor for accepting signals");
    return EX_SOFTWARE;
  }
  auto signal_fd = std::move(maybe_signal_fd).value();

  auto maybe_udp_socket = tmd::create_udp_socket(config.udp_port);
  if (!maybe_udp_socket.has_value()) {
    log_crit_error(maybe_udp_socket.error(),
                   "Failed to create an endpoint for UDP communication");
    return EX_SOFTWARE;
  }
  auto udp_socket = std::move(maybe_udp_socket).value();

  auto maybe_control_socket = tmd::create_control_socket(config.control_socket);
  if (!maybe_control_socket.has_value()) {
    log_crit_error(maybe_control_socket.error(),
                   "Failed to create an endpoint for IPC communication");
    return EX_SOFTWARE;
  }
  auto control_socket = std::move(maybe_control_socket).value();

  auto maybe_epoll_fd = tmd::create_epoll();
  if (!maybe_epoll_fd.has_value()) {
    log_crit_error(maybe_epoll_fd.error(),
                   "Failed to open an epoll file descriptor");
    return EX_SOFTWARE;
  }
  auto epoll_fd = std::move(maybe_epoll_fd).value();

  auto maybe_eventloop =
      tmd::EventLoop::create(std::move(udp_socket), std::move(control_socket),
                             std::move(signal_fd), std::move(epoll_fd));
  if (!maybe_eventloop.has_value()) {
    log_crit_error(maybe_eventloop.error(), "Failed to create event loop");
    return EX_SOFTWARE;
  }

  syslog(LOG_INFO,
         "tmd started: listening on UDP %" PRIu16 ", control socket %s",
         config.udp_port, config.control_socket.c_str());

  auto eventloop = std::move(maybe_eventloop).value();
  auto eventloop_result = eventloop.run();
  if (!eventloop_result.has_value()) {
    log_crit_error(eventloop_result.error(),
                   "Eventloop returned error mid execution");
    return EX_SOFTWARE;
  }

  auto remove_error_code = std::error_code();
  (void)std::filesystem::remove(config.control_socket, remove_error_code);
  if (remove_error_code) {
    syslog(LOG_WARNING,
           R"(Attempt to delete control socket at "%s" failed with "%s")",
           config.control_socket.c_str(), remove_error_code.message().c_str());
  }

  return EX_OK;
}

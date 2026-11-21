#include <arpa/inet.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <netinet/in.h>
#include <signal.h> // NOLINT(modernize-deprecated-headers)
#include <span>
#include <stdlib.h> // NOLINT(modernize-deprecated-headers)
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <system_error>
#include <unistd.h>
#include <utility>

#include <spp/error.hpp>
#include <spp/packet.hpp>
#include <tmd/descriptors.hpp>
#include <tmd/error.hpp>
#include <tmd/eventloop.hpp>
#include <tmd/statistics.hpp>
#include <tmd/uniquefd.hpp>

#include "test_common.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
ssize_t udp_send(int fd, std::uint16_t port, std::span<const std::byte> data) {
  sockaddr_in server_addr = {};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
  auto rc = sendto(fd, data.data(), data.size(), 0,
                   reinterpret_cast<const struct sockaddr *>(&server_addr),
                   sizeof(server_addr));
  // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
  return rc;
}

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class EventLoopFixture {
public:
  // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
  std::string dir_path;
  sigset_t orig_mask = {};
  // NOLINTEND(misc-non-private-member-variables-in-classes)

  EventLoopFixture() {
    dir_path = tmdtest::make_tmp_dir("tmd_test_eventloop_XXXXXX");
    REQUIRE_FALSE(dir_path.empty());

    REQUIRE(sigemptyset(&orig_mask) == 0);
    REQUIRE(sigprocmask(SIG_BLOCK, nullptr, &orig_mask) == 0);
  }

  ~EventLoopFixture() {
    auto remove_error_code = std::error_code();
    // best-effort rollback
    (void)std::filesystem::remove_all(dir_path, remove_error_code);

    sigset_t sset;
    const timespec tspec = {.tv_sec = 0, .tv_nsec = 0};
    (void)sigemptyset(&sset);
    (void)sigaddset(&sset, SIGUSR1);
    (void)sigtimedwait(&sset, nullptr, &tspec);
    (void)sigprocmask(SIG_SETMASK, &orig_mask, nullptr);
  }
};
} // namespace

TEST_CASE_METHOD(EventLoopFixture, "EventLoop create success smoketest",
                 "[EventLoop]") {
  auto udp_fd = tmd::create_udp_socket(0);
  REQUIRE(udp_fd.has_value());

  auto signal_fd = tmd::create_signalfd(std::array<const int, 1>{SIGUSR1});
  REQUIRE(signal_fd.has_value());

  const std::string path = dir_path + "/tmd_control_socket";
  if (path.size() > sizeof(sockaddr_un::sun_path) - 1) {
    SKIP("tmp path too long");
  }
  auto control_socket_fd = tmd::create_control_socket(path);
  REQUIRE(control_socket_fd.has_value());

  auto epoll_fd = tmd::create_epoll();
  REQUIRE(epoll_fd.has_value());

  auto evl =
      tmd::EventLoop::create(*std::move(udp_fd), *std::move(control_socket_fd),
                             *std::move(signal_fd), *std::move(epoll_fd));
  REQUIRE(evl.has_value());
}

TEST_CASE("EventLoop create failure smoketest", "[EventLoop]") {
  auto udp_fd = tmd::create_udp_socket(0);
  REQUIRE(udp_fd.has_value());
  auto fd2 = tmd::UniqueFd();
  auto fd3 = tmd::UniqueFd();
  auto fd4 = tmd::UniqueFd();

  auto evl = tmd::EventLoop::create(*std::move(udp_fd), std::move(fd2),
                                    std::move(fd3), std::move(fd4));
  REQUIRE(!evl.has_value());
  REQUIRE(evl.error().origin == tmd::ErrorOrigin::EpollCtlAdd);
  REQUIRE(evl.error().condition == std::errc::bad_file_descriptor);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE_METHOD(EventLoopFixture, "EventLoop run smoketest", "[EventLoop]") {
  auto udp_server_fd = tmd::create_udp_socket(0);
  REQUIRE(udp_server_fd.has_value());

  auto signal_fd = tmd::create_signalfd(std::array<const int, 1>{SIGUSR1});
  REQUIRE(signal_fd.has_value());

  const std::string tmp_path = dir_path + "/tmd_control_socket";
  if (tmp_path.size() > sizeof(sockaddr_un::sun_path) - 1) {
    SKIP("tmp path too long");
  }
  auto control_socket_fd = tmd::create_control_socket(tmp_path);
  REQUIRE(control_socket_fd.has_value());

  auto epoll_fd = tmd::create_epoll();
  REQUIRE(epoll_fd.has_value());

  auto udp_server_port = tmdtest::get_udp_socket_port(*udp_server_fd);

  auto eventloop_create_res = tmd::EventLoop::create(
      *std::move(udp_server_fd), *std::move(control_socket_fd),
      *std::move(signal_fd), *std::move(epoll_fd));
  REQUIRE(eventloop_create_res.has_value());
  auto eventloop = std::move(eventloop_create_res).value();

  auto udp_client_fd = tmd::create_udp_socket(0);
  REQUIRE(udp_client_fd.has_value());

  std::array<std::byte, tmdtest::FakePacketSize> pkt_buf = {};
  auto pkt_size = spp::serialize_packet(tmdtest::FakePacket, pkt_buf);
  REQUIRE(pkt_size.has_value());
  REQUIRE(udp_send(udp_client_fd->get(), udp_server_port, pkt_buf) ==
          tmdtest::FakePacketSize);

  auto packet = tmdtest::FakePacket;
  packet.header.sequence_count =
      *spp::SequenceCount::create(packet.header.sequence_count.next());
  pkt_size = spp::serialize_packet(packet, pkt_buf);
  REQUIRE(udp_send(udp_client_fd->get(), udp_server_port, pkt_buf) ==
          tmdtest::FakePacketSize);

  REQUIRE(udp_send(udp_client_fd->get(), udp_server_port,
                   std::span(pkt_buf).first(3)) == 3);

  auto conn_fd = tmd::UniqueFd(socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0));
  sockaddr_un addr = {};
  addr.sun_family = AF_UNIX;
  memcpy(&addr.sun_path, tmp_path.data(), tmp_path.length());
  // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
  REQUIRE(connect(conn_fd.get(),
                  reinterpret_cast<const struct sockaddr *>(&addr),
                  sizeof(addr)) == 0);
  // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
  std::string cmd = "stats\n";
  REQUIRE(write(conn_fd.get(), cmd.data(), cmd.size()) >= 0);

  REQUIRE(raise(SIGUSR1) == 0);

  auto eventloop_res = eventloop.run();
  REQUIRE(eventloop_res);

  std::string reply;
  // NOLINTNEXTLINE(readability-magic-numbers)
  std::array<char, 128> read_buf = {};
  for (;;) {
    const ssize_t rc = read(conn_fd.get(), read_buf.data(), sizeof(read_buf));
    REQUIRE(rc >= 0);
    if (rc == 0) {
      break;
    }
    const auto read_bytes = static_cast<std::size_t>(rc);
    reply.append(read_buf.data(), read_bytes);
  }
  const std::string expected_reply =
      "Valid packets:\n  APID=0x0, packets=2, bytes=14, gaps=0, lost=0, "
      "duplicates=0\nErrors: 'Packet Primary Header is too short'=1\n";
  REQUIRE(reply == expected_reply);

  const auto &stats = eventloop.get_stats();
  tmd::StatisticsEntry expected_entry = {.nr_packets = 2,
                                         .nr_bytes =
                                             2 * tmdtest::FakePacketSize,
                                         .nr_gap_events = 0,
                                         .nr_lost_packets = 0,
                                         .nr_duplicates = 0};
  REQUIRE(stats.get(*tmdtest::FakeApid) == expected_entry);

  REQUIRE(stats.get_errors()[static_cast<std::size_t>(
              spp::Error::PrimaryHeaderTooShort)] == 1);
}

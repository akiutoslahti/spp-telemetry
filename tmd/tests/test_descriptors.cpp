#include <tmd/descriptors.hpp>
#include <tmd/error.hpp>
#include <tmd/uniquefd.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <netinet/in.h>
#include <signal.h> // NOLINT(modernize-deprecated-headers)
#include <stdlib.h> // NOLINT(modernize-deprecated-headers)
#include <string>
#include <sys/poll.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <system_error>
#include <unistd.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_tostring.hpp>

namespace Catch {
template <> struct StringMaker<tmd::ErrorOrigin> {
  static std::string convert(tmd::ErrorOrigin const &origin) {
    return std::string(to_string(origin));
  }
};

template <> struct StringMaker<std::error_condition> {
  static std::string convert(std::error_condition const &cond) {
    return cond.message();
  }
};

template <> struct StringMaker<std::errc> {
  static std::string convert(std::errc const &err) {
    const std::error_condition cond = err;
    return cond.message();
  }
};
} // namespace Catch

namespace {
std::uint16_t get_udp_socket_port(const tmd::UniqueFd &fd) {
  sockaddr_in addr = {};
  socklen_t addr_len = sizeof(addr);
  // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
  const int rc = getsockname(
      fd.get(), reinterpret_cast<struct sockaddr *>(&addr), &addr_len);
  // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
  REQUIRE(rc == 0);
  REQUIRE(addr_len == sizeof(addr));
  return ntohs(addr.sin_port);
}

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class ControlSocketFixture {
public:
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::string dir_path;
  ControlSocketFixture() {
    std::string dir_path_template =
        std::filesystem::temp_directory_path().generic_string() +
        "/tmd_test_descriptors_XXXXXX";
    auto *res = mkdtemp(dir_path_template.data());
    REQUIRE(res != nullptr);
    dir_path = res;
    REQUIRE_FALSE(dir_path.empty());
  }

  ~ControlSocketFixture() {
    auto remove_error_code = std::error_code();
    // best-effort rollback
    (void)std::filesystem::remove_all(dir_path, remove_error_code);
  }
};
} // namespace

TEST_CASE("UDP on port 0", "[UdpSocket]") {
  auto fd = tmd::create_udp_socket(0);
  REQUIRE(fd);
  const auto port = get_udp_socket_port(*fd);
  REQUIRE(port != 0);
}

TEST_CASE("UDP port already in use", "[UdpSocket]") {
  auto fd = tmd::create_udp_socket(0);
  REQUIRE(fd);

  const auto port = get_udp_socket_port(*fd);
  auto fd2 = tmd::create_udp_socket(port);
  REQUIRE_FALSE(fd2);
  REQUIRE(fd2.error().origin == tmd::ErrorOrigin::SyscallBind);
  REQUIRE(fd2.error().condition == std::errc::address_in_use);
}

TEST_CASE("Signalfd with an invalid signal number", "[Signalfd]") {
  // NOLINTNEXTLINE(readability-magic-numbers)
  auto fd = tmd::create_signalfd(std::array<const int, 1>{999});
  REQUIRE_FALSE(fd);
  REQUIRE(fd.error().origin == tmd::ErrorOrigin::LibcCallSigaddset);
  REQUIRE(fd.error().condition == std::errc::invalid_argument);
}

TEST_CASE("Signalfd is created successfully and it preserves old mask",
          "[Signalfd]") {
  sigset_t mask = {};
  sigset_t orig_mask = {};
  REQUIRE(sigemptyset(&mask) == 0);
  REQUIRE(sigaddset(&mask, SIGUSR2) == 0);
  REQUIRE(sigemptyset(&orig_mask) == 0);
  REQUIRE(sigprocmask(SIG_BLOCK, &mask, &orig_mask) == 0);

  auto fd = tmd::create_signalfd(std::array<const int, 1>{SIGUSR1});
  REQUIRE(fd);

  CHECK(sigemptyset(&mask) == 0);
  CHECK(sigprocmask(SIG_BLOCK, nullptr, &mask) == 0);
  CHECK(sigismember(&mask, SIGUSR1) == 1);
  CHECK(sigismember(&mask, SIGUSR2) == 1);

  CHECK(raise(SIGUSR1) == 0);

  pollfd pfd = {};
  pfd.fd = fd->get();
  pfd.events = POLLIN;
  const int rc = poll(&pfd, 1, 0);
  CHECK(rc > 0);
  CHECK(pfd.revents == POLLIN);

  signalfd_siginfo ssi = {};
  CHECK(read(fd->get(), &ssi, sizeof(ssi)) == sizeof(ssi));
  CHECK(ssi.ssi_signo == SIGUSR1);

  CHECK(sigprocmask(SIG_SETMASK, &orig_mask, nullptr) == 0);
}

TEST_CASE("Epoll is created successfully", "[Epoll]") {
  auto fd = tmd::create_epoll();
  REQUIRE(fd);
}

TEST_CASE_METHOD(ControlSocketFixture,
                 "Control socket in a temporary directory", "[ControlSocket]") {
  auto path = dir_path + "/tmd_control_socket";
  if (path.size() > sizeof(sockaddr_un::sun_path) - 1) {
    SKIP("tmp path too long");
  }
  auto fd = tmd::create_control_socket(path);
  REQUIRE(fd);
  REQUIRE(std::filesystem::exists(path));
}

TEST_CASE_METHOD(ControlSocketFixture,
                 "Control socket with a path of maximum length",
                 "[ControlSocket]") {
  auto path_len_max = sizeof(sockaddr_un::sun_path) - 1;
  auto path = std::string(path_len_max, 'A');
  auto path_prefix = dir_path + "/tmd_control_socket";
  if (path_prefix.size() > path_len_max) {
    SKIP("tmp path too long");
  }
  std::copy(path_prefix.begin(), path_prefix.end(), path.begin());
  auto fd = tmd::create_control_socket(path);
  REQUIRE(fd);
  REQUIRE(std::filesystem::exists(path));
}

TEST_CASE_METHOD(ControlSocketFixture,
                 "Control socket with a path one byte too long",
                 "[ControlSocket]") {
  auto path_len_too_large = sizeof(sockaddr_un::sun_path);
  auto path = std::string(path_len_too_large, 'A');
  auto path_prefix = dir_path + "/tmd_control_socket";
  if (path_prefix.size() > path.size()) {
    path = path_prefix;
  } else {
    std::copy(path_prefix.begin(), path_prefix.end(), path.begin());
  }
  auto fd = tmd::create_control_socket(path);
  REQUIRE_FALSE(fd);
  REQUIRE(fd.error().origin == tmd::ErrorOrigin::ControlSocketPathValidation);
  REQUIRE(fd.error().condition == std::errc::filename_too_long);
  REQUIRE_FALSE(std::filesystem::exists(path));
}

TEST_CASE("Control socket with an empty path", "[ControlSocket]") {
  auto fd = tmd::create_control_socket("");
  REQUIRE_FALSE(fd);
  REQUIRE(fd.error().origin == tmd::ErrorOrigin::ControlSocketPathValidation);
  REQUIRE(fd.error().condition == std::errc::invalid_argument);
}

TEST_CASE_METHOD(ControlSocketFixture,
                 "Control socket bound twice on the same path",
                 "[ControlSocket]") {
  auto path = dir_path + "/tmd_control_socket";
  if (path.size() > sizeof(sockaddr_un::sun_path) - 1) {
    SKIP("tmp path too long");
  }
  auto fd = tmd::create_control_socket(path);
  REQUIRE(fd);

  auto fd2 = tmd::create_control_socket(path);
  REQUIRE_FALSE(fd2);
  REQUIRE(fd2.error().origin == tmd::ErrorOrigin::SyscallBind);
  REQUIRE(fd2.error().condition == std::errc::address_in_use);

  REQUIRE(std::filesystem::exists(path));
}

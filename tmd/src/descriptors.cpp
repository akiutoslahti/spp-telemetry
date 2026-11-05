#include <spp/expected.hpp>
#include <tmd/error.hpp>
#include <tmd/uniquefd.hpp>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <netinet/in.h>
#include <signal.h> // NOLINT(modernize-deprecated-headers)
#include <span>
#include <string_view>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <system_error>

namespace {
spp::Unexpected<tmd::Error> capture_errno(tmd::ErrorOrigin origin) {
  const int errno_copy = errno;
  return spp::Unexpected(tmd::Error{
      .origin = origin,
      .condition = std::system_category().default_error_condition(errno_copy)});
}
} // namespace

namespace tmd {
spp::Expected<UniqueFd, Error> create_udp_socket(std::uint16_t port) {
  UniqueFd fd =
      UniqueFd(socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0));
  if (!fd) {
    return capture_errno(ErrorOrigin::SyscallSocket);
  }

  sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;
  // Can't be avoided
  // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
  if (bind(fd.get(), reinterpret_cast<const struct sockaddr *>(&addr),
           sizeof(addr)) != 0) {
    return capture_errno(ErrorOrigin::SyscallBind);
  }
  // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

  return fd;
}

spp::Expected<UniqueFd, Error>
create_signalfd(std::span<const int> signal_set) {
  sigset_t new_mask;
  sigset_t old_mask;
  // Can only fail with NULL argument
  (void)sigemptyset(&new_mask);
  for (const int sig : signal_set) {
    if (sigaddset(&new_mask, sig) != 0) {
      return capture_errno(ErrorOrigin::LibcCallSigaddset);
    }
  }

  if (sigprocmask(SIG_BLOCK, &new_mask, &old_mask) != 0) {
    return capture_errno(ErrorOrigin::SyscallSigprocmask);
  }

  UniqueFd fd = UniqueFd(signalfd(-1, &new_mask, SFD_NONBLOCK | SFD_CLOEXEC));
  if (!fd) {
    auto ret = capture_errno(ErrorOrigin::SyscallSignalfd);
    // We are already in error return so do best-effort rollback
    (void)sigprocmask(SIG_SETMASK, &old_mask, nullptr);
    return ret;
  }

  return fd;
}

spp::Expected<UniqueFd, Error> create_epoll() {
  UniqueFd fd = UniqueFd(epoll_create1(EPOLL_CLOEXEC));
  if (!fd) {
    return capture_errno(ErrorOrigin::SyscallEpollCreate1);
  }

  return fd;
}

spp::Expected<UniqueFd, Error> create_control_socket(std::string_view path) {
  sockaddr_un addr = {};
  if (path.empty()) {
    return spp::Unexpected(
        Error{.origin = ErrorOrigin::ControlSocketPathValidation,
              .condition = std::errc::invalid_argument});
  }
  if (path.length() >= sizeof(addr.sun_path)) {
    return spp::Unexpected(
        Error{.origin = ErrorOrigin::ControlSocketPathValidation,
              .condition = std::errc::filename_too_long});
  }

  UniqueFd fd =
      UniqueFd(socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0));
  if (!fd) {
    return capture_errno(ErrorOrigin::SyscallSocket);
  }

  addr.sun_family = AF_UNIX;
  memcpy(&addr.sun_path, path.data(), path.length());
  // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
  if (bind(fd.get(), reinterpret_cast<const struct sockaddr *>(&addr),
           sizeof(addr)) != 0) {
    return capture_errno(ErrorOrigin::SyscallBind);
  }
  // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

  if (listen(fd.get(), SOMAXCONN) != 0) {
    auto ret = capture_errno(ErrorOrigin::SyscallListen);
    auto remove_error_code = std::error_code();
    // We are already in error return so do best-effort rollback
    (void)std::filesystem::remove(path, remove_error_code);
    return ret;
  }

  return fd;
}
} // namespace tmd
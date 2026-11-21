#pragma once

#include <cstdint>
#include <string_view>
#include <system_error>

#include <spp/expected.hpp>

namespace tmd {
enum class ErrorOrigin : std::uint8_t {
  SyscallSocket,
  SyscallBind,
  LibcCallSigaddset,
  SyscallSigprocmask,
  SyscallSignalfd,
  SyscallEpollCreate1,
  SyscallListen,
  ControlSocketPathValidation,
  EpollCtlAdd,
  EpollCtlMod,
  EpollWait,
};

[[nodiscard]] constexpr std::string_view
to_string(ErrorOrigin origin) noexcept {
  switch (origin) {
  case tmd::ErrorOrigin::SyscallSocket:
    return "system call: socket()";
  case tmd::ErrorOrigin::SyscallBind:
    return "system call: bind()";
  case tmd::ErrorOrigin::LibcCallSigaddset:
    return "libc call: sigaddset()";
  case tmd::ErrorOrigin::SyscallSigprocmask:
    return "system call: sigprocmask()";
  case tmd::ErrorOrigin::SyscallSignalfd:
    return "system call: signalfd()";
  case tmd::ErrorOrigin::SyscallEpollCreate1:
    return "system call: epoll_create1()";
  case tmd::ErrorOrigin::SyscallListen:
    return "system call: listen()";
  case tmd::ErrorOrigin::ControlSocketPathValidation:
    return "control socket: path validation";
  case tmd::ErrorOrigin::EpollCtlAdd:
    return "system call: epoll_ctl(..., EPOLL_CTL_ADD, ...)";
  case tmd::ErrorOrigin::EpollCtlMod:
    return "system call: epoll_ctl(..., EPOLL_CTL_MOD, ...)";
  case tmd::ErrorOrigin::EpollWait:
    return "system call: epoll_wait()";
  }
  return "unknown origin";
}

struct Error {
  ErrorOrigin origin;
  std::error_condition condition;

  [[nodiscard]] friend bool operator==(const Error &,
                                       const Error &) noexcept = default;
};

spp::Unexpected<Error> capture_errno(ErrorOrigin origin);
} // namespace tmd

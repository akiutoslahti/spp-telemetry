#pragma once

#include <cstdint>
#include <format>
#include <string_view>
#include <system_error>

namespace tmd {
enum class ErrorOrigin : std::uint8_t {
  SyscallSocket,
  SyscallBind,
  LibcCallSigaddset,
  SyscallSigprocmask,
  SyscallSignalfd,
  SyscallEpollCreate1,
  SyscallListen,
  ControlSocketPathValidation
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
  }
  return "unknown origin";
}

struct Error {
  ErrorOrigin origin;
  std::error_condition condition;

  [[nodiscard]] friend bool operator==(const Error &,
                                       const Error &) noexcept = default;
};
} // namespace tmd

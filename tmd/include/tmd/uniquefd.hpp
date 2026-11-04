#pragma once

#include <unistd.h>
#include <utility>

namespace tmd {

class UniqueFd {
private:
  static constexpr int EmptyFd = -1;
  int m_fd;

public:
  constexpr UniqueFd() noexcept : m_fd{EmptyFd} {}

  explicit constexpr UniqueFd(int fd) noexcept : m_fd{fd} {}

  ~UniqueFd() noexcept {
    if (m_fd != EmptyFd) {
      (void)close(m_fd);
      // FIXME log rc later, once syslog exists
      m_fd = EmptyFd;
    }
  }

  constexpr UniqueFd(const UniqueFd &) = delete;

  constexpr UniqueFd &operator=(const UniqueFd &) = delete;

  constexpr UniqueFd(UniqueFd &&other) noexcept
      : m_fd{std::exchange(other.m_fd, EmptyFd)} {}

  UniqueFd &operator=(UniqueFd &&other) noexcept {
    reset(other.release());
    return *this;
  }

  [[nodiscard]] constexpr int get() const noexcept { return m_fd; }

  [[nodiscard]] constexpr int release() noexcept {
    return std::exchange(m_fd, EmptyFd);
  }

  void reset(int fd = EmptyFd) noexcept {
    if (fd != m_fd) {
      if (m_fd != EmptyFd) {
        (void)close(m_fd);
        // FIXME log rc later, once syslog exists
      }
      m_fd = fd;
    }
  }

  constexpr explicit operator bool() const noexcept { return m_fd != EmptyFd; }
};
} // namespace tmd

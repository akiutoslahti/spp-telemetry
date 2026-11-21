#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>
#include <variant>

#include <spp/expected.hpp>
#include <spp/packet.hpp>
#include <tmd/control.hpp>
#include <tmd/error.hpp>
#include <tmd/eventloop.hpp>
#include <tmd/uniquefd.hpp>

namespace {
constexpr std::size_t EpollBatchSize = 10;
constexpr std::size_t NrConnectionsMax = 16;
constexpr std::size_t CommandMaxLen = 128;

spp::Expected<std::monostate, tmd::Error>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
epoll_register_fd(int efd, int fd, std::uint32_t flags) {
  epoll_event eev = {};
  eev.events = flags;
  eev.data.fd = fd;

  if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &eev) != 0) {
    return tmd::capture_errno(tmd::ErrorOrigin::EpollCtlAdd);
  }

  return std::monostate{};
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
spp::Expected<std::monostate, tmd::Error> epoll_modify_fd(int efd, int fd,
                                                          std::uint32_t flags) {
  epoll_event eev = {};
  eev.events = flags;
  eev.data.fd = fd;

  if (epoll_ctl(efd, EPOLL_CTL_MOD, fd, &eev) != 0) {
    return tmd::capture_errno(tmd::ErrorOrigin::EpollCtlMod);
  }

  return std::monostate{};
}

enum class ReadIntoResult : std::uint8_t {
  Open,
  Eof,
  TooLong,
  Error,
};

ReadIntoResult read_into(int fd, std::string &out_buf) {
  std::array<char, CommandMaxLen> read_buf = {};

  for (;;) {
    const ssize_t rc = read(fd, read_buf.data(), read_buf.size());

    if (rc == -1) {
      const int errno_local = errno;

      if (errno_local == EINTR) {
        continue;
      }

      if (errno_local == EAGAIN || errno_local == EWOULDBLOCK) {
        return ReadIntoResult::Open;
      }

      return ReadIntoResult::Error;
    }

    if (rc == 0) {
      return ReadIntoResult::Eof;
    }

    const auto read_size = static_cast<std::size_t>(rc);

    if (out_buf.size() + read_size > CommandMaxLen) {
      return ReadIntoResult::TooLong;
    }

    out_buf.append(read_buf.data(), read_size);
  }
}

enum class WriteFromResult : std::uint8_t { Done, WouldBlock, Error };

WriteFromResult write_from(int fd, std::string_view reply,
                           std::size_t &offset) {
  for (;;) {
    auto reply_part = reply.substr(offset);
    const ssize_t rc = write(fd, reply_part.data(), reply_part.size());

    if (rc == -1) {
      const int errno_local = errno;

      if (errno_local == EINTR) {
        continue;
      }

      if (errno_local == EAGAIN || errno_local == EWOULDBLOCK) {
        return WriteFromResult::WouldBlock;
      }

      return WriteFromResult::Error;
    }

    const auto write_size = static_cast<std::size_t>(rc);
    offset += write_size;

    if (offset == reply.size()) {
      return WriteFromResult::Done;
    }
  }
}
} // namespace

namespace tmd {
void EventLoop::handle_datagrams() {
  for (;;) {
    const ssize_t rc =
        recv(m_udp_socket.get(), m_recv_buf.data(), m_recv_buf.size(), 0);

    if (rc >= 0) {
      auto pkt_size = static_cast<std::size_t>(rc);
      auto src = std::span(m_recv_buf.data(), pkt_size);
      auto pkt = spp::parse_packet(src);
      m_stats.record(pkt);
    } else {
      const int errno_local = errno;

      if (errno_local == EINTR) {
        continue;
      }

      if (errno_local != EAGAIN && errno_local != EWOULDBLOCK) {
        /* TODO Log error once syslog exists */
      }

      return;
    }
  }
}

void EventLoop::handle_signals() {
  signalfd_siginfo ssi = {};

  for (;;) {
    const ssize_t rc = read(m_signal_fd.get(), &ssi, sizeof(ssi));

    if (rc == sizeof(ssi)) {
      m_stop_flag = true;
      return;
    }

    if (rc == -1) {
      const int errno_local = errno;

      if (errno_local == EINTR) {
        continue;
      }

      if (errno_local != EAGAIN && errno_local != EWOULDBLOCK) {
        /* TODO Log error once syslog exists */
        m_stop_flag = true;
      }

      return;
    }

    /* TODO Log error once syslog exists */
    m_stop_flag = true;
    return;
  }
}

void EventLoop::accept_connections() {
  for (;;) {
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    auto fd = UniqueFd(accept4(m_control_socket.get(), nullptr, nullptr,
                               SOCK_NONBLOCK | SOCK_CLOEXEC));
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

    if (!fd) {
      const int errno_local = errno;

      if (errno_local == EAGAIN || errno_local == EWOULDBLOCK) {
        return;
      }

      if (errno_local == EINTR || errno_local == ECONNABORTED) {
        continue;
      }

      /* TODO Log error once syslog exists */
      return;
    }

    if (m_connections.size() >= NrConnectionsMax) {
      continue;
    }

    auto ret = epoll_register_fd(m_epoll_fd.get(), fd.get(), EPOLLIN);
    if (ret) {
      const int fd_local = fd.get();
      m_connections.insert({fd_local, Connection{.state = ConnectionState::Read,
                                                 .fd = std::move(fd),
                                                 .read_buf = "",
                                                 .write_buf = "",
                                                 .write_offs = 0}});
      handle_connection(fd_local);
    }
  }
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void EventLoop::handle_connection(int fd) {
  auto conn_it = m_connections.find(fd);
  if (conn_it == m_connections.end()) {
    return;
  }

  auto &conn = conn_it->second;

  if (conn.state == ConnectionState::Read) {
    auto read_res = read_into(conn.fd.get(), conn.read_buf);

    if (read_res == ReadIntoResult::Error) {
      m_connections.erase(conn_it);
      return;
    }

    if (read_res == ReadIntoResult::TooLong) {
      conn.write_buf = "Command is too long\n";
      conn.state = ConnectionState::Write;
    } else {
      auto cmd = find_command(conn.read_buf, read_res == ReadIntoResult::Eof);

      if (!cmd.has_value()) {
        if (read_res == ReadIntoResult::Eof) {
          m_connections.erase(conn_it);
        }

        return;
      }

      conn.write_buf = tmd::execute_command(cmd.value(), m_stats);
      conn.state = ConnectionState::Write;
    }
  }

  if (conn.state == ConnectionState::Write) {
    auto write_res = write_from(conn.fd.get(), conn.write_buf, conn.write_offs);

    if (write_res == WriteFromResult::Done ||
        write_res == WriteFromResult::Error) {
      m_connections.erase(conn_it);
      return;
    }

    if (write_res == WriteFromResult::WouldBlock) {
      auto epoll_res =
          epoll_modify_fd(m_epoll_fd.get(), conn.fd.get(), EPOLLOUT);

      if (!epoll_res) {
        m_connections.erase(conn_it);
      }

      return;
    }
  }
}

spp::Expected<std::monostate, Error> EventLoop::run() {
  std::array<epoll_event, EpollBatchSize> events = {};

  for (;;) {
    auto rc = epoll_wait(m_epoll_fd.get(), events.data(), EpollBatchSize, -1);

    if (rc == -1) {
      const int errno_local = errno;
      if (errno_local == EINTR) {
        continue;
      }
      return capture_errno(ErrorOrigin::EpollWait);
    }

    const auto &event_count = static_cast<std::size_t>(rc);
    for (std::size_t i = 0; i < event_count; i++) {
      auto event = events.at(i);
      if (event.data.fd == m_udp_socket.get()) {
        handle_datagrams();
      } else if (event.data.fd == m_signal_fd.get()) {
        handle_signals();
      } else if (event.data.fd == m_control_socket.get()) {
        accept_connections();
      } else {
        handle_connection(event.data.fd);
      }
    }

    if (m_stop_flag) {
      break;
    }
  }

  return std::monostate{};
}

spp::Expected<EventLoop, Error> EventLoop::create(UniqueFd udp_socket,
                                                  UniqueFd control_socket,
                                                  UniqueFd signal_fd,
                                                  UniqueFd epoll_fd) {
  auto ret = epoll_register_fd(epoll_fd.get(), udp_socket.get(), EPOLLIN);
  if (!ret) {
    return spp::Unexpected(ret.error());
  }

  ret = epoll_register_fd(epoll_fd.get(), control_socket.get(), EPOLLIN);
  if (!ret) {
    return spp::Unexpected(ret.error());
  }

  ret = epoll_register_fd(epoll_fd.get(), signal_fd.get(), EPOLLIN);
  if (!ret) {
    return spp::Unexpected(ret.error());
  }

  return EventLoop(std::move(udp_socket), std::move(control_socket),
                   std::move(signal_fd), std::move(epoll_fd));
}
} // namespace tmd

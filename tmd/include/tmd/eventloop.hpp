#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

#include <spp/expected.hpp>
#include <tmd/error.hpp>
#include <tmd/statistics.hpp>
#include <tmd/uniquefd.hpp>

namespace tmd {
class EventLoop {
private:
  static constexpr std::size_t ReceiveBufferSize =
      spp::PrimaryHeaderSize + spp::PacketDataSizeMax;

  enum class ConnectionState : std::uint8_t {
    Read,
    Write,
  };

  struct Connection {
    ConnectionState state;
    UniqueFd fd;
    std::string read_buf;
    std::string write_buf;
    std::size_t write_offs;
  };

  UniqueFd m_udp_socket;
  UniqueFd m_control_socket;
  UniqueFd m_signal_fd;
  UniqueFd m_epoll_fd;
  Statistics m_stats;
  std::array<std::byte, ReceiveBufferSize> m_recv_buf = {};
  std::unordered_map<int, Connection> m_connections;
  bool m_stop_flag = false;

  EventLoop(UniqueFd udp_socket, UniqueFd control_socket, UniqueFd signal_fd,
            UniqueFd epoll_fd)
      : m_udp_socket{std::move(udp_socket)},
        m_control_socket{std::move(control_socket)},
        m_signal_fd{std::move(signal_fd)}, m_epoll_fd{std::move(epoll_fd)} {}

  void handle_datagrams();

  void handle_signals();

  void accept_connections();

  void handle_connection(int fd);

public:
  [[nodiscard]] spp::Expected<std::monostate, Error> run();

  static spp::Expected<EventLoop, Error> create(UniqueFd udp_socket,
                                                UniqueFd control_socket,
                                                UniqueFd signal_fd,
                                                UniqueFd epoll_fd);

  [[nodiscard]] const Statistics &get_stats() const & noexcept {
    return m_stats;
  }
};
} // namespace tmd

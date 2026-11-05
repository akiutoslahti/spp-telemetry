#pragma once

#include <spp/expected.hpp>
#include <tmd/error.hpp>
#include <tmd/uniquefd.hpp>

#include <cstdint>
#include <span>
#include <string_view>

namespace tmd {
spp::Expected<UniqueFd, Error> create_udp_socket(std::uint16_t port);

spp::Expected<UniqueFd, Error> create_signalfd(std::span<const int> signals);

spp::Expected<UniqueFd, Error> create_epoll();

spp::Expected<UniqueFd, Error> create_control_socket(std::string_view path);
} // namespace tmd
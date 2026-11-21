#include <cstdint>
#include <filesystem>
#include <netinet/in.h>
#include <stdlib.h> // NOLINT(modernize-deprecated-headers)
#include <string>
#include <sys/socket.h>

#include <tmd/uniquefd.hpp>

#include "test_common.hpp"

#include <catch2/catch_test_macros.hpp>

namespace tmdtest {
std::string make_tmp_dir(const std::string &dir_name_template) {
  std::string dir_path_template =
      std::filesystem::temp_directory_path().generic_string() + "/" +
      dir_name_template;
  auto *res = mkdtemp(dir_path_template.data());
  REQUIRE(res != nullptr);
  return res;
}

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
} // namespace tmdtest

#include <cerrno>
#include <system_error>

#include <spp/expected.hpp>
#include <tmd/error.hpp>

namespace tmd {
spp::Unexpected<Error> capture_errno(ErrorOrigin origin) {
  const int errno_copy = errno;
  return spp::Unexpected(Error{
      .origin = origin,
      .condition = std::system_category().default_error_condition(errno_copy)});
}
} // namespace tmd

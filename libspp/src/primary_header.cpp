#include <spp/error.hpp>
#include <spp/expected.hpp>
#include <spp/primary_header.hpp>

#include <cstddef>
#include <span>

namespace spp {
Expected<std::size_t, Error>
serialize_header(PacketPrimaryHeader header,
                 std::span<std::byte> dst) noexcept {
  (void)header;
  (void)dst;

  return Unexpected(Error::Internal);
}

Expected<PacketPrimaryHeader, Error>
parse_header(std::span<const std::byte> src) noexcept {
  (void)src;

  return Unexpected(Error::Internal);
}
} // namespace spp

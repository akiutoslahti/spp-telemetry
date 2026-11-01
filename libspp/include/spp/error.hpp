#pragma once

#include <cstdint>
#include <string_view>

namespace spp {
enum class Error : std::uint8_t { Internal, InvalidApid, InvalidSequenceCount };

[[nodiscard]] constexpr std::string_view to_string(Error error) noexcept {
  switch (error) {
  case Error::Internal:
    return "Internal error";
  case Error::InvalidApid:
    return "Invalid APID";
  case Error::InvalidSequenceCount:
    return "Invalid Sequence Count";
  }

  return "Unknown error";
}
} // namespace spp

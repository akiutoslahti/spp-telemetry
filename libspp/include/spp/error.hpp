#pragma once

#include <cstdint>
#include <string_view>

namespace spp {
enum class Error : std::uint8_t { Internal };

[[nodiscard]] constexpr std::string_view to_string(Error error) noexcept {
  switch (error) {
  case Error::Internal:
    return "Internal error";
  }

  return "Unknown error";
}
} // namespace spp

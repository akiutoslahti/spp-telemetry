#pragma once

#include <type_traits>
#include <utility>

namespace spp {
template <class E> class Unexpected {
private:
  E m_unexp;

public:
  explicit constexpr Unexpected(E unexp) noexcept(
      std::is_nothrow_move_constructible_v<E>)
      : m_unexp{std::move(unexp)} {}

  [[nodiscard]] constexpr const E &error() const noexcept { return m_unexp; }
};
} // namespace spp

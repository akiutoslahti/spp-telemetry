#pragma once

#include <type_traits>
#include <utility>
#include <variant>

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

template <class T, class E> class Expected {
private:
  std::variant<T, E> m_exp;

public:
  constexpr Expected(T exp) noexcept(std::is_nothrow_move_constructible_v<T>)
      : m_exp(std::in_place_index<0>, std::move(exp)) {}

  // FIX This copies instead of moving
  constexpr Expected(Unexpected<E> unexp) noexcept(
      std::is_nothrow_move_constructible_v<E>)
      : m_exp(std::in_place_index<1>, std::move(unexp.error())) {}

  [[nodiscard]] constexpr bool has_value() const noexcept {
    return m_exp.index() == 0;
  }
};
} // namespace spp

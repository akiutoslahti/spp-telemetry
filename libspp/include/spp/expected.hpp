#pragma once

#include <cassert>
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

  [[nodiscard]] constexpr const E &error() const & noexcept { return m_unexp; }

  [[nodiscard]] constexpr E &&error() && noexcept { return std::move(m_unexp); }
};

template <class T, class E> class Expected {
private:
  std::variant<T, E> m_exp;

public:
  constexpr Expected(T exp) noexcept(std::is_nothrow_move_constructible_v<T>)
      : m_exp(std::in_place_index<0>, std::move(exp)) {}

  constexpr Expected(Unexpected<E> unexp) noexcept(
      std::is_nothrow_move_constructible_v<E>)
      : m_exp(std::in_place_index<1>, std::move(unexp).error()) {}

  [[nodiscard]] constexpr bool has_value() const noexcept {
    return m_exp.index() == 0;
  }

  [[nodiscard]] constexpr const T &value() const & noexcept {
    assert(has_value());
    return *std::get_if<0>(&m_exp);
  }

  [[nodiscard]] constexpr const E &error() const & noexcept {
    assert(!has_value());
    return *std::get_if<1>(&m_exp);
  }
};
} // namespace spp

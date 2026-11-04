#include <tmd/uniquefd.hpp>

#include <cerrno>
#include <fcntl.h>
#include <sys/eventfd.h>
#include <type_traits>
#include <unistd.h>
#include <utility>

#include <catch2/catch_test_macros.hpp>

namespace {
bool fd_is_closed(int fd) {
  // no other way around this call
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  const int rc = fcntl(fd, F_GETFD);
  return rc == -1 && errno == EBADF;
}
} // namespace

TEST_CASE("UniqueFd can't be copy constructed", "[UniqueFd]") {
  STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<tmd::UniqueFd>);
}

TEST_CASE("UniqueFd can't be copy assigned", "[UniqueFd]") {
  STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<tmd::UniqueFd>);
}

TEST_CASE("UniqueFd constructor (default) can't throw", "[UniqueFd]") {
  STATIC_REQUIRE(std::is_nothrow_constructible_v<tmd::UniqueFd>);
}

TEST_CASE("UniqueFd constructor (int) can't throw", "[UniqueFd]") {
  STATIC_REQUIRE(std::is_nothrow_constructible_v<tmd::UniqueFd, int>);
}

TEST_CASE("UniqueFd move constructor can't throw", "[UniqueFd]") {
  STATIC_REQUIRE(std::is_nothrow_move_constructible_v<tmd::UniqueFd>);
}

TEST_CASE("UniqueFd move assignment can't throw", "[UniqueFd]") {
  STATIC_REQUIRE(std::is_nothrow_move_assignable_v<tmd::UniqueFd>);
}

TEST_CASE("UniqueFd get can't throw", "[UniqueFd]") {
  STATIC_REQUIRE(noexcept(std::declval<tmd::UniqueFd>().get()));
}

TEST_CASE("UniqueFd release can't throw", "[UniqueFd]") {
  STATIC_REQUIRE(noexcept(std::declval<tmd::UniqueFd>().release()));
}

TEST_CASE("UniqueFd operator bool can't throw", "[UniqueFd]") {
  STATIC_REQUIRE(noexcept(std::declval<tmd::UniqueFd>().operator bool()));
}

TEST_CASE("UniqueFd can't be implicitly constructed from int", "[UniqueFd]") {
  STATIC_REQUIRE_FALSE(std::is_convertible_v<int, tmd::UniqueFd>);
}

TEST_CASE("UniqueFd operator bool is false for default-constructed object",
          "[UniqueFd]") {
  auto ufd = tmd::UniqueFd();
  REQUIRE_FALSE(ufd);
}

TEST_CASE("UniqueFd operator bool is false for moved-from object",
          "[UniqueFd]") {
  const int fd1 = eventfd(0, EFD_CLOEXEC);
  REQUIRE(fd1 > -1);
  auto ufd1 = tmd::UniqueFd(fd1);

  const int fd2 = eventfd(0, EFD_CLOEXEC);
  REQUIRE(fd2 > -1);
  auto ufd2 = tmd::UniqueFd(fd2);

  ufd1 = std::move(ufd2);
  REQUIRE_FALSE(ufd2);
}

TEST_CASE("UniqueFd operator bool is false for released object", "[UniqueFd]") {
  const int fd1 = eventfd(0, EFD_CLOEXEC);
  REQUIRE(fd1 > -1);
  auto ufd = tmd::UniqueFd(fd1);

  const int fd2 = ufd.release();
  CHECK_FALSE(ufd);

  (void)close(fd2);
}

TEST_CASE("UniqueFd operator bool is true for one owning a descriptor",
          "[UniqueFd]") {
  const int fd = eventfd(0, EFD_CLOEXEC);
  REQUIRE(fd > -1);
  auto ufd = tmd::UniqueFd(fd);
  REQUIRE(ufd);
}

TEST_CASE("UniqueFd default constructed state is empty", "[UniqueFd]") {
  auto ufd = tmd::UniqueFd();
  REQUIRE(ufd.get() == -1);
}

TEST_CASE(
    "UniqueFd default-constructed object is destroyed without calling close",
    "[UniqueFd]") {
  errno = 0;
  { auto ufd = tmd::UniqueFd(); }
  REQUIRE(errno == 0);
}

TEST_CASE("UniqueFd destructor closes fd", "[UniqueFd]") {
  const int fd = eventfd(0, EFD_CLOEXEC);
  REQUIRE(fd > -1);

  { auto ufd = tmd::UniqueFd(fd); }

  REQUIRE(fd_is_closed(fd));
}

TEST_CASE("UniqueFd move constructor leaves the source empty, and the target "
          "owns the fd",
          "[UniqueFd]") {
  const int fd = eventfd(0, EFD_CLOEXEC);
  REQUIRE(fd > -1);
  auto ufd1 = tmd::UniqueFd(fd);
  REQUIRE(ufd1.get() == fd);

  auto ufd2 = tmd::UniqueFd(std::move(ufd1));
  REQUIRE(ufd2.get() == fd);
  // deliberate to verify correctness
  // NOLINTNEXTLINE(bugprone-use-after-move)
  REQUIRE(ufd1.get() == -1);
}

TEST_CASE("UniqueFd move assignment into a non-empty target closes the "
          "target's old fd",
          "[UniqueFd]") {
  const int fd1 = eventfd(0, EFD_CLOEXEC);
  REQUIRE(fd1 > -1);
  auto ufd1 = tmd::UniqueFd(fd1);
  const int fd2 = eventfd(0, EFD_CLOEXEC);
  REQUIRE(fd2 > -1);
  auto ufd2 = tmd::UniqueFd(fd2);

  ufd1 = std::move(ufd2);
  REQUIRE(ufd1.get() == fd2);
  // deliberate to verify correctness
  // NOLINTNEXTLINE(bugprone-use-after-move)
  REQUIRE(ufd2.get() == -1);

  REQUIRE(fd_is_closed(fd1));
}

TEST_CASE("UniqueFd move assignment into a non-empty target from empty source "
          "should close target's old fd",
          "[UniqueFd]") {
  const int fd1 = eventfd(0, EFD_CLOEXEC);
  REQUIRE(fd1 > -1);
  auto ufd1 = tmd::UniqueFd(fd1);
  auto ufd2 = tmd::UniqueFd();

  ufd1 = std::move(ufd2);
  REQUIRE(ufd1.get() == -1);
  // deliberate to verify correctness
  // NOLINTNEXTLINE(bugprone-use-after-move)
  REQUIRE(ufd2.get() == -1);

  REQUIRE(fd_is_closed(fd1));
}

TEST_CASE("UniqueFd moved-from object is destroyed without calling close",
          "[UniqueFd]") {
  const int fd1 = eventfd(0, EFD_CLOEXEC);
  REQUIRE(fd1 > -1);
  auto ufd1 = tmd::UniqueFd(fd1);
  const int fd2 = eventfd(0, EFD_CLOEXEC);
  REQUIRE(fd2 > -1);

  {
    auto ufd2 = tmd::UniqueFd(fd2);
    errno = 0;
    ufd1 = std::move(ufd2);
  }
  REQUIRE(errno == 0);
}

TEST_CASE("UniqueFd self-move assignment keeps the fd open", "[UniqueFd]") {
  const int fd = eventfd(0, EFD_CLOEXEC);
  REQUIRE(fd > -1);
  auto ufd = tmd::UniqueFd(fd);
  auto &alias = ufd;

  ufd = std::move(alias);
  REQUIRE(ufd.get() == fd);

  REQUIRE_FALSE(fd_is_closed(fd));
}

TEST_CASE("UniqueFd release doesn't close the fd", "[UniqueFd]") {
  const int fd1 = eventfd(0, EFD_CLOEXEC);
  REQUIRE(fd1 > -1);
  auto ufd = tmd::UniqueFd(fd1);

  const int fd2 = ufd.release();
  CHECK(ufd.get() == -1);

  REQUIRE_FALSE(fd_is_closed(fd2));

  (void)close(fd2);
}

TEST_CASE("UniqueFd reset closes the old fd and adopts the new one",
          "[UniqueFd]") {
  const int fd1 = eventfd(0, EFD_CLOEXEC);
  REQUIRE(fd1 > -1);
  auto ufd = tmd::UniqueFd(fd1);
  const int fd2 = eventfd(0, EFD_CLOEXEC);
  REQUIRE(fd2 > -1);

  ufd.reset(fd2);
  REQUIRE(ufd.get() == fd2);

  REQUIRE(fd_is_closed(fd1));
}

TEST_CASE("UniqueFd reset without argument closes the old fd and leaves the "
          "object empty",
          "[UniqueFd]") {
  const int fd = eventfd(0, EFD_CLOEXEC);
  REQUIRE(fd > -1);
  auto ufd = tmd::UniqueFd(fd);

  ufd.reset();
  REQUIRE(ufd.get() == -1);

  REQUIRE(fd_is_closed(fd));
}

TEST_CASE("UniqueFd reset with already owned descriptor as argument is no-op",
          "[UniqueFd]") {
  const int fd = eventfd(0, EFD_CLOEXEC);
  REQUIRE(fd > -1);
  auto ufd = tmd::UniqueFd(fd);

  ufd.reset(fd);
  REQUIRE(ufd.get() == fd);

  REQUIRE_FALSE(fd_is_closed(fd));
}

TEST_CASE("UniqueFd reset on an empty object must not call close",
          "[UniqueFd]") {
  const int fd = eventfd(0, EFD_CLOEXEC);
  REQUIRE(fd > -1);
  auto ufd = tmd::UniqueFd();

  errno = 0;
  ufd.reset(fd);
  REQUIRE(errno == 0);
  REQUIRE(ufd.get() == fd);
}

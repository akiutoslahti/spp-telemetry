# Plan

A three-week learning project. The goal is to learn modern C++ (C++20) and Buildroot properly by building one small, real system instead of working through tutorials: a CCSDS Space Packet library, a Linux telemetry daemon that uses it, a Python packet generator to feed it, and a Buildroot image that runs the daemon under QEMU.

## Ground rules

- All code and configuration is written by hand. An AI assistant is used as a reviewer, a build runner and a reference for documentation, never for writing code. The rules are in CLAUDE.md.
- One repository, small commits with real messages. The history is part of the result.
- Build on Linux. Under WSL2 the repository and the Buildroot tree live inside the Linux filesystem, not under /mnt/c. Buildroot needs a case-sensitive filesystem. Keep 20 GB free.
- Scope is fixed. If a week slips, cut from that week's cut list. Do not extend.

## The system

Four parts in one repository.

1. libspp: a C++20 library for the CCSDS Space Packet Protocol primary header (CCSDS 133.0-B-2, free PDF from public.ccsds.org).
2. tmd: a Linux user space telemetry daemon in C++20 that receives Space Packets over UDP, validates them, keeps per-APID statistics and answers queries over a Unix domain socket.
3. spp-gen: a Python packet generator with fault injection, tested with pytest.
4. br2-external: a Buildroot external tree with a package for tmd, an init script, a QEMU aarch64 defconfig and a root filesystem overlay.

### libspp

Primary header, 6 octets:

- version (3 bits, must be 0), type (1 bit, 0 telemetry, 1 telecommand), secondary header flag (1 bit), APID (11 bits, 0x7FF is idle)
- sequence flags (2 bits), sequence count (14 bits)
- packet data length (16 bits), stored as length minus one, so the data field is 1–65536 octets

Design:

- Parse from `std::span<const std::byte>`, serialize into a caller-provided span. No allocation in the hot path.
- Explicit big-endian handling with shifts. No casts through struct pointers. Know why (alignment, aliasing, endianness).
- Strong types: `enum class` for packet type and sequence flags, a small `Apid` type with range-checked construction.
- Errors as values. A 40-line `Expected<T, Error>` on `std::variant` is the first exercise. It stands in for `std::expected` (C++23).
- `SequenceTracker`: per-APID 14-bit sequence count with wraparound and gap detection.
- The library throws nothing. Many embedded teams build with `-fno-exceptions`. Be able to explain the trade-off.
- The secondary header is mission specific. Expose the flag and the raw data field. Do not model it.

Tests (Catch2 v3 through FetchContent): golden byte vectors from the standard, round trips, both length boundaries, every malformed case (bad version, truncated header, truncated data, length mismatch), sequence wraparound, a random round-trip property test. Target 30 or more test cases.

Build: CMake presets for Debug with ASan and UBSan, Release, and a clang-tidy preset. Warnings `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`. A `.clang-format` from the first commit.

### tmd

- Single thread. `epoll` over three descriptors: the UDP socket, the Unix domain control socket, a `signalfd` for SIGTERM and SIGINT. Clean shutdown.
- File descriptors wrapped in an RAII `UniqueFd` (move only, closes in the destructor). This is the rule-of-five exercise.
- Parser and statistics are pure code with unit tests. The socket layer is thin and gets one smoke test.
- Per-APID statistics: packets, bytes, sequence gaps, malformed packets, last seen (`std::chrono`).
- Control socket answers `stats` with a text dump and `reset`. Test from the shell with `socat` or `nc -U`.
- Logging through `syslog(3)`, so it works under BusyBox syslogd and under journald without changes.
- Configuration from command line flags only: UDP port, control socket path, log level. No config library.
- Writes a pid file and runs in the foreground. The init system handles the rest.

### spp-gen

Hand-written Python 3, standard library only: `argparse`, `struct`, `socket`. Modes: a valid stream over N APIDs at a given rate, and fault injection (bad version, length mismatch, truncated packet, sequence gap, idle packets). Packaged properly: `pyproject.toml`, a console entry point, type hints checked with `mypy --strict`, `pytest` tests for the packing functions.

### br2-external

- Start from `qemu_aarch64_virt_defconfig`. Build it unchanged and boot it with the command line in `board/qemu/aarch64-virt/readme.txt`. Add `hostfwd=udp::5555-:5555` to the user network device so the host can reach the daemon.
- External tree: `external.desc`, `external.mk`, `Config.in`, `package/tmd/Config.in`, `package/tmd/tmd.mk` on the `cmake-package` infrastructure with `TMD_SITE_METHOD = local` pointing at the repository.
- Init script `S90tmd` installed through `TMD_INSTALL_INIT_SYSV`. Stretch: switch the defconfig to systemd and add a unit through `TMD_INSTALL_INIT_SYSTEMD`, so both init systems have been touched.
- `configs/tmd_qemu_aarch64_defconfig` saved with `make savedefconfig`. Enables tmd, BusyBox syslogd, socat, dropbear.
- Root filesystem overlay for `/etc/tmd/` and a post-build script that writes the git revision into `/etc/tmd-release`.
- Acceptance: the image boots, `ps` shows tmd started by init, `logread` shows its startup line, spp-gen on the host sends packets through the forwarded port, `socat - UNIX-CONNECT:/run/tmd.sock` inside the guest shows the counts and the injected faults.
- Builds take one to two hours on a laptop. Start them early and do hands-on work while they run. Iterate with `make tmd-rebuild`, not full rebuilds.

## Week by week

### Week 1, library (10 h)

- Toolchain (1 h): GCC 13 or newer and Clang, CMake, Ninja, clang-format, clang-tidy. Empty repository with CMake, Catch2, one failing test, GitHub Actions running it. Green CI before any real code.
- Reading (2 h across the week): CCSDS 133.0-B-2 section 4, the packet format, about ten pages. A Tour of C++ (3rd edition, covers C++20) chapters 1–6: basics, user-defined types, modularity, classes, essential operations, templates.
- Code (6 h): `Expected` first, then header parse and serialize, then `SequenceTracker`. Test first.
- Buffer (1 h).
- Done when: CI green on GCC and Clang, sanitizer build clean, 30 tests, README with the header diagram.
- Cut list if slipping: the property test, the clang-tidy preset.

### Week 2, daemon and generator (10 h)

- Reading (1 h): A Tour of C++ chapters 7–9 (concepts and generic programming, library overview, strings and regular expressions). Skim Core Guidelines sections R (resource management) and ES (expressions and statements).
- tmd (5 h): `UniqueFd`, epoll loop with signalfd, UDP receive into libspp, statistics, control socket, syslog, pid file.
- spp-gen (3 h): generator, packaging, mypy, pytest.
- CI (1 h): matrix of GCC and Clang, Debug with sanitizers and Release, clang-format check, mypy and pytest jobs.
- Buffer: none this week, the packaging hour took it.
- Done when: tmd runs on the host, spp-gen feeds it, injected faults show in the stats, SIGTERM shuts it down cleanly, ASan and UBSan stay clean under load.
- Cut list if slipping: the `reset` command, last-seen timestamps.

### Week 3, Buildroot and write-up (10 h plus unattended build time)

- Reading (1 h): the Buildroot manual, the chapters on using Buildroot, project-specific customization (BR2_EXTERNAL, overlays, post-build scripts) and adding new packages (the cmake-package section).
- Build 1 (1 h hands-on): stock qemu_aarch64_virt image, boot, networking with hostfwd.
- External tree and package (4 h): tmd package, init script, defconfig, overlay, post-build script.
- Acceptance run (1 h): the checklist above, terminal log saved into the repository.
- Write-up (2 h): README with architecture, build and run instructions, what was learned, honest limits.
- Buffer (1 h).
- Cut list if slipping: the systemd variant, the post-build script.

## Modern C++ checklist

Not all of this ends up in the code. Tick each item when you can explain it and have used it once.

- RAII, rule of zero, rule of five, move semantics, `std::move` versus `std::forward`
- `unique_ptr` by default, when `shared_ptr` is justified, why non-owning raw pointers still appear
- `const` correctness, references versus pointers, `std::span`, `std::string_view`, `std::byte`
- `enum class`, `constexpr` and `consteval`, structured bindings, designated initializers
- `std::optional`, `std::variant` with `std::visit`, the `Expected` pattern, `std::expected` in C++23
- templates, concepts, `auto` and deduction rules, lambdas and captures
- STL containers and algorithms, iterators, ranges basics
- `std::chrono`, `std::thread` and `std::jthread`, `std::mutex`, `std::atomic` (know them even though tmd is single threaded)
- undefined behaviour, sanitizers, strict aliasing, `memcpy` and `std::bit_cast` for type punning, `std::endian`
- exceptions versus error values, and what `-fno-exceptions` changes
- CMake targets, `target_link_libraries` visibility, presets, FetchContent

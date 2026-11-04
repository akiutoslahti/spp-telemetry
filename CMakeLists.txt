cmake_minimum_required(VERSION 3.28)
project(spp-telemetry LANGUAGES CXX)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

option(SPP_SANITIZE "Enable ASan and UBSan" OFF)

if (SPP_SANITIZE)
  set(SPP_SANITIZE_FLAGS
    "-fsanitize=address,undefined"
    "-fno-omit-frame-pointer"
    "-fno-sanitize-recover=all"
  )

  add_compile_options(${SPP_SANITIZE_FLAGS})
  add_link_options(${SPP_SANITIZE_FLAGS})
endif()

include(CTest)

if (BUILD_TESTING)
  include(FetchContent)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.16.1
    SYSTEM
  )
  FetchContent_MakeAvailable(Catch2)

  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
  include(Catch)
endif()

option(SPP_TIDY "Enable linting with clang-tidy" OFF)

if (SPP_TIDY)
  find_program(SPP_CLANG_TIDY_EXE NAMES "clang-tidy-18")
  if (SPP_CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY "${SPP_CLANG_TIDY_EXE}")
  else()
    message(FATAL_ERROR "clang-tidy-18 not found!")
  endif()
endif()

add_library(project-warnings INTERFACE)
target_compile_options(project-warnings INTERFACE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)

add_subdirectory(libspp)
add_subdirectory(tmd)

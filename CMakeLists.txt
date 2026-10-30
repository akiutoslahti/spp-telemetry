cmake_minimum_required(VERSION 3.28)
project(spp-telemetry LANGUAGES CXX)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(project-warnings INTERFACE)
target_compile_options(project-warnings INTERFACE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)

add_subdirectory(libspp)

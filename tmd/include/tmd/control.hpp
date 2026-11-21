#pragma once

#include <string>
#include <string_view>

#include <tmd/statistics.hpp>

namespace tmd {
[[nodiscard]] std::optional<std::string_view> find_command(std::string_view buf,
                                                           bool is_at_eof);
[[nodiscard]] std::string execute_command(std::string_view cmd,
                                          Statistics &stats);
} // namespace tmd

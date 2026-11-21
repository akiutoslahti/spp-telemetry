#include <algorithm>
#include <format>
#include <optional>
#include <string>
#include <string_view>

#include <tmd/control.hpp>
#include <tmd/statistics.hpp>

namespace tmd {
std::optional<std::string_view> find_command(std::string_view buf,
                                             bool is_at_eof) {
  auto nl_pos = buf.find_first_of('\n');
  if (nl_pos != std::string_view::npos) {
    return buf.substr(0, nl_pos);
  }

  if (is_at_eof && !buf.empty()) {
    return buf;
  }

  return std::nullopt;
}

std::string execute_command(std::string_view cmd, Statistics &stats) {
  constexpr std::string_view whitespace = " \t\r\n";

  cmd.remove_prefix(std::min(cmd.find_first_not_of(whitespace), cmd.size()));
  if (cmd.empty()) {
    return "Empty command\n";
  }

  cmd.remove_suffix(cmd.size() - cmd.find_last_not_of(whitespace) - 1);

  if (cmd == "stats") {
    return stats.report() + "\n";
  }

  if (cmd == "reset") {
    stats.reset();
    return "ok\n";
  }

  return std::format("Unknown command: \"{}\"\n", cmd);
}
} // namespace tmd

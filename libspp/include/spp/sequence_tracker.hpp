#pragma once

#include <spp/primary_header.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>

namespace spp {
enum class SequenceResult : std::uint8_t { InOrder, Gap, Duplicate, Ignored };

struct SequenceTrackerLogResult {
  SequenceResult result;
  std::uint16_t lost_count;
};

class SequenceTracker {
private:
  std::array<std::optional<spp::SequenceCount>, ApidMax + 1> m_data;

public:
  constexpr SequenceTracker() noexcept = default;

  // Logging ignores Idle Packets.
  // Early packets are logged with Gap close to maximum as there is no good
  // heuristic to distinguish between early and late/missing packets.
  [[nodiscard]] constexpr SequenceTrackerLogResult
  log(PacketPrimaryHeader header) noexcept {
    auto ret = SequenceTrackerLogResult{.result = SequenceResult::Ignored,
                                        .lost_count = 0};

    auto apid = header.apid.get();
    auto next = header.sequence_count;

    // Apid guarantees <= ApidMax and the array has ApidMax + 1 entries
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    auto previous = m_data[apid];

    if (apid == spp::ApidIdlePacket) {
      return ret;
    }

    if (!previous.has_value()) {
      ret.result = SequenceResult::InOrder;
    } else {
      const std::uint16_t distance = next.distance_from(*previous);

      if (distance == 0) {
        ret.result = SequenceResult::Duplicate;
      } else if (distance == 1) {
        ret.result = SequenceResult::InOrder;
      } else {
        ret.result = SequenceResult::Gap;
        ret.lost_count = distance - 1;
      }
    }

    // Apid guarantees <= ApidMax and the array has ApidMax + 1 entries
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    m_data[apid] = next;

    return ret;
  }

  constexpr void reset(Apid apid) noexcept {
    // Apid guarantees <= ApidMax and the array has ApidMax + 1 entries
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    m_data[apid.get()] = std::nullopt;
  }

  constexpr void reset_all() noexcept {
    std::fill(m_data.begin(), m_data.end(), std::nullopt);
  }
};
} // namespace spp

#pragma once

#include <spp/error.hpp>
#include <spp/expected.hpp>
#include <spp/packet.hpp>
#include <spp/sequence_tracker.hpp>

#include <array>
#include <cstdint>
#include <format>
#include <optional>
#include <sstream>
#include <string>

namespace tmd {
struct StatisticsEntry {
  std::uint64_t nr_packets;
  std::uint64_t nr_bytes;
  std::uint64_t nr_gap_events;
  std::uint64_t nr_lost_packets;
  std::uint64_t nr_duplicates;

  [[nodiscard]] friend bool
  operator==(const StatisticsEntry &,
             const StatisticsEntry &) noexcept = default;
};

std::string inline to_string(StatisticsEntry entry) {
  return std::format("packets={}, bytes={}, gaps={}, lost={}, duplicates={}",
                     entry.nr_packets, entry.nr_bytes, entry.nr_gap_events,
                     entry.nr_lost_packets, entry.nr_duplicates);
}

class Statistics {
private:
  std::array<std::optional<StatisticsEntry>, spp::ApidMax + 1> m_data = {};
  std::array<std::uint64_t, spp::NrErrors> m_errors = {};
  spp::SequenceTracker m_tracker;

public:
  constexpr Statistics() noexcept = default;

  void record(spp::Expected<spp::Packet, spp::Error> packet) noexcept {
    if (packet) {
      auto res = m_tracker.log(packet->header);
      const std::uint16_t apid = *packet->header.apid;
      const std::uint64_t packet_size =
          spp::PrimaryHeaderSize + 1 + packet->header.data_length;

      // Apid guarantees <= ApidMax and the array has ApidMax + 1 entries
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      auto &slot = m_data[apid];
      StatisticsEntry &entry = slot.has_value() ? *slot : slot.emplace();

      entry.nr_packets++;
      entry.nr_bytes += packet_size;

      switch (res.result) {
      case spp::SequenceResult::InOrder:
        break;
      case spp::SequenceResult::Gap:
        entry.nr_gap_events++;
        entry.nr_lost_packets += res.lost_count;
        break;
      case spp::SequenceResult::Duplicate:
        entry.nr_duplicates++;
        break;
      case spp::SequenceResult::Ignored:
        break;
      }
    } else {
      auto err = packet.error();
      auto idx = static_cast<std::size_t>(err);
      // Error guarantees < NrErrors and the array has NrErrors entries
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      auto &slot = m_errors[idx];
      slot++;
    }
  }

  [[nodiscard]] const std::optional<StatisticsEntry> &
  get(spp::Apid apid) const & noexcept {
    // Apid guarantees <= ApidMax and the array has ApidMax + 1 entries
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    return m_data[*apid];
  }

  [[nodiscard]] const std::array<std::uint64_t, spp::NrErrors> &
  get_errors() const & noexcept {
    return m_errors;
  }

  [[nodiscard]] std::string report() const {
    std::stringstream report;
    std::stringstream error_report;

    for (std::size_t i = 0; i < m_data.size(); i++) {
      // i is guaranteed to be < m_data.size()
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      const auto &slot = m_data[i];
      if (slot.has_value()) {
        if (report.tellp() == 0) {
          report << "Valid packets:";
        }
        const auto &entry = *slot;
        report << std::format("\n  APID={:#x}, ", i) << to_string(entry);
      }
    }

    if (report.tellp() == 0) {
      report << "Valid packets: None";
    }

    for (std::size_t i = 0; i < m_errors.size(); i++) {
      // i is guaranteed to be < m_errors.size()
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      if (m_errors[i] != 0) {
        if (error_report.tellp() != 0) {
          error_report << ", ";
        }
        // i is guaranteed to be < m_errors.size()
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
        error_report << std::format(
            "'{}'={}", to_string(static_cast<spp::Error>(i)), m_errors[i]);
        // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
      }
    }

    report << "\nErrors: ";
    if (error_report.tellp() == 0) {
      report << "None";
    } else {
      report << error_report.str();
    }

    return report.str();
  }

  void reset() noexcept {
    std::fill(m_data.begin(), m_data.end(), std::nullopt);
    std::fill(m_errors.begin(), m_errors.end(), 0);
    m_tracker.reset_all();
  }
};
} // namespace tmd

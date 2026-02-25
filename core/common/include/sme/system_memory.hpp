// System memory information

#pragma once

#include <cstddef>
#include <optional>

namespace sme::common {

/**
 * @brief System physical memory totals in bytes.
 */
struct SystemMemoryInfo {
  /**
   * @brief Total installed physical memory.
   */
  std::size_t totalPhysicalBytes{};
  /**
   * @brief Currently available physical memory.
   */
  std::size_t availablePhysicalBytes{};
};

/**
 * @brief Query physical memory information for the current machine.
 */
[[nodiscard]] std::optional<SystemMemoryInfo> getSystemMemoryInfo();
/**
 * @brief Query memory usage of the current process.
 */
[[nodiscard]] std::optional<std::size_t> getCurrentProcessMemoryUsageBytes();

} // namespace sme::common

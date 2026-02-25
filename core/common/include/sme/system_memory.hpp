// System memory information

#pragma once

#include <cstddef>
#include <optional>

namespace sme::common {

struct SystemMemoryInfo {
  std::size_t totalPhysicalBytes{};
  std::size_t availablePhysicalBytes{};
};

[[nodiscard]] std::optional<SystemMemoryInfo> getSystemMemoryInfo();
[[nodiscard]] std::optional<std::size_t> getCurrentProcessMemoryUsageBytes();

} // namespace sme::common

#include "sme/system_memory.hpp"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

#if defined(__APPLE__)
#include <mach/mach.h>
#include <sys/sysctl.h>
#endif

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// windows include has to come before psapi:
#include <windows.h>

// psapi assumes windows.h has been included:
#include <psapi.h>

#endif

namespace sme::common {

namespace {

[[nodiscard]] std::size_t toSizeT(std::uint64_t value) {
  return value > static_cast<std::uint64_t>(
                     std::numeric_limits<std::size_t>::max())
             ? std::numeric_limits<std::size_t>::max()
             : static_cast<std::size_t>(value);
}

#if defined(__linux__)

[[nodiscard]] std::optional<std::uint64_t>
readMeminfoBytes(const std::string &key) {
  std::ifstream file("/proc/meminfo");
  if (!file) {
    return {};
  }
  std::string label{};
  std::uint64_t valueInKb{0};
  std::string unit{};
  while (file >> label >> valueInKb >> unit) {
    if (!label.empty() && label.back() == ':') {
      label.pop_back();
    }
    if (label == key) {
      return valueInKb * 1024;
    }
    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  return {};
}

[[nodiscard]] std::optional<std::uint64_t>
readUintFromFile(const std::string &filename) {
  std::ifstream file(filename);
  if (!file) {
    return {};
  }
  std::string value{};
  file >> value;
  if (!file || value.empty() || value == "max") {
    return {};
  }
  try {
    return std::stoull(value);
  } catch (...) {
    return {};
  }
}

[[nodiscard]] std::optional<std::uint64_t>
readProcStatusBytes(const std::string &key) {
  std::ifstream file("/proc/self/status");
  if (!file) {
    return {};
  }
  std::string line{};
  while (std::getline(file, line)) {
    const auto prefix = key + ":";
    if (line.rfind(prefix, 0) != 0) {
      continue;
    }
    std::istringstream iss(line.substr(prefix.size()));
    std::uint64_t valueInKb{0};
    std::string unit{};
    if (iss >> valueInKb >> unit && unit == "kB") {
      return valueInKb * 1024;
    }
    return {};
  }
  return {};
}

[[nodiscard]] bool isUnconstrainedCgroupLimit(std::uint64_t limit) {
  constexpr std::uint64_t unconstrainedThreshold{1ULL << 60};
  return limit >= unconstrainedThreshold;
}

[[nodiscard]] std::optional<std::uint64_t> getCgroupLimitBytes() {
  if (auto limit = readUintFromFile("/sys/fs/cgroup/memory.max");
      limit.has_value() && !isUnconstrainedCgroupLimit(limit.value())) {
    return limit;
  }
  if (auto limit =
          readUintFromFile("/sys/fs/cgroup/memory/memory.limit_in_bytes");
      limit.has_value() && !isUnconstrainedCgroupLimit(limit.value())) {
    return limit;
  }
  return {};
}

[[nodiscard]] std::optional<std::uint64_t> getCgroupAvailableBytes() {
  if (const auto limit = readUintFromFile("/sys/fs/cgroup/memory.max"),
      usage = readUintFromFile("/sys/fs/cgroup/memory.current");
      limit.has_value() && usage.has_value() &&
      !isUnconstrainedCgroupLimit(limit.value())) {
    return usage.value() >= limit.value() ? 0 : limit.value() - usage.value();
  }
  if (const auto limit =
          readUintFromFile("/sys/fs/cgroup/memory/memory.limit_in_bytes"),
      usage = readUintFromFile("/sys/fs/cgroup/memory/memory.usage_in_bytes");
      limit.has_value() && usage.has_value() &&
      !isUnconstrainedCgroupLimit(limit.value())) {
    return usage.value() >= limit.value() ? 0 : limit.value() - usage.value();
  }
  return {};
}

#endif

} // namespace

std::optional<SystemMemoryInfo> getSystemMemoryInfo() {
#if defined(__linux__)
  auto total = readMeminfoBytes("MemTotal");
  auto available = readMeminfoBytes("MemAvailable");
  if (!available.has_value()) {
    available = readMeminfoBytes("MemFree");
  }
  if (!total.has_value() || !available.has_value()) {
    return {};
  }
  if (auto cgroupLimit = getCgroupLimitBytes(); cgroupLimit.has_value()) {
    total = std::min(total.value(), cgroupLimit.value());
  }
  if (auto cgroupAvailable = getCgroupAvailableBytes();
      cgroupAvailable.has_value()) {
    available = std::min(available.value(), cgroupAvailable.value());
  }
  if (available.value() > total.value()) {
    available = total;
  }
  return SystemMemoryInfo{toSizeT(total.value()), toSizeT(available.value())};
#elif defined(__APPLE__)
  std::uint64_t totalBytes{0};
  std::size_t totalBytesLen{sizeof(totalBytes)};
  if (sysctlbyname("hw.memsize", &totalBytes, &totalBytesLen, nullptr, 0) !=
      0) {
    return {};
  }
  vm_size_t pageSize{0};
  if (host_page_size(mach_host_self(), &pageSize) != KERN_SUCCESS) {
    return {};
  }
  vm_statistics64_data_t vmStats{};
  mach_msg_type_number_t count{HOST_VM_INFO64_COUNT};
  if (host_statistics64(mach_host_self(), HOST_VM_INFO64,
                        reinterpret_cast<host_info64_t>(&vmStats),
                        &count) != KERN_SUCCESS) {
    return {};
  }
  const std::uint64_t availablePages{
      static_cast<std::uint64_t>(vmStats.free_count) +
      static_cast<std::uint64_t>(vmStats.inactive_count) +
      static_cast<std::uint64_t>(vmStats.speculative_count)};
  const std::uint64_t availableBytes{availablePages *
                                     static_cast<std::uint64_t>(pageSize)};
  return SystemMemoryInfo{toSizeT(totalBytes), toSizeT(availableBytes)};
#elif defined(_WIN32)
  MEMORYSTATUSEX memoryStatus{};
  memoryStatus.dwLength = sizeof(memoryStatus);
  if (!GlobalMemoryStatusEx(&memoryStatus)) {
    return {};
  }
  return SystemMemoryInfo{toSizeT(memoryStatus.ullTotalPhys),
                          toSizeT(memoryStatus.ullAvailPhys)};
#else
  return {};
#endif
}

std::optional<std::size_t> getCurrentProcessMemoryUsageBytes() {
#if defined(__linux__)
  if (const auto residentBytes = readProcStatusBytes("VmRSS");
      residentBytes.has_value()) {
    return toSizeT(residentBytes.value());
  }
  return {};
#elif defined(__APPLE__)
  mach_task_basic_info taskInfo{};
  mach_msg_type_number_t count{MACH_TASK_BASIC_INFO_COUNT};
  if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                reinterpret_cast<task_info_t>(&taskInfo),
                &count) != KERN_SUCCESS) {
    return {};
  }
  return toSizeT(taskInfo.resident_size);
#elif defined(_WIN32)
  PROCESS_MEMORY_COUNTERS counters{};
  if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters))) {
    return {};
  }
  return toSizeT(counters.WorkingSetSize);
#else
  return {};
#endif
}

} // namespace sme::common

// Runtime availability check for CUDA libraries.
// The actual CUDA API functions are provided as stub implementations
// in cuda_stubs.cpp that forward to dlopen'd libraries at runtime.

#pragma once

#include <cstddef>
#include <optional>
#include <string>

namespace sme::simulate {

struct CudaMemoryInfo {
  std::size_t totalBytes{};
  std::size_t availableBytes{};
};

// Attempts to load the CUDA driver library at runtime (thread-safe, loads
// once). Returns true if the library was loaded and all symbols resolved.
[[nodiscard]] bool isCudaAvailable();

// Returns a human-readable reason why CUDA is unavailable.
// Only meaningful when isCudaAvailable() returns false.
[[nodiscard]] const std::string &cudaUnavailableReason();

// Returns total and currently available memory for CUDA device 0.
// Returns std::nullopt if the CUDA driver, device, or query is unavailable.
[[nodiscard]] std::optional<CudaMemoryInfo> getCudaMemoryInfo();

} // namespace sme::simulate

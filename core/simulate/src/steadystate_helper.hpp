#pragma once
#include <functional>
#include <vector>

namespace sme {
namespace simulate {
class SteadyStateHelper {
public:
  [[nodiscard]] virtual std::vector<double> getDcdt() const = 0;
  [[nodiscard]] virtual std::size_t
  run(double timeout_ms, const std::function<bool()> &stopRunningCallback) = 0;
  virtual ~SteadyStateHelper() = default;
};
} // namespace simulate
} // namespace sme

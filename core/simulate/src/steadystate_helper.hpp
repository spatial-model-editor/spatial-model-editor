#pragma once
#include <functional>

namespace sme {
namespace simulate {

/**
 * @brief Base class for steady state simulations
 *
 */
class SteadyStateHelper {
public:
  [[nodiscard]] virtual double
  run(double timeout_ms, const std::function<bool()> &stopRunningCallback) = 0;
  virtual ~SteadyStateHelper() = default;
};
} // namespace simulate
} // namespace sme

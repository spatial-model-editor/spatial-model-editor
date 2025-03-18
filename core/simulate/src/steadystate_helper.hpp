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
  [[nodiscard]] virtual double getStopTolerance() const = 0;
  [[nodiscard]] virtual std::vector<double> getConcentrations() const = 0;
  [[nodiscard]] virtual double
  compute_stopping_criterion(const std::vector<double> &c_old,
                             const std::vector<double> &c_new, double dt) = 0;
  [[nodiscard]] virtual double
  run(double timeout_ms, const std::function<bool()> &stopRunningCallback) = 0;
  virtual ~SteadyStateHelper() = default;
};
} // namespace simulate
} // namespace sme

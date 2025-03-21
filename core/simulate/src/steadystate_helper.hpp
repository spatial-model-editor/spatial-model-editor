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
  [[nodiscard]] virtual double getCurrentError() const = 0;
  [[nodiscard]] virtual std::size_t getStepsBelowTolerance() const = 0;
  [[nodiscard]] virtual std::size_t getNumStepsSteady() const = 0;
  virtual void setNumStepsSteady(std::size_t new_numstepssteady) = 0;
  virtual void setStopTolerance(double stop_tolerance) = 0;
  [[nodiscard]] virtual double
  computeStoppingCriterion(const std::vector<double> &c_old,
                           const std::vector<double> &c_new, double dt) = 0;
  virtual ~SteadyStateHelper() = default;
};
} // namespace simulate
} // namespace sme
